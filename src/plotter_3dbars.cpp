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

#include "plotter_3dbars.h"
#include "ui_plotter_3dbars.h"

#include "benchmark_results.h"
#include "plot_parameters.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtDataVisualization>
using namespace QtDataVisualization;


Plotter3DBars::Plotter3DBars(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                             const PlotParams &plotParams, const QString &filename,
                             QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Plotter3DBars)
{
    // UI
    ui->setupUi(this);
    this->setWindowTitle("3D Bars - " + filename);
    
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
    connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DBars::onComboThemeChanged);
    
    //TODO: selection mode
    
    // Bars
    setupGradients();
    connect(ui->comboBoxGradient, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DBars::onComboGradientChanged);
    
    connect(ui->doubleSpinBoxThickness, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinThicknessChanged);
    connect(ui->doubleSpinBoxFloor,     QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinFloorChanged);
    connect(ui->doubleSpinBoxSpacingX,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinSpaceXChanged);
    connect(ui->doubleSpinBoxSpacingZ,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinSpaceZChanged);
    
    
    // Axes
    ui->comboBoxAxis->addItem("X-Axis");
    ui->comboBoxAxis->addItem("Y-Axis");
    ui->comboBoxAxis->addItem("Z-Axis");
    connect(ui->comboBoxAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DBars::onComboAxisChanged);
    
    connect(ui->checkBoxAxisRotate,  &QCheckBox::stateChanged, this, &Plotter3DBars::onCheckAxisRotate);
    connect(ui->checkBoxTitle,       &QCheckBox::stateChanged, this, &Plotter3DBars::onCheckTitleVisible);
    connect(ui->checkBoxLog,         &QCheckBox::stateChanged, this, &Plotter3DBars::onCheckLog);
    connect(ui->spinBoxLogBase,      QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DBars::onSpinLogBaseChanged);
    connect(ui->lineEditTitle,       &QLineEdit::textEdited, this, &Plotter3DBars::onEditTitleChanged);
    connect(ui->lineEditFormat,      &QLineEdit::textEdited, this, &Plotter3DBars::onEditFormatChanged);
    connect(ui->doubleSpinBoxMin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinMinChanged);
    connect(ui->doubleSpinBoxMax,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Plotter3DBars::onSpinMaxChanged);
    connect(ui->comboBoxMin,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DBars::onComboMinChanged);
    connect(ui->comboBoxMax,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Plotter3DBars::onComboMaxChanged);
    connect(ui->spinBoxTicks,        QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DBars::onSpinTicksChanged);
    connect(ui->spinBoxMTicks,       QOverload<int>::of(&QSpinBox::valueChanged), this, &Plotter3DBars::onSpinMTicksChanged);
    
    // Snapshot
    connect(ui->pushButtonSnapshot, &QPushButton::clicked, this, &Plotter3DBars::onSnapshotClicked);
    
    
    /*
     *  Chart
     */
    mBars = new Q3DBars();
    QWidget *container = QWidget::createWindowContainer(mBars);
    
    
    // 3D
    // X: argumentA or templateB
    // Y: time/iter/bytes/items (not name dependent)
    // Z: argumentC or templateD (with C!=A, D!=B)
    bool hasZParam = plotParams.zType != PlotEmptyType;
    
    //
    // No Z-param -> one row per benchmark type
    if (!hasZParam)
    {
        // Single series (i.e. color)
        QBar3DSeries *series = new QBar3DSeries;

        QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                                bchIdxs, plotParams.xIdx, "X");
        bool firstCol = true;
        for (const auto& bchSubset : bchSubsets)
        {
            // One row per benchmark * X-group
            QBarDataRow *data = new QBarDataRow;
            
            const QString & subsetName = bchSubset.name;
//            qDebug() << "subsetName:" << subsetName;
//            qDebug() << "subsetIdxs:" << bchSubset.idxs;
            
            QStringList colLabels;
            for (int idx : bchSubset.idxs)
            {
                QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                        idx, plotParams.xIdx);
                colLabels.append(xName);
                
                // Add column
                data->append( static_cast<float>(getYPlotValue(bchResults.benchmarks[idx], plotParams.yType)) );
            }
            // Add benchmark row
            series->dataProxy()->addRow(data, subsetName);
            
            // Set column labels (only if no collision, empty otherwise)
            if (firstCol) // init
                series->dataProxy()->setColumnLabels(colLabels);
            else if ( commonPartEqual(series->dataProxy()->columnLabels(), colLabels) ) {
                if (series->dataProxy()->columnLabels().size() < colLabels.size()) // replace by longest
                    series->dataProxy()->setColumnLabels(colLabels);
            }
            else { // collision
                series->dataProxy()->setColumnLabels( QStringList("") );
            }
            firstCol = false;
//            qDebug() << "[Multi-NoZ] colLabels:" << colLabels << "|" << series->dataProxy()->columnLabels();
        }
        // Add series
        series->setItemLabelFormat(QStringLiteral("@rowLabel [X=@colLabel]: @valueLabel"));
        series->setMesh(QAbstract3DSeries::MeshBevelBar);
        series->setMeshSmooth(false);
        
        mBars->addSeries(series);
    }
    //
    // Z-param -> one series per benchmark, one row per Z, one column per X
    else
    {
        // Initial segmentation by 'full name % param1 % param2' (group benchmarks)
        const auto bchNames = bchResults.segment2DNames(bchIdxs,
                                                        plotParams.xType == PlotArgumentType, plotParams.xIdx,
                                                        plotParams.zType == PlotArgumentType, plotParams.zIdx);
        QStringList prevRowLabels, prevColLabels;
        bool sameRowLabels = true, sameColLabels = true;
        for (const auto& bchName : bchNames)
        {
            // One series (i.e. color) per 2D name
            QBar3DSeries *series = new QBar3DSeries;
//            qDebug() << "bchName" << bchName.name << "|" << bchName.idxs;
            
            // Segment: one sub per Z-param from 2D names
            QVector<BenchSubset> bchZSubs = bchResults.segmentParam(plotParams.zType == PlotArgumentType,
                                                                    bchName.idxs, plotParams.zIdx);
            QStringList curRowLabels;
            for (const auto& bchZSub : bchZSubs)
            {
//                qDebug() << "bchZSub" << bchZSub.name << "|" << bchZSub.idxs;
                curRowLabels.append(bchZSub.name);
                
                // One row per Z-param from 2D names
                QBarDataRow *data = new QBarDataRow;
                
                // Group: one column per X-param
                QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                                        bchZSub.idxs, plotParams.xIdx, "X");
                for (const auto& bchSubset : bchSubsets)
                {
                    QStringList curColLabels;
                    for (int idx : bchSubset.idxs)
                    {
                        QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                                idx, plotParams.xIdx);
                        curColLabels.append(xName);
                        
                        // Y-values on row
                        data->append( static_cast<float>(getYPlotValue(bchResults.benchmarks[idx], plotParams.yType)) );
                    }
                    // Add benchmark row
                    series->dataProxy()->addRow(data);
                                        
                    // Check column labels collisions
                    if (sameColLabels) {
                        if ( prevColLabels.isEmpty() ) // init
                            prevColLabels = curColLabels;
                        else {
                            if ( commonPartEqual(prevColLabels, curColLabels) ) {
                                if (prevColLabels.size() < curColLabels.size()) // replace by longest
                                    prevColLabels = curColLabels;
                            }
                            else sameColLabels = false;
                        }
                    }
//                    qDebug() << "[Multi-Z] curColLabels:" << curColLabels << "|" << prevColLabels;
                }
            }
            // Check row labels collisions
            if (sameRowLabels) {
                if ( prevRowLabels.isEmpty() ) // init
                    prevRowLabels = curRowLabels;
                else {
                    if ( commonPartEqual(prevRowLabels, curRowLabels) ) {
                        if (prevRowLabels.size() < curRowLabels.size()) // replace by longest
                            prevRowLabels = curRowLabels;
                    }
                    else sameRowLabels = false;
                }
            }
//            qDebug() << "[Multi-Z] curRowLabels:" << curRowLabels << "|" << prevRowLabels;
            //
            // Add series
            series->setName( bchName.name );
            series->setItemLabelFormat(QStringLiteral("@seriesName [@colLabel, @rowLabel]: @valueLabel"));
            series->setMesh(QAbstract3DSeries::MeshBevelBar);
            series->setMeshSmooth(false);
            
            mBars->addSeries(series);
        }
        // Set row/column labels (empty if collisions)
        if ( !mBars->seriesList().isEmpty() && mBars->seriesList()[0]->dataProxy()->rowCount() > 0)
        {
            for (auto &series : mBars->seriesList()) {
                series->dataProxy()->setColumnLabels(sameColLabels ? prevColLabels : QStringList(""));
                series->dataProxy()->setRowLabels(   sameRowLabels ? prevRowLabels : QStringList(""));
            }
        }
    }
    
    
    /*
     *  Chart options
     */
    if ( !mBars->seriesList().isEmpty() && mBars->seriesList()[0]->dataProxy()->rowCount() > 0)
    {
        // General
        mBars->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
        
        // X-axis
        QCategory3DAxis *colAxis = mBars->columnAxis();
        if (plotParams.xType == PlotArgumentType)
            colAxis->setTitle("Argument " + QString::number(plotParams.xIdx+1));
        else if (plotParams.xType == PlotTemplateType)
            colAxis->setTitle("Template " + QString::number(plotParams.xIdx+1));
        if (plotParams.xType != PlotEmptyType)
            colAxis->setTitleVisible(true);
        
        // Y-axis
        QValue3DAxis *valAxis = mBars->valueAxis();
        valAxis->setTitle( getYPlotName(plotParams.yType) );
        valAxis->setTitleVisible(true);
        
        // Z-axis
        if (plotParams.zType != PlotEmptyType)
        {
            QCategory3DAxis *rowAxis = mBars->rowAxis();
            if (plotParams.zType == PlotArgumentType)
                rowAxis->setTitle("Argument " + QString::number(plotParams.zIdx+1));
            else
                rowAxis->setTitle("Template " + QString::number(plotParams.zIdx+1));
            rowAxis->setTitleVisible(true);
        }
    }
    else {
        // Title-like
        QCategory3DAxis *colAxis = mBars->columnAxis();
        colAxis->setTitle("No compatible series to display");
        colAxis->setTitleVisible(true);
    }
    
    
    // Options
    mBars->activeTheme()->setType(Q3DTheme::ThemePrimaryColors);
    
    // Axes options
    mIgnoreEvents = true;
    // X-axis
    QCategory3DAxis *colAxis = mBars->columnAxis();
    if (colAxis)
    {
        axesParams[0].titleText = colAxis->title();
        axesParams[0].title = !axesParams[0].titleText.isEmpty();
        
        ui->doubleSpinBoxMin->setVisible(false);
        ui->doubleSpinBoxMax->setVisible(false);
        
        ui->lineEditTitle->setText( axesParams[0].titleText );
        if ( !colAxis->labels().isEmpty() && !colAxis->labels()[0].isEmpty() ) {
            axesParams[0].range = colAxis->labels();
        }
        else if ( !mBars->seriesList().isEmpty() ) {
            int maxCol = 0;
            for (const auto& series : mBars->seriesList())
                for (int iR=0; iR < series->dataProxy()->rowCount(); ++iR)
                    if (maxCol < series->dataProxy()->rowAt(iR)->size())
                        maxCol = series->dataProxy()->rowAt(iR)->size();
            for (int i=0; i<maxCol; ++i)
                axesParams[0].range.append( QString::number(i+1) );
        }
        ui->comboBoxMin->addItems( axesParams[0].range );
        ui->comboBoxMax->addItems( axesParams[0].range );
        ui->comboBoxMax->setCurrentIndex( ui->comboBoxMax->count()-1 );
        axesParams[0].maxIdx = ui->comboBoxMax->count()-1;
    }
    // Y-axis
    QValue3DAxis *valAxis = mBars->valueAxis();
    if (valAxis)
    {
        axesParams[1].titleText = valAxis->title();
        axesParams[1].title = !axesParams[1].titleText.isEmpty();
        
        ui->doubleSpinBoxFloor->setMinimum( valAxis->min() );
        ui->doubleSpinBoxFloor->setMaximum( valAxis->max() );
        ui->lineEditFormat->setText( valAxis->labelFormat() );
        ui->doubleSpinBoxMin->setValue( valAxis->min() );
        ui->doubleSpinBoxMax->setValue( valAxis->max() );
        ui->spinBoxTicks->setValue( valAxis->segmentCount() );
        ui->spinBoxMTicks->setValue( valAxis->subSegmentCount() );
    }
    // Z-axis
    QCategory3DAxis *rowAxis = mBars->rowAxis();
    if (rowAxis)
    {
        axesParams[2].titleText = rowAxis->title();
        axesParams[2].title = !axesParams[2].titleText.isEmpty();
        if ( !rowAxis->labels().isEmpty() && !rowAxis->labels()[0].isEmpty() ) {
            axesParams[2].range = rowAxis->labels();
        }
        else if ( !mBars->seriesList().isEmpty() ) {
            int maxRow = 0;
            for (const auto& series : mBars->seriesList())
                if (maxRow < series->dataProxy()->rowCount())
                    maxRow = series->dataProxy()->rowCount();
            for (int i=0; i<maxRow; ++i)
                axesParams[2].range.append( QString::number(i+1) );
        }
        axesParams[2].maxIdx = axesParams[2].range.size()-1;
    }
    mIgnoreEvents = false;
    
    
    // Show
    ui->horizontalLayout->insertWidget(0, container, 1);
}

Plotter3DBars::~Plotter3DBars()
{
    delete ui;
}


void Plotter3DBars::setupGradients()
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
void Plotter3DBars::onComboThemeChanged(int index)
{
    Q3DTheme::Theme theme = static_cast<Q3DTheme::Theme>(
                ui->comboBoxTheme->itemData(index).toInt());
    mBars->activeTheme()->setType(theme);
    
    onComboGradientChanged( ui->comboBoxGradient->currentIndex() );
}

//
// Bars
void Plotter3DBars::onComboGradientChanged(int idx)
{
    if (idx == 0)
    {
        for (auto& series : mBars->seriesList())
            series->setColorStyle(Q3DTheme::ColorStyleUniform);
    }
    else
    {
        for (auto& series : mBars->seriesList()) {
            series->setBaseGradient( mGrads[idx-1] );
            series->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
        }
    }
}

void Plotter3DBars::onSpinThicknessChanged(double d)
{
    mBars->setBarThickness(d);
}

void Plotter3DBars::onSpinFloorChanged(double d)
{
    mBars->setFloorLevel(d);
}

void Plotter3DBars::onSpinSpaceXChanged(double d)
{
    QSizeF barSpacing = mBars->barSpacing();
    barSpacing.setWidth(d);
    
    mBars->setBarSpacing(barSpacing);
}

void Plotter3DBars::onSpinSpaceZChanged(double d)
{
    QSizeF barSpacing = mBars->barSpacing();
    barSpacing.setHeight(d);
    
    mBars->setBarSpacing(barSpacing);
}

//
// Axes
void Plotter3DBars::onComboAxisChanged(int idx)
{
    // Update UI
    mIgnoreEvents = true;
    
    ui->checkBoxAxisRotate->setChecked( axesParams[idx].rotate );
    ui->checkBoxTitle->setChecked( axesParams[idx].title );
    ui->checkBoxLog->setEnabled( idx == 1 );
    ui->spinBoxLogBase->setEnabled( ui->checkBoxLog->isEnabled() && ui->checkBoxLog->isChecked() );
    ui->lineEditTitle->setText( axesParams[idx].titleText );
    ui->lineEditFormat->setEnabled( idx == 1 );
    ui->comboBoxMin->setVisible( idx != 1 );
    ui->comboBoxMax->setVisible( idx != 1 );
    if (idx != 1) {
        ui->comboBoxMin->clear();
        ui->comboBoxMax->clear();
        ui->comboBoxMin->addItems( axesParams[idx].range );
        ui->comboBoxMax->addItems( axesParams[idx].range );
        ui->comboBoxMin->setCurrentIndex( axesParams[idx].minIdx );
        ui->comboBoxMax->setCurrentIndex( axesParams[idx].maxIdx );
    }
    ui->doubleSpinBoxMin->setVisible( idx == 1 );
    ui->doubleSpinBoxMax->setVisible( idx == 1 );
    ui->spinBoxTicks->setEnabled(  idx == 1 && !ui->checkBoxLog->isChecked() );
    ui->spinBoxMTicks->setEnabled( idx == 1 && !ui->checkBoxLog->isChecked() );
    
    mIgnoreEvents = false;
}

void Plotter3DBars::onCheckAxisRotate(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QAbstract3DAxis* axis;
    if      (iAxis == 0)    axis = mBars->columnAxis();
    else if (iAxis == 1)    axis = mBars->valueAxis();
    else                    axis = mBars->rowAxis();
    
    if (axis) {
        axis->setTitleFixed(state != Qt::Checked);
        axis->setLabelAutoRotation(state == Qt::Checked ? 90 : 0);
        axesParams[iAxis].rotate = state == Qt::Checked;
    }
}

void Plotter3DBars::onCheckTitleVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QAbstract3DAxis* axis;
    if      (iAxis == 0)    axis = mBars->columnAxis();
    else if (iAxis == 1)    axis = mBars->valueAxis();
    else                    axis = mBars->rowAxis();
    
    if (axis) {
        axis->setTitleVisible(state == Qt::Checked);
        axesParams[iAxis].title = state == Qt::Checked;
    }
}

void Plotter3DBars::onCheckLog(int state)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QValue3DAxis* axis = mBars->valueAxis();
    
    if (axis)
    {
        if (state == Qt::Checked) {
            axis->setFormatter(new QLogValue3DAxisFormatter());
            ui->doubleSpinBoxMin->setMinimum(0.001);
        }
        else {
            axis->setFormatter(new QValue3DAxisFormatter());
            ui->doubleSpinBoxMin->setMinimum(0.);
        }
        ui->spinBoxTicks->setEnabled(  state != Qt::Checked);
        ui->spinBoxMTicks->setEnabled( state != Qt::Checked);
        ui->spinBoxLogBase->setEnabled(state == Qt::Checked);
    }
}

void Plotter3DBars::onSpinLogBaseChanged(int i)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QValue3DAxis* axis = mBars->valueAxis();
    
    if (axis)
    {
        QLogValue3DAxisFormatter* formatter = (QLogValue3DAxisFormatter*)axis->formatter();
        if (formatter)
            formatter->setBase(i);
    }
}

void Plotter3DBars::onEditTitleChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QAbstract3DAxis* axis;
    if      (iAxis == 0)    axis = mBars->columnAxis();
    else if (iAxis == 1)    axis = mBars->valueAxis();
    else                    axis = mBars->rowAxis();
    
    if (axis) {
        axis->setTitle(text);
        axesParams[iAxis].titleText = text;
    }
}

void Plotter3DBars::onEditFormatChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QValue3DAxis* axis = mBars->valueAxis();
    
    if (axis) {
        axis->setLabelFormat(text);
    }
}

void Plotter3DBars::onSpinMinChanged(double d)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QAbstract3DAxis* axis = mBars->valueAxis();
    
    if (axis) {
        axis->setMin(d);
    }
}

void Plotter3DBars::onSpinMaxChanged(double d)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QAbstract3DAxis* axis = mBars->valueAxis();
    
    if (axis) {
        axis->setMax(d);
    }
}

void Plotter3DBars::onComboMinChanged(int index)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QAbstract3DAxis* axis;
    if      (iAxis == 0)    axis = mBars->columnAxis();
    else                    axis = mBars->rowAxis();
    
    if (axis) {
        axis->setMin(index);
        axesParams[iAxis].minIdx = index;
    }
}

void Plotter3DBars::onComboMaxChanged(int index)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    QAbstract3DAxis* axis;
    if      (iAxis == 0)    axis = mBars->columnAxis();
    else                    axis = mBars->rowAxis();
    
    if (axis) {
        axis->setMax(index);
        axesParams[iAxis].maxIdx = index;
    }
}

void Plotter3DBars::onSpinTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QValue3DAxis* axis = mBars->valueAxis();
    
    if (axis) {
        axis->setSegmentCount(i);
    }
}

void Plotter3DBars::onSpinMTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    assert(ui->comboBoxAxis->currentIndex() == 1);
    QValue3DAxis* axis = mBars->valueAxis();
    
    if (axis) {
        axis->setSubSegmentCount(i);
    }
}

//
// Snapshot
void Plotter3DBars::onSnapshotClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save snapshot"), "", tr("Images (*.png)"));
    
    if ( !fileName.isEmpty() )
    {
        QImage image = mBars->renderToImage(8);

        bool ok = image.save(fileName, "PNG");
        if (!ok)
            QMessageBox::warning(this, "Chart snapshot", "Error saving snapshot file.");
    }
}
