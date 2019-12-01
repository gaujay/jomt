// Copyright 2019 Guillaume AUJAY. All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "plotter_3dsurface.h"
#include "ui_plotter_3dsurface.h"

#include "benchmark_results.h"
#include "plot_parameters.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtDataVisualization>
using namespace QtDataVisualization;


Plotter3DSurface::Plotter3DSurface(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                                   const PlotParams &plotParams, const QString &filename,
                                   QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Plotter3DSurface)
{
    // UI
    ui->setupUi(this);
    this->setWindowTitle("3D Surface - " + filename);
    
    /*
     * Options
     */
    // Theme
    ui->comboBoxTheme->addItem("Primary Colors",    Q3DTheme::ThemePrimaryColors);
    ui->comboBoxTheme->addItem("Digia",             Q3DTheme::ThemeDigia);
    ui->comboBoxTheme->addItem("StoneMoss",         Q3DTheme::ThemeStoneMoss);
    ui->comboBoxTheme->addItem("ArmyBlue",          Q3DTheme::ThemeArmyBlue);
    ui->comboBoxTheme->addItem("Retro",             Q3DTheme::ThemeRetro);
    ui->comboBoxTheme->addItem("Ebony",             Q3DTheme::ThemeEbony);
    ui->comboBoxTheme->addItem("Isabelle",          Q3DTheme::ThemeIsabelle);
    ui->comboBoxTheme->addItem("Qt",                Q3DTheme::ThemeQt);
    connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DSurface::onComboThemeChanged);
    
    //TODO: selection mode
    
    // Surface
    connect(ui->checkBoxFlip,  &QCheckBox::stateChanged, this, &Plotter3DSurface::onCheckFlip);
    
    setupGradients();
    connect(ui->comboBoxGradient, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DSurface::onComboGradientChanged);
    
    // Axes
    ui->comboBoxAxis->addItem("X-Axis");
    ui->comboBoxAxis->addItem("Y-Axis");
    ui->comboBoxAxis->addItem("Z-Axis");
    connect(ui->comboBoxAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DSurface::onComboAxisChanged);
    
    connect(ui->checkBoxAxisRotate,  &QCheckBox::stateChanged, this, &Plotter3DSurface::onCheckAxisRotate);
    connect(ui->checkBoxTitle,       &QCheckBox::stateChanged, this, &Plotter3DSurface::onCheckTitleVisible);
    connect(ui->checkBoxLog,         &QCheckBox::stateChanged, this, &Plotter3DSurface::onCheckLog);
    connect(ui->spinBoxLogBase,      QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DSurface::onSpinLogBaseChanged);
    connect(ui->lineEditTitle,       &QLineEdit::textEdited, this, &Plotter3DSurface::onEditTitleChanged);
    connect(ui->lineEditFormat,      &QLineEdit::textEdited, this, &Plotter3DSurface::onEditFormatChanged);
    connect(ui->doubleSpinBoxMin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DSurface::onSpinMinChanged);
    connect(ui->doubleSpinBoxMax,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DSurface::onSpinMaxChanged);
    connect(ui->spinBoxTicks,        QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DSurface::onSpinTicksChanged);
    connect(ui->spinBoxMTicks,       QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DSurface::onSpinMTicksChanged);
    
    // Snapshot
    connect(ui->pushButtonSnapshot, &QPushButton::clicked, this, &Plotter3DSurface::onSnapshotClicked);
    
    
    /*
     *  Chart
     */
    mSurface = new Q3DSurface();
    QWidget *container = QWidget::createWindowContainer(mSurface);
        
    
    // 3D
    // X: argumentA or templateB
    // Y: time/iter/bytes/items (not name dependent)
    // Z: argumentC or templateD (with C!=A, D!=B)
    bool custXAxis = true, custZAxis = true;
    QString custXName, custZName;
    bool hasZParam = plotParams.zType != PlotEmptyType;
    
    //
    // No Z-param -> one row per benchmark type
    if (!hasZParam)
    {
        // Single series (i.e. color)
        QSurfaceDataProxy *dataProxy = new QSurfaceDataProxy();
        QSurface3DSeries  *series = new QSurface3DSeries(dataProxy);
        
        QSurfaceDataArray *dataArray = new QSurfaceDataArray;
        
        // Segment per X-param
        QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                                bchIdxs, plotParams.xIdx, "X");
        // Check subsets symmetry/min size
        bool symBchOK = true, symOK = true, minOK = true;
        QString culpritName;
        int refSize = bchSubsets[0].idxs.size();
        for (int i=0; symOK && minOK && i<bchSubsets.size(); ++i) {
            symOK = bchSubsets[i].idxs.size() == refSize;
            minOK = bchSubsets[i].idxs.size() >= 2;
            if (!symOK || !minOK)
                culpritName = bchSubsets[0].name;
        }
        // Ignore asymmetrical series
        if (!symOK) {
            qWarning() << "Inconsistent number of X-values between benchmarks to trace surface for:" << culpritName;
        }
        // Ignore single-row series
        else if (!minOK) {
            qWarning() << "Not enough X-values to trace surface for:" << culpritName;
        }
        else
        {
            int prevRowSize = 0;
            double zFallback = 0.;
            for (const auto& bchSubset : bchSubsets)
            {
                // Check inter benchmark consistency
                if (prevRowSize > 0 && prevRowSize != bchSubset.idxs.size()) {
                    symBchOK = false;
                    qWarning() << "Inconsistent number of X-values between benchmarks to trace surface";
                    break;
                }
                prevRowSize = bchSubset.idxs.size();
                
                // One row per X-group
                QSurfaceDataRow *newRow = new QSurfaceDataRow( bchSubset.idxs.size() );
                
//                const QString & subsetName = bchSubset.name;
//                qDebug() << "subsetName:" << subsetName;
//                qDebug() << "subsetIdxs:" << bchSubset.idxs;
                
                int index = 0;
                double xFallback = 0.;
                for (int idx : bchSubset.idxs)
                {
                    QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                            idx, plotParams.xIdx);
                    double xVal = BenchResults::getParamValue(xName, custXName, custXAxis, xFallback);
                    
                    // Y val
                    double yVal = getYPlotValue(bchResults.benchmarks[idx], plotParams.yType);
//                    qDebug() << "-> [" << xVal << yVal << zFallback << "]";
                    
                    // Add column
                    (*newRow)[index++].setPosition( QVector3D(xVal, yVal, zFallback) );
                }
                // Add row
                *dataArray << newRow;
                
                ++zFallback;
            }
        }
        if (symBchOK && dataArray->size() > 0)
        {
            // Add series
            dataProxy->resetArray(dataArray);
            
            series->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
            series->setFlatShadingEnabled(true);
            series->setItemLabelFormat(QStringLiteral("[@xLabel, @zLabel]: @yLabel"));
            
            mSurface->addSeries(series);
        }
    }
    //
    // Z-param -> one series per benchmark type
    else
    {
        // Initial segmentation by 'full name % param1 % param2' (group benchmarks)
        const auto bchNames = bchResults.segment2DNames(bchIdxs,
                                                        plotParams.xType == PlotArgumentType, plotParams.xIdx,
                                                        plotParams.zType == PlotArgumentType, plotParams.zIdx);
        for (const auto& bchName : bchNames)
        {
            // One series (i.e. color) per 2D-name
            QSurfaceDataProxy *dataProxy = new QSurfaceDataProxy();
            QSurface3DSeries  *series = new QSurface3DSeries(dataProxy);
            
            QSurfaceDataArray *dataArray = new QSurfaceDataArray;
            
//            qDebug() << "bchName" << bchName.name << "|" << bchName.idxs;
            
            // One subset per Z-param from 2D-names
            QVector<BenchSubset> bchZSubs = bchResults.segmentParam(plotParams.zType == PlotArgumentType,
                                                                    bchName.idxs, plotParams.zIdx);
            // Ignore incompatible series
            if ( bchZSubs.isEmpty() ) {
                qWarning() << "No Z-value to trace surface for other benchmarks";
                continue;
            }
            
            // Check subsets symmetry/min size
            bool symOK = true, minOK = true;
            QString culpritName;
            int refSize = bchZSubs[0].idxs.size();
            for (int i=0; symOK && minOK && i<bchZSubs.size(); ++i) {
                symOK = bchZSubs[i].idxs.size() == refSize;
                minOK = bchZSubs[i].idxs.size() >= 2;
                if (!symOK || !minOK)
                    culpritName = bchZSubs[0].name;
            }
            // Ignore asymmetrical series
            if (!symOK) {
                qWarning() << "Inconsistent number of X-values between benchmarks to trace surface for:"
                           << bchName.name + "[Z=" + culpritName + "]";
                continue;
            }
            // Ignore single-row series
            else if (!minOK) {
                qWarning() << "Not enough X-values to trace surface for:"
                           << bchName.name + "[Z=" + culpritName + "]";
                continue;
            }

            double zFallback = 0.;
            for (const auto& bchZSub : bchZSubs)
            {
                QString zName = bchZSub.name;
//                qDebug() << "bchZSub" << bchZSub.name << "|" << bchZSub.idxs;

                double zVal = BenchResults::getParamValue(zName, custZName, custZAxis, zFallback);
                
                // One row per Z-param from 2D-names
                QSurfaceDataRow *newRow = new QSurfaceDataRow( bchZSub.idxs.size() );
                
                // One subset per X-param from Z-Subset
                QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                                        bchZSub.idxs, plotParams.xIdx, "X");
                for (const auto& bchSubset : bchSubsets)
                {
                    int index = 0;
                    double xFallback = 0.;
                    for (int idx : bchSubset.idxs)
                    {
                        QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                                idx, plotParams.xIdx);
                        double xVal = BenchResults::getParamValue(xName, custXName, custXAxis, xFallback);
                        
                        // Y val
                        double yVal = getYPlotValue(bchResults.benchmarks[idx], plotParams.yType);
//                        qDebug() << "-> [" << xVal << yVal << zVal << "]";
                        
                        // Add column
                        (*newRow)[index++].setPosition( QVector3D(xVal, yVal, zVal) );
                    }
                    // Add row
                    *dataArray << newRow;
                }
            }
            // Add series
            dataProxy->resetArray(dataArray);
            
            series->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
            series->setFlatShadingEnabled(true);
            series->setName(bchName.name);
            series->setItemLabelFormat(QStringLiteral("@seriesName [@xLabel, @zLabel]: @yLabel"));
            
            mSurface->addSeries(series);
        }
    }
    
    
    
    /*
     *  Chart options
     */
    if ( !mSurface->seriesList().isEmpty() && mSurface->seriesList()[0]->dataProxy()->rowCount() > 0)
    {
        // General
        mSurface->setHorizontalAspectRatio(1.0);
        mSurface->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
        
        // X-axis
        QValue3DAxis *xAxis = mSurface->axisX();
        if (plotParams.xType == PlotArgumentType)
            xAxis->setTitle("Argument " + QString::number(plotParams.xIdx+1));
        else { // template
            if ( !custXName.isEmpty() )
                xAxis->setTitle(custXName);
            else
                xAxis->setTitle("Template " + QString::number(plotParams.xIdx+1));
        }
        xAxis->setTitleVisible(true);
        xAxis->setSegmentCount(8);
        
        // Y-axis
        QValue3DAxis *yAxis = mSurface->axisY();
        yAxis->setTitle( getYPlotName(plotParams.yType) );
        yAxis->setTitleVisible(true);
        
        // Z-axis
        QValue3DAxis *zAxis = mSurface->axisZ();
        if (plotParams.zType != PlotEmptyType)
        {
            if (plotParams.zType == PlotArgumentType)
                zAxis->setTitle("Argument " + QString::number(plotParams.zIdx+1));
            else { // template
                if ( !custZName.isEmpty() )
                    zAxis->setTitle(custZName);
                else
                    zAxis->setTitle("Template " + QString::number(plotParams.zIdx+1));
            }
            zAxis->setTitleVisible(true);
        }
        zAxis->setSegmentCount(8);
    }
    else {
        // Title-like
        QValue3DAxis *yAxis = mSurface->axisY();
        yAxis->setTitle("No compatible series to display");
        yAxis->setTitleVisible(true);

        qWarning() << "No compatible series to display";
    }
    
    
    // Options
    mSurface->activeTheme()->setType(Q3DTheme::ThemePrimaryColors);
    
    // Axes options
    mIgnoreEvents = true;
    QValue3DAxis *xAxis = mSurface->axisX();
    if (xAxis)
    {
        axesParams[0].titleText = xAxis->title();
        axesParams[0].title = !axesParams[0].titleText.isEmpty();
        axesParams[0].labelFormat = xAxis->labelFormat();
        axesParams[0].min = xAxis->min();
        axesParams[0].max = xAxis->max();
        axesParams[0].ticks = xAxis->segmentCount();
        axesParams[0].mticks = xAxis->subSegmentCount();
        
        ui->checkBoxTitle->setChecked( axesParams[0].title );
        ui->lineEditTitle->setText( axesParams[0].titleText );
        ui->lineEditFormat->setText( axesParams[0].labelFormat );
        ui->doubleSpinBoxMin->setValue( axesParams[0].min );
        ui->doubleSpinBoxMax->setValue( axesParams[0].max );
        ui->spinBoxTicks->setValue( axesParams[0].ticks );
        ui->spinBoxMTicks->setValue( axesParams[0].mticks );
    }
    QValue3DAxis *yAxis = mSurface->axisY();
    if (yAxis)
    {
        axesParams[1].titleText = yAxis->title();
        axesParams[1].title = !axesParams[1].titleText.isEmpty();
        axesParams[1].labelFormat = yAxis->labelFormat();
        axesParams[1].min = yAxis->min();
        axesParams[1].max = yAxis->max();
        axesParams[1].ticks = yAxis->segmentCount();
        axesParams[1].mticks = yAxis->subSegmentCount();
    }
    QValue3DAxis *zAxis = mSurface->axisZ();
    if (zAxis)
    {
        axesParams[2].titleText = zAxis->title();
        axesParams[2].title = !axesParams[2].titleText.isEmpty();
        axesParams[2].labelFormat = zAxis->labelFormat();
        axesParams[2].min = zAxis->min();
        axesParams[2].max = zAxis->max();
        axesParams[2].ticks = zAxis->segmentCount();
        axesParams[2].mticks = zAxis->subSegmentCount();
    }
    mIgnoreEvents = false;
    
    
    // Show
    ui->horizontalLayout->insertWidget(0, container, 1);
}

Plotter3DSurface::~Plotter3DSurface()
{
    delete ui;
}


void Plotter3DSurface::setupGradients()
{
    ui->comboBoxGradient->addItem("No gradient");
    
    ui->comboBoxGradient->addItem("Deep volcano"); {
        QLinearGradient gr;
        gr.setColorAt(0.0,  Qt::black); gr.setColorAt(0.33, Qt::blue);
        gr.setColorAt(0.67, Qt::red);   gr.setColorAt(1.0,  Qt::yellow);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Jungle heat"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::darkGreen); gr.setColorAt(0.5, Qt::yellow);
        gr.setColorAt(0.8, Qt::red);       gr.setColorAt(1.0, Qt::darkRed);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Spectral redux"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::blue);   gr.setColorAt(0.33, Qt::green);
        gr.setColorAt(0.5, Qt::yellow); gr.setColorAt(1.0,  Qt::red);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Spectral extended"); {
        QLinearGradient gr;
        gr.setColorAt(0.0,  Qt::magenta); gr.setColorAt(0.25, Qt::blue);
        gr.setColorAt(0.5,  Qt::cyan);
        gr.setColorAt(0.67, Qt::green);   gr.setColorAt(0.83, Qt::yellow);
        gr.setColorAt(1.0,  Qt::red);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Reddish"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::darkRed); gr.setColorAt(1.0, Qt::red);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Greenish"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::darkGreen); gr.setColorAt(1.0, Qt::green);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Bluish"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::darkCyan); gr.setColorAt(1.0, Qt::cyan);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Gray"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::black); gr.setColorAt(1.0, Qt::white);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Gray inverted"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::white); gr.setColorAt(1.0, Qt::black);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Gray centered"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::black); gr.setColorAt(0.5, Qt::white);
        gr.setColorAt(1.0, Qt::black);
        mGrads.push_back(gr);
    }
    
    ui->comboBoxGradient->addItem("Gray inv-centered"); {
        QLinearGradient gr;
        gr.setColorAt(0.0, Qt::white); gr.setColorAt(0.5, Qt::black);
        gr.setColorAt(1.0, Qt::white);
        mGrads.push_back(gr);
    }
}

//
// Theme
void Plotter3DSurface::onComboThemeChanged(int index)
{
    Q3DTheme::Theme theme = static_cast<Q3DTheme::Theme>(
                ui->comboBoxTheme->itemData(index).toInt());
    mSurface->activeTheme()->setType(theme);
    
    onComboGradientChanged( ui->comboBoxGradient->currentIndex() );
}

//
// Surface
void Plotter3DSurface::onCheckFlip(int state)
{
    mSurface->setFlipHorizontalGrid(state == Qt::Checked);
}

void Plotter3DSurface::onComboGradientChanged(int idx)
{
    if (idx == 0)
    {
        for (auto& series : mSurface->seriesList())
            series->setColorStyle(Q3DTheme::ColorStyleUniform);
    }
    else
    {
        for (auto& series : mSurface->seriesList()) {
            series->setBaseGradient( mGrads[idx-1] );
            series->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
        }
    }
}

//
// Axes
void Plotter3DSurface::onComboAxisChanged(int idx)
{
    // Update UI
    mIgnoreEvents = true;
    
    ui->checkBoxAxisRotate->setChecked( axesParams[idx].rotate );
    ui->checkBoxTitle->setChecked( axesParams[idx].title );
    ui->checkBoxLog->setChecked( axesParams[idx].log );
    ui->spinBoxLogBase->setValue( axesParams[idx].logBase );
    ui->spinBoxLogBase->setEnabled( ui->checkBoxLog->isChecked() );
    ui->lineEditTitle->setText( axesParams[idx].titleText );
    ui->lineEditFormat->setText( axesParams[idx].labelFormat );
    ui->doubleSpinBoxMin->setValue( axesParams[idx].min );
    ui->doubleSpinBoxMax->setValue( axesParams[idx].max );
    ui->doubleSpinBoxMin->setSingleStep(idx == 1 ? 0.1 : 1.0);
    ui->doubleSpinBoxMax->setSingleStep(idx == 1 ? 0.1 : 1.0);
    ui->spinBoxTicks->setValue( axesParams[idx].ticks );
    ui->spinBoxTicks->setEnabled( !ui->checkBoxLog->isChecked() );
    ui->spinBoxMTicks->setValue( axesParams[idx].mticks );
    ui->spinBoxMTicks->setEnabled( !ui->checkBoxLog->isChecked() );
    
    mIgnoreEvents = false;
}

void Plotter3DSurface::onCheckAxisRotate(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setTitleFixed(state != Qt::Checked);
        axis->setLabelAutoRotation(state == Qt::Checked ? 90 : 0);
        axesParams[iAxis].rotate = state == Qt::Checked;
    }
}

void Plotter3DSurface::onCheckTitleVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setTitleVisible(state == Qt::Checked);
        axesParams[iAxis].title = state == Qt::Checked;
    }
}

void Plotter3DSurface::onCheckLog(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis)
    {
        if (state == Qt::Checked) {
            axis->setFormatter(new QLogValue3DAxisFormatter());
            ui->doubleSpinBoxMin->setMinimum( 0.001 );
            axesParams[iAxis].min = axis->min();
        }
        else {
            axis->setFormatter(new QValue3DAxisFormatter());
            ui->doubleSpinBoxMin->setMinimum( 0. );
            axesParams[iAxis].max = axis->max();
        }
        axesParams[iAxis].log = state == Qt::Checked;
        ui->spinBoxTicks->setEnabled(  state != Qt::Checked);
        ui->spinBoxMTicks->setEnabled( state != Qt::Checked);
        ui->spinBoxLogBase->setEnabled(state == Qt::Checked);
    }
}

void Plotter3DSurface::onSpinLogBaseChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis)
    {
        QLogValue3DAxisFormatter* formatter = (QLogValue3DAxisFormatter*)axis->formatter();
        if (formatter) {
            formatter->setBase(i);
            axesParams[iAxis].logBase = i;
        }
    }
}

void Plotter3DSurface::onEditTitleChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setTitle(text);
        axesParams[iAxis].titleText = text;
    }
}

void Plotter3DSurface::onEditFormatChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setLabelFormat(text);
        axesParams[iAxis].labelFormat = text;
    }
}

void Plotter3DSurface::onSpinMinChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setMin(d);
        axesParams[iAxis].min = d;
    }
}

void Plotter3DSurface::onSpinMaxChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setMax(d);
        axesParams[iAxis].max = d;
    }
}

void Plotter3DSurface::onSpinTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setSegmentCount(i);
        axesParams[iAxis].ticks = i;
    }
}

void Plotter3DSurface::onSpinMTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QValue3DAxis* axis;
    if      (iAxis == 0)    axis = mSurface->axisX();
    else if (iAxis == 1)    axis = mSurface->axisY();
    else                    axis = mSurface->axisZ();
    
    if (axis) {
        axis->setSubSegmentCount(i);
        axesParams[iAxis].mticks = i;
    }
}

//
// Snapshot
void Plotter3DSurface::onSnapshotClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save snapshot"), "", tr("Images (*.png)"));
    
    if ( !fileName.isEmpty() )
    {
        QImage image = mSurface->renderToImage(8);

        bool ok = image.save(fileName, "PNG");
        if (!ok)
            QMessageBox::warning(this, "Chart snapshot", "Error saving snapshot file.");
    }
}
