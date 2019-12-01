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

#include "plotter_linechart.h"
#include "ui_plotter_linechart.h"

#include "benchmark_results.h"
#include "plot_parameters.h"

#include <QtCharts>
using namespace QtCharts;


PlotterLineChart::PlotterLineChart(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                                   const PlotParams &plotParams, const QString &filename,
                                   QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlotterLineChart)
{
    // UI
    ui->setupUi(this);
    
    if (plotParams.type == ChartLineType)
        this->setWindowTitle("Lines - "   + filename);
    else
        this->setWindowTitle("Splines - " + filename);
    
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
    connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterLineChart::onComboThemeChanged);
    
    // Legend
    connect(ui->checkBoxLegendVisible, &QCheckBox::stateChanged, this, &PlotterLineChart::onCheckLegendVisible);
    
    ui->comboBoxLegendAlign->addItem("Top",     Qt::AlignTop);
    ui->comboBoxLegendAlign->addItem("Bottom",  Qt::AlignBottom);
    ui->comboBoxLegendAlign->addItem("Left",    Qt::AlignLeft);
    ui->comboBoxLegendAlign->addItem("Right",   Qt::AlignRight);
    connect(ui->comboBoxLegendAlign, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterLineChart::onComboLegendAlignChanged);
    
    connect(ui->spinBoxLegendFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinLegendFontSizeChanged);
    
    // Axes
    ui->comboBoxAxis->addItem("X-Axis");
    ui->comboBoxAxis->addItem("Y-Axis");
    connect(ui->comboBoxAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterLineChart::onComboAxisChanged);
    
    connect(ui->checkBoxAxisVisible, &QCheckBox::stateChanged, this, &PlotterLineChart::onCheckAxisVisible);
    connect(ui->checkBoxTitle,       &QCheckBox::stateChanged, this, &PlotterLineChart::onCheckTitleVisible);
    connect(ui->checkBoxLog,         &QCheckBox::stateChanged, this, &PlotterLineChart::onCheckLog);
    connect(ui->spinBoxLogBase,      QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinLogBaseChanged);
    connect(ui->lineEditTitle,       &QLineEdit::textEdited, this, &PlotterLineChart::onEditTitleChanged);
    connect(ui->spinBoxTitleSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinTitleSizeChanged);
    connect(ui->lineEditFormat,      &QLineEdit::textEdited, this, &PlotterLineChart::onEditFormatChanged);
    connect(ui->spinBoxLabelSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinLabelSizeChanged);
    connect(ui->doubleSpinBoxMin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterLineChart::onSpinMinChanged);
    connect(ui->doubleSpinBoxMax,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterLineChart::onSpinMaxChanged);
    connect(ui->spinBoxTicks,        QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinTicksChanged);
    connect(ui->spinBoxMTicks,       QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterLineChart::onSpinMTicksChanged);
    
    // Snapshot
    connect(ui->pushButtonSnapshot, &QPushButton::clicked, this, &PlotterLineChart::onSnapshotClicked);
    
    //TODO: select points
    //See: https://doc.qt.io/qt-5/qtcharts-callout-example.html
    
    
    /*
     *  Chart
     */
    QChart *chart = new QChart();
    
    bool custDataAxis = true;
    QString custDataName;
    
    // 2D Lines
    // X: argumentA or templateB
    // Y: time/iter/bytes/items (not name dependent)
    // Line: one per benchmark % X-param
    QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                            bchIdxs, plotParams.xIdx, "X");
    for (const auto& bchSubset : bchSubsets)
    {
        // Ignore single point lines
        if (bchSubset.idxs.size() < 2) {
            qWarning() << "Not enough points to trace line for:" << bchSubset.name;
            continue;
        }
        
        // Chart type
        QLineSeries *series;
        if (plotParams.type == ChartLineType)   series = new QLineSeries();
        else                                    series = new QSplineSeries();
        
        const QString& subsetName = bchSubset.name;
//        qDebug() << "subsetName:" << subsetName;
//        qDebug() << "subsetIdxs:" << bchSubset.idxs;
        
        double xFallback = 0.;
        for (int idx : bchSubset.idxs)
        {
            QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                    idx, plotParams.xIdx);
            double xVal = BenchResults::getParamValue(xName, custDataName, custDataAxis, xFallback);
            
            // Add point
            series->append(xVal, getYPlotValue(bchResults.benchmarks[idx], plotParams.yType));
        }
        // Add series
        series->setName( subsetName.toHtmlEscaped() );
        chart->addSeries(series);
    }
    
    //
    // Axes
    if ( !chart->series().isEmpty() )
    {
        chart->createDefaultAxes();
        
        // X-axis
        QValueAxis* xAxis = (QValueAxis*)(chart->axes(Qt::Horizontal).first());
        if (plotParams.xType == PlotArgumentType)
            xAxis->setTitleText("Argument " + QString::number(plotParams.xIdx+1));
        else { // template
            if ( !custDataName.isEmpty() )
                xAxis->setTitleText(custDataName);
            else
                xAxis->setTitleText("Template " + QString::number(plotParams.xIdx+1));
        }
        xAxis->setTickCount(9);
        
        // Y-axis
        QValueAxis* yAxis = (QValueAxis*)(chart->axes(Qt::Vertical).first());
        yAxis->setTitleText( getYPlotName(plotParams.yType) );
        yAxis->applyNiceNumbers();
    }
    else
        chart->setTitle("No series with at least 2 points to display");
    
    
    /*
     *  Chart options
     */
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
    const auto& hAxes = chart->axes(Qt::Horizontal);
    if ( !hAxes.isEmpty() )
    {
        QValueAxis* xAxis = (QValueAxis*)(hAxes.first());
        
        axesParams[0].titleText = xAxis->titleText();
        axesParams[0].titleSize = xAxis->titleFont().pointSize();
        axesParams[0].labelFormat = xAxis->labelFormat();
        axesParams[0].labelSize   = xAxis->labelsFont().pointSize();
        axesParams[0].min = xAxis->min();
        axesParams[0].max = xAxis->max();
        axesParams[0].ticks  = xAxis->tickCount();
        axesParams[0].mticks = xAxis->minorTickCount();
        
        ui->lineEditTitle->setText( axesParams[0].titleText );
        ui->spinBoxTitleSize->setValue( axesParams[0].titleSize );
        ui->lineEditFormat->setText( axesParams[0].labelFormat );
        ui->spinBoxLabelSize->setValue( axesParams[0].labelSize );
        ui->doubleSpinBoxMin->setValue( axesParams[0].min );
        ui->doubleSpinBoxMax->setValue( axesParams[0].max );
        ui->spinBoxTicks->setValue( axesParams[0].ticks );
        ui->spinBoxMTicks->setValue( axesParams[0].mticks );
    }
    const auto& vAxes = chart->axes(Qt::Vertical);
    if ( !vAxes.isEmpty() )
    {
        QValueAxis* yAxis = (QValueAxis*)(vAxes.first());
        
        axesParams[1].titleText = yAxis->titleText();
        axesParams[1].titleSize = yAxis->titleFont().pointSize();
        axesParams[1].labelFormat = yAxis->labelFormat();
        axesParams[1].labelSize   = yAxis->labelsFont().pointSize();
        axesParams[1].min = yAxis->min();
        axesParams[1].max = yAxis->max();
        axesParams[1].ticks  = yAxis->tickCount();
        axesParams[1].mticks = yAxis->minorTickCount();
    }
    mIgnoreEvents = false;
    
    // Show
    ui->horizontalLayout->insertWidget(0, mChartView);
}

PlotterLineChart::~PlotterLineChart()
{
    delete ui;
}

//
// Theme
void PlotterLineChart::onComboThemeChanged(int index)
{
    QChart::ChartTheme theme = static_cast<QChart::ChartTheme>(
                ui->comboBoxTheme->itemData(index).toInt());
    mChartView->chart()->setTheme(theme);
}

//
// Legend
void PlotterLineChart::onCheckLegendVisible(int state)
{
    mChartView->chart()->legend()->setVisible(state == Qt::Checked);
}

void PlotterLineChart::onComboLegendAlignChanged(int index)
{
    Qt::Alignment align = static_cast<Qt::Alignment>(
                ui->comboBoxLegendAlign->itemData(index).toInt());
    mChartView->chart()->legend()->setAlignment(align);
}

void PlotterLineChart::onSpinLegendFontSizeChanged(int i)
{
    QFont font = mChartView->chart()->legend()->font();
    font.setPointSize(i);
    mChartView->chart()->legend()->setFont(font);
}

//
// Axes
void PlotterLineChart::onComboAxisChanged(int idx)
{
    // Update UI
    mIgnoreEvents = true;
    
    ui->checkBoxAxisVisible->setChecked( axesParams[idx].visible );
    ui->checkBoxTitle->setChecked( axesParams[idx].title );
    ui->checkBoxLog->setChecked( axesParams[idx].log );
    ui->spinBoxLogBase->setValue( axesParams[idx].logBase );
    ui->lineEditTitle->setText( axesParams[idx].titleText );
    ui->spinBoxTitleSize->setValue( axesParams[idx].titleSize );
    ui->lineEditFormat->setText( axesParams[idx].labelFormat );
    ui->spinBoxLabelSize->setValue( axesParams[idx].labelSize );
    ui->doubleSpinBoxMin->setValue( axesParams[idx].min );
    ui->doubleSpinBoxMax->setValue( axesParams[idx].max );
    ui->doubleSpinBoxMin->setSingleStep(idx == 1 ? 0.1 : 1.0);
    ui->doubleSpinBoxMax->setSingleStep(idx == 1 ? 0.1 : 1.0);
    ui->spinBoxTicks->setValue( axesParams[idx].ticks );
    ui->spinBoxMTicks->setValue( axesParams[idx].mticks );
    
    ui->spinBoxTicks->setEnabled( !axesParams[idx].log );
    ui->spinBoxLogBase->setEnabled( axesParams[idx].log );
    
    mIgnoreEvents = false;
}

void PlotterLineChart::onCheckAxisVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setVisible(state == Qt::Checked);
        axesParams[iAxis].visible = state == Qt::Checked;
    }
}

void PlotterLineChart::onCheckTitleVisible(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setTitleVisible(state == Qt::Checked);
        axesParams[iAxis].title = state == Qt::Checked;
    }
}

void PlotterLineChart::onCheckLog(int state)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal  : Qt::Vertical;
    Qt::Alignment   align  = iAxis == 0 ? Qt::AlignBottom : Qt::AlignLeft;
    
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
            
            logAxis->setBase( axesParams[iAxis].logBase );
            logAxis->setMin( axesParams[iAxis].min );
            logAxis->setMax( axesParams[iAxis].max );
            logAxis->setMinorTickCount( axesParams[iAxis].mticks );
        }
        else
        {
            QLogValueAxis*logAxis = (QLogValueAxis*)(axes.first());
    
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
            
            axis->setMin( axesParams[iAxis].min );
            axis->setMax( axesParams[iAxis].max );
            axis->setTickCount( axesParams[iAxis].ticks );
            axis->setMinorTickCount( axesParams[iAxis].mticks );
        }
        ui->spinBoxTicks->setEnabled(  state != Qt::Checked);
        ui->spinBoxLogBase->setEnabled(state == Qt::Checked);
        axesParams[iAxis].log = state == Qt::Checked;
    }
}

void PlotterLineChart::onSpinLogBaseChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() && ui->checkBoxLog->isChecked())
    {
        QLogValueAxis*logAxis = (QLogValueAxis*)(axes.first());
        logAxis->setBase(i);
        axesParams[iAxis].logBase = i;
    }
}

void PlotterLineChart::onEditTitleChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setTitleText(text);
        axesParams[iAxis].titleText = text;
    }
}

void PlotterLineChart::onSpinTitleSizeChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        
        QFont font = axis->titleFont();
        font.setPointSize(i);
        axis->setTitleFont(font);
        axesParams[iAxis].titleSize = i;
    }
}

void PlotterLineChart::onEditFormatChanged(const QString& text)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
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
        axesParams[iAxis].labelFormat = text;
    }
}

void PlotterLineChart::onSpinLabelSizeChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        
        QFont font = axis->labelsFont();
        font.setPointSize(i);
        axis->setLabelsFont(font);
        axesParams[iAxis].labelSize = i;
    }
}

void PlotterLineChart::onSpinMinChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMin(d);
        axesParams[iAxis].min = d;
    }
}

void PlotterLineChart::onSpinMaxChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMax(d);
        axesParams[iAxis].max = d;
    }
}

void PlotterLineChart::onSpinTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() )
    {
        if ( !ui->checkBoxLog->isChecked() ) {
            QValueAxis* axis = (QValueAxis*)(axes.first());
            axis->setTickCount(i);
            axesParams[iAxis].ticks = i;
        }
    }
}

void PlotterLineChart::onSpinMTicksChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
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
        axesParams[iAxis].mticks = i;
    }
}

//
// Snapshot
void PlotterLineChart::onSnapshotClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save snapshot"), "", tr("Images (*.png)"));
    
    if ( !fileName.isEmpty() )
    {
        QPixmap pixmap = mChartView->grab();
        
        bool ok = pixmap.save(fileName, "PNG");
        if (!ok)
            QMessageBox::warning(this, "Chart snapshot", "Error saving snapshot file.");
    }
}
