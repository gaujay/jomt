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

#include "plotter_barchart.h"
#include "ui_plotter_barchart.h"

#include "benchmark_results.h"
#include "plot_parameters.h"

#include <QtCharts>
using namespace QtCharts;


PlotterBarChart::PlotterBarChart(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                                 const PlotParams &plotParams, const QString &filename,
                                 QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlotterBarChart)
{
    ui->setupUi(this);
    mIsVert = plotParams.type == ChartBarType;
    if (mIsVert)
        this->setWindowTitle("Bars - " + filename);
    else
        this->setWindowTitle("HBars - " + filename);
    
    /*
     * Options
     */
    // Theme
    ui->comboBoxTheme->addItem("Light",         QChart::ChartThemeLight);
    ui->comboBoxTheme->addItem("Blue Cerulean", QChart::ChartThemeBlueCerulean);
    ui->comboBoxTheme->addItem("Dark",          QChart::ChartThemeDark);
    ui->comboBoxTheme->addItem("Brown Sand",    QChart::ChartThemeBrownSand);
    ui->comboBoxTheme->addItem("Blue Ncs",      QChart::ChartThemeBlueNcs);
    ui->comboBoxTheme->addItem("High Contrast", QChart::ChartThemeHighContrast);
    ui->comboBoxTheme->addItem("Blue Icy",      QChart::ChartThemeBlueIcy);
    ui->comboBoxTheme->addItem("Qt",            QChart::ChartThemeQt);
    connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboThemeChanged);
    
    // Legend
    connect(ui->checkBoxLegendVisible, &QCheckBox::stateChanged, this, &PlotterBarChart::onCheckLegendVisible);
    
    ui->comboBoxLegendAlign->addItem("Top",     Qt::AlignTop);
    ui->comboBoxLegendAlign->addItem("Bottom",  Qt::AlignBottom);
    ui->comboBoxLegendAlign->addItem("Left",    Qt::AlignLeft);
    ui->comboBoxLegendAlign->addItem("Right",   Qt::AlignRight);
    connect(ui->comboBoxLegendAlign, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboLegendAlignChanged);
    
    connect(ui->spinBoxLegendFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinLegendFontSizeChanged);
    
    // Axes
    ui->comboBoxAxis->addItem("X-Axis");
    ui->comboBoxAxis->addItem("Y-Axis");
    connect(ui->comboBoxAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboAxisChanged);
    
    ui->comboBoxValuePosition->addItem("None",       -1);
    ui->comboBoxValuePosition->addItem("Center",     QAbstractBarSeries::LabelsCenter);
    ui->comboBoxValuePosition->addItem("InsideEnd",  QAbstractBarSeries::LabelsInsideEnd);
    ui->comboBoxValuePosition->addItem("InsideBase", QAbstractBarSeries::LabelsInsideBase);
    ui->comboBoxValuePosition->addItem("OutsideEnd", QAbstractBarSeries::LabelsOutsideEnd);
    connect(ui->comboBoxValuePosition, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboValuePositionChanged);
    
    ui->comboBoxValueAngle->addItem("Right", 360.);
    ui->comboBoxValueAngle->addItem("Up",     -90.);
    ui->comboBoxValueAngle->addItem("Down",    90.);
    connect(ui->comboBoxValueAngle, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboValueAngleChanged);
    
    connect(ui->checkBoxAxisVisible, &QCheckBox::stateChanged, this, &PlotterBarChart::onCheckAxisVisible);
    connect(ui->checkBoxTitle,       &QCheckBox::stateChanged, this, &PlotterBarChart::onCheckTitleVisible);
    connect(ui->checkBoxLog,         &QCheckBox::stateChanged, this, &PlotterBarChart::onCheckLog);
    connect(ui->spinBoxLogBase,      QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinLogBaseChanged);
    connect(ui->lineEditTitle,       &QLineEdit::textEdited, this, &PlotterBarChart::onEditTitleChanged);
    connect(ui->spinBoxTitleSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinTitleSizeChanged);
    connect(ui->lineEditFormat,      &QLineEdit::textEdited, this, &PlotterBarChart::onEditFormatChanged);
    connect(ui->spinBoxLabelSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinLabelSizeChanged);
    connect(ui->doubleSpinBoxMin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterBarChart::onSpinMinChanged);
    connect(ui->doubleSpinBoxMax,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterBarChart::onSpinMaxChanged);
    connect(ui->comboBoxMin,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboMinChanged);
    connect(ui->comboBoxMax,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBarChart::onComboMaxChanged);
    connect(ui->spinBoxTicks,        QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinTicksChanged);
    connect(ui->spinBoxMTicks,       QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBarChart::onSpinMTicksChanged);
    
    // Snapshot
    connect(ui->pushButtonSnapshot, &QPushButton::clicked, this, &PlotterBarChart::onSnapshotClicked);
    
    
    /*
     *  Chart
     */
    QChart* chart = new QChart();
    
    // Single series, one barset per benchmark type
    QAbstractBarSeries* series;
    if (mIsVert)    series = new QBarSeries();
    else            series = new QHorizontalBarSeries();
    
    
    // 2D Bars
    // X: argumentA or templateB
    // Y: time/iter/bytes/items (not name dependent)
    // Bar: one per benchmark % X-param
    QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                            bchIdxs, plotParams.xIdx, "X");
    // Ignore empty series
    if ( bchSubsets.isEmpty() ) {
        qWarning() << "No compatible series to display";
    }
    
    bool firstCol = true;
    QStringList prevColLabels;
    for (const auto& bchSubset : bchSubsets)
    {
        // Ignore empty set
        if ( bchSubset.idxs.isEmpty() ) {
            qWarning() << "No X-value to trace bar for:" << bchSubset.name;
            continue;
        }
        
        const QString& subsetName = bchSubset.name;
//        qDebug() << "subsetName:" << subsetName;
//        qDebug() << "subsetIdxs:" << bchSubset.idxs;
        
        // X-row
        QBarSet* barSet = new QBarSet( subsetName.toHtmlEscaped() );
        
        QStringList colLabels;
        for (int idx : bchSubset.idxs)
        {
            QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                    idx, plotParams.xIdx);
            colLabels.append( xName.toHtmlEscaped() );
            
            // Add column
            *barSet << getYPlotValue(bchResults.benchmarks[idx], plotParams.yType);
        }
        // Add set (i.e. color)
        series->append(barSet);
        
        // Set column labels (only if no collision, empty otherwise)
        if (firstCol) // init
            prevColLabels = colLabels;
        else if ( commonPartEqual(prevColLabels, colLabels) ) {
            if (prevColLabels.size() < colLabels.size()) // replace by longest
                prevColLabels = colLabels;
        }
        else { // collision
            prevColLabels = QStringList("");
        }
        firstCol = false;
    }
    // Add the series
    chart->addSeries(series);
    
    
    /*
     *  Chart options
     */
    if (series->count() > 0)
    {
        // Chart type
        Qt::Alignment catAlign = mIsVert ? Qt::AlignBottom : Qt::AlignLeft;
        Qt::Alignment valAlign = mIsVert ? Qt::AlignLeft   : Qt::AlignBottom;
        
        // X-axis
        QBarCategoryAxis* catAxis = new QBarCategoryAxis();
        catAxis->append(prevColLabels);
        chart->addAxis(catAxis, catAlign);
        series->attachAxis(catAxis);
        if (plotParams.xType == PlotArgumentType)
            catAxis->setTitleText("Argument " + QString::number(plotParams.xIdx+1));
        else if (plotParams.xType == PlotTemplateType)
            catAxis->setTitleText("Template " + QString::number(plotParams.xIdx+1));
        
        // Y-axis
        QValueAxis* valAxis = new QValueAxis();
        chart->addAxis(valAxis, valAlign);
        series->attachAxis(valAxis);
        valAxis->applyNiceNumbers();
        valAxis->setTitleText( getYPlotName(plotParams.yType) );
    }
    else
        chart->setTitle("No compatible series to display");
    
    
    // View
    mChartView = new QChartView(chart);
    mChartView->setRenderHint(QPainter::Antialiasing);
    
    // Options
    chart->setTheme(QChart::ChartThemeLight);
//    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setShowToolTips(true);
    ui->spinBoxLegendFontSize->setValue( chart->legend()->font().pointSize() );
    
    // Axes options
    mIgnoreEvents = true;
    Qt::Orientation xOrient = mIsVert ? Qt::Horizontal : Qt::Vertical;
    const auto& xAxes = chart->axes(xOrient);
    if ( !xAxes.isEmpty() )
    {
        QBarCategoryAxis* xAxis = (QBarCategoryAxis*)(xAxes.first());
        
        axesParams[0].titleText = xAxis->titleText();
        axesParams[0].titleSize = xAxis->titleFont().pointSize();
        axesParams[0].labelSize = xAxis->labelsFont().pointSize();
        
        ui->labelFormat->setVisible(false);
        ui->lineEditFormat->setVisible(false);
        ui->doubleSpinBoxMin->setVisible(false);
        ui->doubleSpinBoxMax->setVisible(false);
        
        ui->lineEditTitle->setText( axesParams[0].titleText );
        ui->spinBoxTitleSize->setValue( axesParams[0].titleSize );
        ui->spinBoxLabelSize->setValue( axesParams[0].labelSize );
        for (const auto& cat : xAxis->categories()) {
            ui->comboBoxMin->addItem(cat);
            ui->comboBoxMax->addItem(cat);
        }
        ui->comboBoxMax->setCurrentIndex( ui->comboBoxMax->count()-1 );
        
    }
    Qt::Orientation yOrient = mIsVert ? Qt::Vertical : Qt::Horizontal;
    const auto& yAxes = chart->axes(yOrient);
    if ( !yAxes.isEmpty() )
    {
        QValueAxis* yAxis = (QValueAxis*)(yAxes.first());
        
        axesParams[1].titleText = yAxis->titleText();
        axesParams[1].titleSize = yAxis->titleFont().pointSize();
        axesParams[1].labelSize = yAxis->labelsFont().pointSize();
        
        ui->lineEditFormat->setText( yAxis->labelFormat() );
        ui->doubleSpinBoxMin->setValue( yAxis->min() );
        ui->doubleSpinBoxMax->setValue( yAxis->max() );
        ui->spinBoxTicks->setValue( yAxis->tickCount() );
        ui->spinBoxMTicks->setValue( yAxis->minorTickCount() );
    }
    mIgnoreEvents = false;
    
    // Show
    ui->horizontalLayout->insertWidget(0, mChartView);
}

PlotterBarChart::~PlotterBarChart()
{
    delete ui;
}

//
// Theme
void PlotterBarChart::onComboThemeChanged(int index)
{
    QChart::ChartTheme theme = static_cast<QChart::ChartTheme>(
                ui->comboBoxTheme->itemData(index).toInt());
    mChartView->chart()->setTheme(theme);
}

//
// Legend
void PlotterBarChart::onCheckLegendVisible(int state)
{
    mChartView->chart()->legend()->setVisible(state == Qt::Checked);
}

void PlotterBarChart::onComboLegendAlignChanged(int index)
{
    Qt::Alignment align = static_cast<Qt::Alignment>(
                ui->comboBoxLegendAlign->itemData(index).toInt());
    mChartView->chart()->legend()->setAlignment(align);
}

void PlotterBarChart::onSpinLegendFontSizeChanged(int i)
{
    QFont font = mChartView->chart()->legend()->font();
    font.setPointSize(i);
    mChartView->chart()->legend()->setFont(font);
}

//
// Axes
void PlotterBarChart::onComboAxisChanged(int idx)
{
    // Update UI
    mIgnoreEvents = true;
    
    ui->checkBoxAxisVisible->setChecked( axesParams[idx].visible );
    ui->checkBoxTitle->setChecked( axesParams[idx].title );
    ui->checkBoxLog->setEnabled( idx == 1 );
    ui->spinBoxLogBase->setEnabled( ui->checkBoxLog->isEnabled() && ui->checkBoxLog->isChecked() );
    ui->lineEditTitle->setText( axesParams[idx].titleText );
    ui->spinBoxTitleSize->setValue( axesParams[idx].titleSize );
    ui->labelFormat->setVisible( idx == 1 );
    ui->lineEditFormat->setVisible( idx == 1 );
    ui->labelValue->setVisible( idx == 0 );
    ui->comboBoxValuePosition->setVisible( idx == 0 );
    ui->comboBoxValueAngle->setVisible( idx == 0 );
    ui->spinBoxLabelSize->setValue( axesParams[idx].labelSize );
    ui->comboBoxMin->setVisible( idx == 0 );
    ui->comboBoxMax->setVisible( idx == 0 );
    ui->doubleSpinBoxMin->setVisible( idx == 1 );
    ui->doubleSpinBoxMax->setVisible( idx == 1 );
    ui->spinBoxTicks->setEnabled(   idx == 1 && !ui->checkBoxLog->isChecked() );
    ui->spinBoxMTicks->setEnabled(  idx == 1 );
    
    mIgnoreEvents = false;
}

void PlotterBarChart::onCheckAxisVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setVisible(state == Qt::Checked);
        axesParams[iAxis].visible = state == Qt::Checked;
    }
}

void PlotterBarChart::onCheckTitleVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setTitleVisible(state == Qt::Checked);
        axesParams[iAxis].title = state == Qt::Checked;
    }
}

void PlotterBarChart::onCheckLog(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal  : Qt::Vertical;
    Qt::Alignment   align  = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::AlignBottom : Qt::AlignLeft;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() )
    {
        if (state == Qt::Checked)
        {
            QValueAxis* axis = (QValueAxis*)(axes.first());
    
            QLogValueAxis* logAxis = new QLogValueAxis();
            logAxis->setVisible( axis->isVisible() );
            logAxis->setTitleVisible( axis->isTitleVisible() );
            logAxis->setTitleText( axis->titleText() );
            logAxis->setTitleFont( axis->titleFont() );
            logAxis->setLabelFormat( axis->labelFormat() );
            logAxis->setLabelsFont( axis->labelsFont() );
            
            mChartView->chart()->removeAxis(axis);
            mChartView->chart()->addAxis(logAxis, align);
            for (const auto& series : mChartView->chart()->series())
                series->attachAxis(logAxis);
            
            logAxis->setBase( ui->spinBoxLogBase->value() );
            logAxis->setMin( ui->doubleSpinBoxMin->value() );
            logAxis->setMax( ui->doubleSpinBoxMax->value() );
            logAxis->setMinorTickCount( ui->spinBoxMTicks->value() );
        }
        else
        {
            QLogValueAxis* logAxis = (QLogValueAxis*)(axes.first());
    
            QValueAxis* axis = new QValueAxis();
            axis->setVisible( logAxis->isVisible() );
            axis->setTitleVisible( logAxis->isTitleVisible() );
            axis->setTitleText( logAxis->titleText() );
            axis->setTitleFont( logAxis->titleFont() );
            axis->setLabelFormat( logAxis->labelFormat() );
            axis->setLabelsFont( logAxis->labelsFont() );
            
            mChartView->chart()->removeAxis(logAxis);
            mChartView->chart()->addAxis(axis, align);
            for (const auto& series : mChartView->chart()->series())
                series->attachAxis(axis);
            
            axis->setMin( ui->doubleSpinBoxMin->value() );
            axis->setMax( ui->doubleSpinBoxMax->value() );
            axis->setTickCount( ui->spinBoxTicks->value() );
            axis->setMinorTickCount( ui->spinBoxMTicks->value() );
        }
        ui->spinBoxTicks->setEnabled(  state != Qt::Checked);
        ui->spinBoxLogBase->setEnabled(state == Qt::Checked);
    }
}

void PlotterBarChart::onSpinLogBaseChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() && ui->checkBoxLog->isChecked())
    {
        QLogValueAxis* logAxis = (QLogValueAxis*)(axes.first());
        logAxis->setBase(i);
    }
}

void PlotterBarChart::onEditTitleChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setTitleText(text);
        axesParams[iAxis].titleText = text;
    }
}

void PlotterBarChart::onSpinTitleSizeChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        
        QFont font = axis->titleFont();
        font.setPointSize(i);
        axis->setTitleFont(font);
        axesParams[iAxis].titleSize = i;
    }
}

void PlotterBarChart::onEditFormatChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() )
    {
        if ( !ui->checkBoxLog->isChecked() ) {
            QValueAxis* axis = (QValueAxis*)(axes.first());
            axis->setLabelFormat(text);
        }
        else {
            QLogValueAxis* axis = (QLogValueAxis*)(axes.first());
            axis->setLabelFormat(text);
        }
    }
}

void PlotterBarChart::onComboValuePositionChanged(int index)
{
    if (mIgnoreEvents) return;
    for (auto series : mChartView->chart()->series())
    {
        QAbstractBarSeries* barSeries = (QAbstractBarSeries*)(series);
        if (index == 0)
            barSeries->setLabelsVisible(false);
        else {
            barSeries->setLabelsVisible(true);
            barSeries->setLabelsPosition( (QAbstractBarSeries::LabelsPosition)
                                          (ui->comboBoxValuePosition->currentData().toInt()) );
        }
    }
}

void PlotterBarChart::onComboValueAngleChanged(int /*index*/)
{
    if (mIgnoreEvents) return;
    for (auto series : mChartView->chart()->series())
    {
        QAbstractBarSeries* barSeries = (QAbstractBarSeries*)(series);
        barSeries->setLabelsAngle( ui->comboBoxValueAngle->currentData().toDouble() );
    }
}

void PlotterBarChart::onSpinLabelSizeChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        
        QFont font = axis->labelsFont();
        font.setPointSize(i);
        axis->setLabelsFont(font);
        axesParams[iAxis].labelSize = i;
    }
}

void PlotterBarChart::onSpinMinChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMin(d);
    }
}

void PlotterBarChart::onSpinMaxChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMax(d);
    }
}

void PlotterBarChart::onComboMinChanged(int /*index*/)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QBarCategoryAxis* axis = (QBarCategoryAxis*)(axes.first());
        axis->setMin( ui->comboBoxMin->currentText() );
    }
}

void PlotterBarChart::onComboMaxChanged(int /*index*/)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QBarCategoryAxis* axis = (QBarCategoryAxis*)(axes.first());
        axis->setMax( ui->comboBoxMax->currentText() );
    }
}

void PlotterBarChart::onSpinTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() )
    {
        if ( !ui->checkBoxLog->isChecked() ) {
            QValueAxis* axis = (QValueAxis*)(axes.first());
            axis->setTickCount(i);
        }
    }
}

void PlotterBarChart::onSpinMTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = (iAxis == 0 && mIsVert) || (iAxis == 1 && !mIsVert)
            ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() )
    {
        if ( !ui->checkBoxLog->isChecked() ) {
            QValueAxis* axis = (QValueAxis*)(axes.first());
            axis->setMinorTickCount(i);
        }
        else {
            QLogValueAxis* axis = (QLogValueAxis*)(axes.first());
            axis->setMinorTickCount(i);
            
            // Force update
            const int base = (int)axis->base();
            axis->setBase(base + 1);
            axis->setBase(base);
        }
    }
}

//
// Snapshot
void PlotterBarChart::onSnapshotClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save snapshot"), "", tr("Images (*.png)"));
    
    if ( !fileName.isEmpty() )
    {
        const QPixmap& pixmap = mChartView->grab();
        
        bool ok = pixmap.save(fileName, "PNG");
        if (!ok)
            QMessageBox::warning(this, "Chart snapshot", "Error saving snapshot file.");
    }
}
