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

#include "plotter_boxchart.h"
#include "ui_plotter_boxchart.h"

#include "benchmark_results.h"
#include "plot_parameters.h"

#include <QtCharts>
using namespace QtCharts;


PlotterBoxChart::PlotterBoxChart(BenchResults &bchResults, const QVector<int> &bchIdxs,
                                 const PlotParams &plotParams, const QString &filename,
                                 QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlotterBoxChart)
{
    // UI
    ui->setupUi(this);
    this->setWindowTitle("Boxes - " + filename);
    
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
    connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBoxChart::onComboThemeChanged);
    
    // Legend
    connect(ui->checkBoxLegendVisible, &QCheckBox::stateChanged, this, &PlotterBoxChart::onCheckLegendVisible);
    
    ui->comboBoxLegendAlign->addItem("Top",     Qt::AlignTop);
    ui->comboBoxLegendAlign->addItem("Bottom",  Qt::AlignBottom);
    ui->comboBoxLegendAlign->addItem("Left",    Qt::AlignLeft);
    ui->comboBoxLegendAlign->addItem("Right",   Qt::AlignRight);
    connect(ui->comboBoxLegendAlign, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBoxChart::onComboLegendAlignChanged);
    
    connect(ui->spinBoxLegendFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinLegendFontSizeChanged);
    
    // Axes
    ui->comboBoxAxis->addItem("X-Axis");
    ui->comboBoxAxis->addItem("Y-Axis");
    connect(ui->comboBoxAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBoxChart::onComboAxisChanged);
    
    connect(ui->checkBoxAxisVisible, &QCheckBox::stateChanged, this, &PlotterBoxChart::onCheckAxisVisible);
    connect(ui->checkBoxTitle,       &QCheckBox::stateChanged, this, &PlotterBoxChart::onCheckTitleVisible);
    connect(ui->checkBoxLog,         &QCheckBox::stateChanged, this, &PlotterBoxChart::onCheckLog);
    connect(ui->spinBoxLogBase,      QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinLogBaseChanged);
    connect(ui->lineEditTitle,       &QLineEdit::textEdited, this, &PlotterBoxChart::onEditTitleChanged);
    connect(ui->spinBoxTitleSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinTitleSizeChanged);
    connect(ui->lineEditFormat,      &QLineEdit::textEdited, this, &PlotterBoxChart::onEditFormatChanged);
    connect(ui->spinBoxLabelSize,    QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinLabelSizeChanged);
    connect(ui->doubleSpinBoxMin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterBoxChart::onSpinMinChanged);
    connect(ui->doubleSpinBoxMax,    QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PlotterBoxChart::onSpinMaxChanged);
    connect(ui->comboBoxMin,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBoxChart::onComboMinChanged);
    connect(ui->comboBoxMax,         QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlotterBoxChart::onComboMaxChanged);
    connect(ui->spinBoxTicks,        QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinTicksChanged);
    connect(ui->spinBoxMTicks,       QOverload<int>::of(&QSpinBox::valueChanged), this, &PlotterBoxChart::onSpinMTicksChanged);
    
    // Snapshot
    connect(ui->pushButtonSnapshot, &QPushButton::clicked, this, &PlotterBoxChart::onSnapshotClicked);
    
    
    /*
     *  Chart
     */
    QChart *chart = new QChart();
    
    // 2D Boxes and whiskers
    // X: argumentA or templateB
    // Y: time/iter/bytes/items (not name dependent)
    // Box: one per benchmark % X-param
    QVector<BenchSubset> bchSubsets = bchResults.groupParam(plotParams.xType == PlotArgumentType,
                                                            bchIdxs, plotParams.xIdx, "X");
    for (const auto& bchSubset : bchSubsets)
    {
        // Series = benchmark % X-param
        QBoxPlotSeries *series = new QBoxPlotSeries();
        
        const QString & subsetName = bchSubset.name;
//        qDebug() << "subsetName:" << subsetName;
//        qDebug() << "subsetIdxs:" << bchSubset.idxs;
        
        for (int idx : bchSubset.idxs)
        {
            QString xName = bchResults.getParamName(plotParams.xType == PlotArgumentType,
                                                    idx, plotParams.xIdx);
            BenchYStats yStats = getYPlotStats(bchResults.benchmarks[idx], plotParams.yType);
            
            // BoxSet
            QBoxSet *box = new QBoxSet( xName.toHtmlEscaped() );
            box->setValue(QBoxSet::LowerExtreme,  yStats.min);
            box->setValue(QBoxSet::UpperExtreme,  yStats.max);
            box->setValue(QBoxSet::Median,        yStats.median);
            box->setValue(QBoxSet::LowerQuartile, yStats.lowQuart);
            box->setValue(QBoxSet::UpperQuartile, yStats.uppQuart);
            
            series->append(box);
        }
        // Add series
        series->setName( subsetName.toHtmlEscaped() );
        chart->addSeries(series);
    }
    
    
    /*
     *  Chart options
     */
    if ( !chart->series().isEmpty() )
    {
        chart->legend()->setVisible(true);
        chart->createDefaultAxes();
        
        // X-axis
        QBarCategoryAxis* xAxis = (QBarCategoryAxis*)(chart->axes(Qt::Horizontal).first());
        if (plotParams.xType == PlotArgumentType)
            xAxis->setTitleText("Argument " + QString::number(plotParams.xIdx+1));
        else if (plotParams.xType == PlotTemplateType)
            xAxis->setTitleText("Template " + QString::number(plotParams.xIdx+1));
        if (plotParams.xType != PlotEmptyType)
            xAxis->setTitleVisible(true);
        
        // Y-axis
        QValueAxis* yAxis = (QValueAxis*)(chart->axes(Qt::Vertical).first());
        yAxis->setTitleText( getYPlotName(plotParams.yType) );
        yAxis->applyNiceNumbers();
    }
    else
        chart->setTitle("No compatible series to display");
    
    
    // View
    mChartView = new QChartView(chart);
    mChartView->setRenderHint(QPainter::Antialiasing);
    
    // Options
    mChartView->chart()->setTheme(QChart::ChartThemeLight);
    
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->legend()->setShowToolTips(true);
    ui->spinBoxLegendFontSize->setValue( chart->legend()->font().pointSize() );
    
    // Axes options
    mIgnoreEvents = true;
    const auto& xAxes = chart->axes(Qt::Horizontal);
    if ( !xAxes.isEmpty() )
    {
        QBarCategoryAxis* xAxis = (QBarCategoryAxis*)(xAxes.first());
        
        axesParams[0].titleText = xAxis->titleText();
        axesParams[0].titleSize = xAxis->titleFont().pointSize();
        axesParams[0].labelSize = xAxis->labelsFont().pointSize();
        
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
    const auto& yAxes = chart->axes(Qt::Vertical);
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

PlotterBoxChart::~PlotterBoxChart()
{
    delete ui;
}

//
// Theme
void PlotterBoxChart::onComboThemeChanged(int index)
{
    QChart::ChartTheme theme = static_cast<QChart::ChartTheme>(
                ui->comboBoxTheme->itemData(index).toInt());
    mChartView->chart()->setTheme(theme);
}

//
// Legend
void PlotterBoxChart::onCheckLegendVisible(int state)
{
    mChartView->chart()->legend()->setVisible(state == Qt::Checked);
}

void PlotterBoxChart::onComboLegendAlignChanged(int index)
{
    Qt::Alignment align = static_cast<Qt::Alignment>(
                ui->comboBoxLegendAlign->itemData(index).toInt());
    mChartView->chart()->legend()->setAlignment(align);
}

void PlotterBoxChart::onSpinLegendFontSizeChanged(int i)
{
    QFont font = mChartView->chart()->legend()->font();
    font.setPointSize(i);
    mChartView->chart()->legend()->setFont(font);
}

//
// Axes
void PlotterBoxChart::onComboAxisChanged(int idx)
{
    // Update UI
    mIgnoreEvents = true;
    
    ui->checkBoxAxisVisible->setChecked( axesParams[idx].visible );
    ui->checkBoxTitle->setChecked( axesParams[idx].title );
    ui->checkBoxLog->setEnabled( idx == 1 );
    ui->spinBoxLogBase->setEnabled( ui->checkBoxLog->isEnabled() && ui->checkBoxLog->isChecked() );
    ui->lineEditTitle->setText( axesParams[idx].titleText );
    ui->spinBoxTitleSize->setValue( axesParams[idx].titleSize );
    ui->lineEditFormat->setEnabled( idx == 1 );
    ui->spinBoxLabelSize->setValue( axesParams[idx].labelSize );
    ui->comboBoxMin->setVisible( idx == 0 );
    ui->comboBoxMax->setVisible( idx == 0 );
    ui->doubleSpinBoxMin->setVisible( idx == 1 );
    ui->doubleSpinBoxMax->setVisible( idx == 1 );
    ui->spinBoxTicks->setEnabled(   idx == 1 && !ui->checkBoxLog->isChecked() );
    ui->spinBoxMTicks->setEnabled(  idx == 1 );
    
    mIgnoreEvents = false;
}

void PlotterBoxChart::onCheckAxisVisible(int state)
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

void PlotterBoxChart::onCheckTitleVisible(int state)
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

void PlotterBoxChart::onCheckLog(int state)
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

void PlotterBoxChart::onSpinLogBaseChanged(int i)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() && ui->checkBoxLog->isChecked())
    {
        QLogValueAxis* logAxis = (QLogValueAxis*)(axes.first());
        logAxis->setBase(i);
    }
}

void PlotterBoxChart::onEditTitleChanged(const QString& text)
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

void PlotterBoxChart::onSpinTitleSizeChanged(int i)
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

void PlotterBoxChart::onEditFormatChanged(const QString& text)
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
    }
}

void PlotterBoxChart::onSpinLabelSizeChanged(int i)
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

void PlotterBoxChart::onSpinMinChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMin(d);
    }
}

void PlotterBoxChart::onSpinMaxChanged(double d)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QAbstractAxis* axis = axes.first();
        axis->setMax(d);
    }
}

void PlotterBoxChart::onComboMinChanged(int /*index*/)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QBarCategoryAxis* axis = (QBarCategoryAxis*)(axes.first());
        axis->setMin( ui->comboBoxMin->currentText() );
    }
}

void PlotterBoxChart::onComboMaxChanged(int /*index*/)
{
    if (mIgnoreEvents) return;
    int iAxis = ui->comboBoxAxis->currentIndex();
    Qt::Orientation orient = iAxis == 0 ? Qt::Horizontal : Qt::Vertical;
    
    const auto& axes = mChartView->chart()->axes(orient);
    if ( !axes.isEmpty() ) {
        QBarCategoryAxis* axis = (QBarCategoryAxis*)(axes.first());
        axis->setMax( ui->comboBoxMax->currentText() );
    }
}

void PlotterBoxChart::onSpinTicksChanged(int i)
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
        }
    }
}

void PlotterBoxChart::onSpinMTicksChanged(int i)
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
    }
}

//
// Snapshot
void PlotterBoxChart::onSnapshotClicked()
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
