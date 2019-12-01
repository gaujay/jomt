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

#include "result_selector.h"
#include "ui_result_selector.h"

#include "result_parser.h"
#include "plot_parameters.h"

#include "plotter_linechart.h"
#include "plotter_barchart.h"
#include "plotter_boxchart.h"
#include "plotter_3dbars.h"
#include "plotter_3dsurface.h"

#include <QFileDialog>
#include <QMessageBox>


ResultSelector::ResultSelector(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ResultSelector)
{
    ui->setupUi(this);
    
    this->setWindowTitle( "Result selector" );
    
    ui->pushButtonAppend->setEnabled(false);
    ui->pushButtonOverwrite->setEnabled(false);
    ui->pushButtonReload->setEnabled(false);
    ui->pushButtonSelectAll->setEnabled(false);
    ui->pushButtonSelectNone->setEnabled(false);
    ui->pushButtonPlot->setEnabled(false);
    
    connectUI();
}

ResultSelector::ResultSelector(const BenchResults &bchResults_, const QString &fileName, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ResultSelector)
    , bchResults(bchResults_)
{
    ui->setupUi(this);
    
    if ( !fileName.isEmpty() ) {
        QFileInfo fileInfo(fileName);
        this->setWindowTitle( fileInfo.fileName() );
    }
    origFilename = fileName; // For reload
    updateResults();
    
    connectUI();
}

ResultSelector::~ResultSelector()
{
    delete ui;
}


void ResultSelector::connectUI()
{
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &ResultSelector::onItemChanged);
    
    connect(ui->comboBoxType, QOverload<int>::of(&QComboBox::activated), this, &ResultSelector::onComboTypeChanged);
    connect(ui->comboBoxX,    QOverload<int>::of(&QComboBox::activated), this, &ResultSelector::onComboXChanged);
    connect(ui->comboBoxZ,    QOverload<int>::of(&QComboBox::activated), this, &ResultSelector::onComboZChanged);
    
    connect(ui->pushButtonNew,       &QPushButton::clicked, this, &ResultSelector::onNewClicked);
    connect(ui->pushButtonAppend,    &QPushButton::clicked, this, &ResultSelector::onAppendClicked);
    connect(ui->pushButtonOverwrite, &QPushButton::clicked, this, &ResultSelector::onOverwriteClicked);
    connect(ui->pushButtonReload,    &QPushButton::clicked, this, &ResultSelector::onReloadClicked);
    
    connect(ui->pushButtonSelectAll,  &QPushButton::clicked, this, &ResultSelector::onSelectAllClicked);
    connect(ui->pushButtonSelectNone, &QPushButton::clicked, this, &ResultSelector::onSelectNoneClicked);
    
    connect(ui->pushButtonPlot, &QPushButton::clicked, this, &ResultSelector::onPlotClicked);
}

void ResultSelector::updateComboBoxY()
{
    PlotChartType chartType = (PlotChartType)ui->comboBoxType->currentData().toInt();
    
    // Classic
    if ((!bchResults.meta.hasAggregate || chartType == ChartBoxType)
            && ui->comboBoxY->findData(QVariant(RealTimeType)) == -1)
    {
        ui->comboBoxY->clear();
        
        ui->comboBoxY->addItem("Real time",     QVariant(RealTimeType));
        ui->comboBoxY->addItem("CPU time",      QVariant(CpuTimeType));
        ui->comboBoxY->addItem("Iterations",    QVariant(IterationsType));
        if (bchResults.meta.hasBytesSec)
            ui->comboBoxY->addItem("Bytes/s",   QVariant(BytesType));
        if (bchResults.meta.hasItemsSec)
            ui->comboBoxY->addItem("Items/s",   QVariant(ItemsType));
    }
    // Aggregate
    else if (ui->comboBoxY->findData(QVariant(RealTimeMinType)) == -1)
    {
        ui->comboBoxY->clear();
        
        ui->comboBoxY->addItem("Real min time",     QVariant(RealTimeMinType));
        ui->comboBoxY->addItem("Real mean time",    QVariant(RealTimeMeanType));
        ui->comboBoxY->addItem("Real median time",  QVariant(RealTimeMedianType));
        
        ui->comboBoxY->addItem("CPU min time",      QVariant(CpuTimeMinType));
        ui->comboBoxY->addItem("CPU mean time",     QVariant(CpuTimeMeanType));
        ui->comboBoxY->addItem("CPU median time",   QVariant(CpuTimeMedianType));
        
        ui->comboBoxY->addItem("Iterations",        QVariant(IterationsType));
        
        if (bchResults.meta.hasBytesSec) {
            ui->comboBoxY->addItem("Bytes/s min",       QVariant(BytesMinType));
            ui->comboBoxY->addItem("Bytes/s mean",      QVariant(BytesMeanType));
            ui->comboBoxY->addItem("Bytes/s median",    QVariant(BytesMedianType));
        }
        if (bchResults.meta.hasItemsSec) {
            ui->comboBoxY->addItem("Items/s min",       QVariant(ItemsMinType));
            ui->comboBoxY->addItem("Items/s mean",      QVariant(ItemsMeanType));
            ui->comboBoxY->addItem("Items/s median",    QVariant(ItemsMedianType));
        }
    }
}

static QTreeWidgetItem* buildTreeItem(const BenchData &bchData, QTreeWidgetItem *item = nullptr)
{
    QStringList labels = {bchData.base_name, bchData.templates.join(", "), bchData.arguments.join("/"),
                          QString::number(bchData.real_time_us), QString::number(bchData.cpu_time_us)};
    if ( !bchData.kbytes_sec.isEmpty() ) labels.append( QString::number(bchData.kbytes_sec_dflt) );
    if ( !bchData.kitems_sec.isEmpty() ) labels.append( QString::number(bchData.kitems_sec_dflt) );
    
    if (item == nullptr) {
        item = new QTreeWidgetItem(labels);
    }
    else {
        for (int iC=0; iC<labels.size(); ++iC)
            item->setText(iC, labels[iC]);
    }
    
    return item;
}

void ResultSelector::updateResults(bool clear)
{
    //
    // Tree widget
    if (clear)
        ui->treeWidget->clear();
    
    // Columns
    int iCol = 5;
    if (bchResults.meta.hasBytesSec) ++iCol;
    if (bchResults.meta.hasItemsSec) ++iCol;
    ui->treeWidget->setColumnCount(iCol);
    
    // Populate tree
    QList<QTreeWidgetItem *> items;
    
    QVector<BenchSubset> bchFamilies = bchResults.segmentFamilies();
    for (const auto &bchFamily : bchFamilies)
    {
        QTreeWidgetItem* topItem = new QTreeWidgetItem( QStringList(bchFamily.name) );
        
        // JOMT: family + container
        if ( !bchResults.benchmarks[bchFamily.idxs[0]].container.isEmpty() )
        {
            QVector<BenchSubset> bchContainers = bchResults.segmentContainers(bchFamily.idxs);
            for (const auto &bchContainer : bchContainers)
            {
                QTreeWidgetItem* midItem = new QTreeWidgetItem( QStringList(bchContainer.name) );
                
                for (int idx : bchContainer.idxs)
                {
                    QTreeWidgetItem *child = buildTreeItem( bchResults.benchmarks[idx] );
                    child->setCheckState(0, Qt::Checked);
                    child->setData(0, Qt::UserRole, idx);
                    midItem->addChild(child);
                }
                midItem->setCheckState(0, Qt::Checked);
                topItem->addChild(midItem);
            }
        }
        // Classic
        else
        {
            // Single
            if (bchFamily.idxs.size() == 1)
            {
                buildTreeItem(bchResults.benchmarks[bchFamily.idxs[0]], topItem);
                topItem->setData(0, Qt::UserRole, bchFamily.idxs[0]);
            }
            else // Family
            {
                for (int idx : bchFamily.idxs)
                {
                    QTreeWidgetItem *child = buildTreeItem( bchResults.benchmarks[idx] );
                    child->setCheckState(0, Qt::Checked);
                    child->setData(0, Qt::UserRole, idx);
                    topItem->addChild(child);
                }
            }
        }
        topItem->setCheckState(0, Qt::Checked);
        items.append(topItem);
    }
    ui->treeWidget->insertTopLevelItems(0, items);
    
    // Headers
    QStringList labels = {"Benchmark", "Templates", "Arguments"};
    if (!bchResults.meta.hasAggregate) {
        labels << "Real time (us)" << "CPU time (us)";
        if (bchResults.meta.hasBytesSec) labels << "Bytes/s (k)";
        if (bchResults.meta.hasItemsSec) labels << "Items/s (k)";
    }
    else {
        labels << "Real min time (us)" << "CPU min time (us)";
        if (bchResults.meta.hasBytesSec) labels << "Bytes/s min (k)";
        if (bchResults.meta.hasItemsSec) labels << "Items/s min (k)";
    }
    
    ui->treeWidget->setHeaderLabels(labels);
    
    ui->treeWidget->expandAll();
    for (int iC=0; iC<ui->treeWidget->columnCount(); ++iC)
        ui->treeWidget->resizeColumnToContents(iC);
    ui->treeWidget->setSortingEnabled(true);
    
    
    //
    // Chart options
    if (clear)
    {
        ui->comboBoxType->clear();
        ui->comboBoxX->clear();
        ui->comboBoxY->clear();
        ui->comboBoxZ->clear();
    }
    
    // Type
    if (bchResults.meta.maxArguments > 0 || bchResults.meta.maxTemplates > 0) {
        ui->comboBoxType->addItem("Lines",      ChartLineType);
        ui->comboBoxType->addItem("Splines",    ChartSplineType);
    }
    ui->comboBoxType->addItem("Bars",   ChartBarType);
    ui->comboBoxType->addItem("HBars",  ChartHBarType);
    if (bchResults.meta.hasAggregate)
        ui->comboBoxType->addItem("Boxes",  ChartBoxType);
    ui->comboBoxType->addItem("3D Bars",    Chart3DBarsType);
    if (bchResults.meta.maxArguments > 0 || bchResults.meta.maxTemplates > 0)
        ui->comboBoxType->addItem("3D Surface", Chart3DSurfaceType);
    
    
    // X-axis
    for (int i=0; i<bchResults.meta.maxArguments; ++i)
    {
        QList<QVariant> qvList;
        qvList.append(PlotArgumentType); qvList.append(i);
        ui->comboBoxX->addItem("Argument " + QString::number(i+1), qvList);
    }
    for (int i=0; i<bchResults.meta.maxTemplates; ++i)
    {
        QList<QVariant> qvList;
        qvList.append(PlotTemplateType); qvList.append(i);
        ui->comboBoxX->addItem("Template " + QString::number(i+1), qvList);
    }
    ui->comboBoxX->setEnabled(ui->comboBoxX->count() > 0);
    
    // Y-axis
    updateComboBoxY();
    
    // Z-axis
    if ( !ui->comboBoxX->isEnabled() ) {
        ui->comboBoxZ->setEnabled(false);
    }
    else {
        QList<QVariant> qvList;
        qvList.append(PlotEmptyType); qvList.append(0);
        ui->comboBoxZ->addItem("Auto", qvList);
        
        PlotChartType chartType = (PlotChartType)ui->comboBoxType->currentData().toInt();
        if (chartType == Chart3DBarsType || chartType == Chart3DSurfaceType) // Any 3D charts
            ui->comboBoxZ->setEnabled(true);
        else
            ui->comboBoxZ->setEnabled(false);

        for (int i=0; i<bchResults.meta.maxArguments; ++i)
        {
            QList<QVariant> qvList;
            qvList.append(PlotArgumentType); qvList.append(i);
            ui->comboBoxZ->addItem("Argument " + QString::number(i+1), qvList);
        }
        for (int i=0; i<bchResults.meta.maxTemplates; ++i)
        {
            QList<QVariant> qvList;
            qvList.append(PlotTemplateType); qvList.append(i);
            ui->comboBoxZ->addItem("Template " + QString::number(i+1), qvList);
        }
    }
}


static void updateItemParentsState(QTreeWidgetItem *item)
{
    auto parent = item->parent();
    if (parent == nullptr) return;
    
    bool allChecked = true, allUnchecked = true;
    for (int idx=0; (allChecked || allUnchecked) && idx<parent->childCount(); ++idx) {
        allChecked   &= parent->child(idx)->checkState(0) == Qt::Checked;
        allUnchecked &= parent->child(idx)->checkState(0) == Qt::Unchecked;
    }
    
    if      (allChecked)    parent->setCheckState(0, Qt::Checked);
    else if (allUnchecked)  parent->setCheckState(0, Qt::Unchecked);
    else                    parent->setCheckState(0, Qt::PartiallyChecked);
}

static void updateItemChildrenState(QTreeWidgetItem *item)
{
    if (item->childCount() <= 0) return;
    
    if (item->checkState(0) == Qt::Checked) {
        for (int idx=0; idx<item->childCount(); ++idx) {
            item->child(idx)->setCheckState(0, Qt::Checked);
        }
    }
    else if (item->checkState(0) == Qt::Unchecked)
    {
        for (int idx=0; idx<item->childCount(); ++idx) {
            item->child(idx)->setCheckState(0, Qt::Unchecked);
        }
    }
    // Nothing if 'PartiallyChecked'
}

void ResultSelector::onItemChanged(QTreeWidgetItem *item, int /*column*/)
{
    if (item == nullptr) return;
    
    updateItemChildrenState(item);
    updateItemParentsState(item);
    
    // Disable plot button if no items selected
    bool allUnchecked = true;
    for (int i=0; allUnchecked && i<ui->treeWidget->topLevelItemCount(); ++i)
        allUnchecked &= ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Unchecked;
    
    ui->pushButtonPlot->setEnabled(!allUnchecked);
}

static QVector<int> getSelectedBenchmarks(const QTreeWidget *tree)
{
    QVector<int> resIdxs;
    
    for (int i=0; i<tree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *topItem = tree->topLevelItem(i);
        if (topItem->childCount() <= 0)
        {
            if (topItem->checkState(0) == Qt::Checked)
                resIdxs.append( topItem->data(0, Qt::UserRole).toInt() );
        }
        else
        {
            for (int i=0; i<topItem->childCount(); ++i)
            {
                QTreeWidgetItem *midItem = topItem->child(i);
                if (midItem->childCount() <= 0)
                {
                    if (midItem->checkState(0) == Qt::Checked)
                        resIdxs.append( midItem->data(0, Qt::UserRole).toInt() );
                }
                else
                {
                    for (int j=0; j<midItem->childCount(); ++j)
                    {
                        QTreeWidgetItem *lowItem = midItem->child(j);
                        if (lowItem->checkState(0) == Qt::Checked)
                            resIdxs.append( lowItem->data(0, Qt::UserRole).toInt() );
                    }
                }
            }
        }
    }
    
    return resIdxs;
}


void ResultSelector::onComboTypeChanged(int /*index*/)
{
    PlotChartType chartType = (PlotChartType)ui->comboBoxType->currentData().toInt();
    
    if (chartType == Chart3DBarsType || chartType == Chart3DSurfaceType) // Any 3D charts
        ui->comboBoxZ->setEnabled( ui->comboBoxX->isEnabled() );
    else
        ui->comboBoxZ->setEnabled(false);
    
    if (bchResults.meta.hasAggregate)
        updateComboBoxY();
}

void ResultSelector::onComboXChanged(int /*index*/)
{
    PlotParamType xType = (PlotParamType)ui->comboBoxX->currentData().toList()[0].toInt();
    int xIdx  = ui->comboBoxX->currentData().toList()[1].toInt();
    
    PlotParamType zType = (PlotParamType)ui->comboBoxZ->currentData().toList()[0].toInt();
    int zIdx  = ui->comboBoxZ->currentData().toList()[1].toInt();
    
    // Change comboZ to avoid having same value
    if (xType == zType && xIdx == zIdx)
    {
        if (ui->comboBoxZ->currentIndex() == 0)
            ui->comboBoxZ->setCurrentIndex(1);
        else
            ui->comboBoxZ->setCurrentIndex(0);
    }
}

void ResultSelector::onComboZChanged(int /*index*/)
{
    PlotParamType xType = (PlotParamType)ui->comboBoxX->currentData().toList()[0].toInt();
    int xIdx  = ui->comboBoxX->currentData().toList()[1].toInt();
    
    PlotParamType zType = (PlotParamType)ui->comboBoxZ->currentData().toList()[0].toInt();
    int zIdx  = ui->comboBoxZ->currentData().toList()[1].toInt();
    
    // Change comboX to avoid having same value
    if (zType == xType && zIdx == xIdx)
    {
        if (ui->comboBoxX->currentIndex() == 0)
            ui->comboBoxX->setCurrentIndex(1);
        else
            ui->comboBoxX->setCurrentIndex(0);
    }
}

// File
void ResultSelector::onNewClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open benchmark results"), "", tr("Benchmark results (*.json)"));
    
    if ( !fileName.isEmpty() && QFile::exists(fileName) )
    {
        BenchResults newResults = ResultParser::parseJsonFile(fileName);
        if (newResults.benchmarks.size() <= 0) {
            QMessageBox::warning(this, "Open benchmark results",
                                 "Error parsing file.\nSee console output for details.");
            return;
        }
        // Replace & upate
        this->bchResults = newResults;
        updateResults(true);
        
        // Update UI
        ui->pushButtonAppend->setEnabled(true);
        ui->pushButtonOverwrite->setEnabled(true);
        ui->pushButtonReload->setEnabled(true);
        ui->pushButtonSelectAll->setEnabled(true);
        ui->pushButtonSelectNone->setEnabled(true);
        ui->pushButtonPlot->setEnabled(true);
        
        // Save for reload
        origFilename = fileName;
        addFilenames.clear();
        
        // Window title
        QFileInfo fileInfo(fileName);
        this->setWindowTitle( fileInfo.fileName() );
    }
}

void ResultSelector::onAppendClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Append benchmark results"), "", tr("Benchmark results (*.json)"));
    
    if ( !fileName.isEmpty() && QFile::exists(fileName) )
    {
        BenchResults newResults = ResultParser::parseJsonFile(fileName);
        if (newResults.benchmarks.size() <= 0) {
            QMessageBox::warning(this, "Open benchmark results",
                                 "Error parsing file.\nSee console output for details.");
            return;
        }
        // Append & upate
        this->bchResults.appendResults(newResults);
        updateResults(true);
        
        // Save for reload
        addFilenames.append( {fileName, true} );
        
        // Window title
        if ( !this->windowTitle().endsWith(" + ...") )
            this->setWindowTitle( this->windowTitle() + " + ..." );
    }
}

void ResultSelector::onOverwriteClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Overwrite benchmark results"), "", tr("Benchmark results (*.json)"));
    
    if ( !fileName.isEmpty() && QFile::exists(fileName) )
    {
        BenchResults newResults = ResultParser::parseJsonFile(fileName);
        if (newResults.benchmarks.size() <= 0) {
            QMessageBox::warning(this, "Open benchmark results",
                                 "Error parsing file.\nSee console output for details.");
            return;
        }
        // Overwrite & upate
        this->bchResults.overwriteResults(newResults);
        updateResults(true);
        
        // Save for reload
        addFilenames.append( {fileName, false} );
        
        // Window title
        if ( !this->windowTitle().endsWith(" + ...") )
            this->setWindowTitle( this->windowTitle() + " + ..." );
    }
}

void ResultSelector::onReloadClicked()
{
    // Check original
    if ( origFilename.isEmpty() ) {
        QMessageBox::warning(this, "Reload benchmark results", "No file to reload");
        return;
    }
    if ( !QFile::exists(origFilename) ) {
        QMessageBox::warning(this, "Reload benchmark results",
                             "File to reload does no exist:" + origFilename);
        return;
    }
    // Load original
    BenchResults newResults = ResultParser::parseJsonFile(origFilename);
    if (newResults.benchmarks.size() <= 0) {
        QMessageBox::warning(this, "Reload benchmark results", "Error parsing file:" + origFilename
                             + "\nSee console output for details.");
        return;
    }
    
    // Load additionnals
    for (const auto &addFile : addFilenames)
    {
        BenchResults addResults = ResultParser::parseJsonFile(addFile.filename);
        if (addResults.benchmarks.size() <= 0) {
            QMessageBox::warning(this, "Reload benchmark results", "Error parsing file:"
                                 + addFile.filename + "\nSee console output for details.");
            return;
        }
        // Append / Overwrite
        if (addFile.isAppend)
            newResults.appendResults(addResults);
        else
            newResults.overwriteResults(addResults);
    }
    
    // Replace & update
    this->bchResults = newResults;
    updateResults(true);
}

// Selection
void ResultSelector::onSelectAllClicked()
{
    for (int i=0; i<ui->treeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *topItem = ui->treeWidget->topLevelItem(i);
        topItem->setCheckState(0, Qt::Checked);
    }
}

void ResultSelector::onSelectNoneClicked()
{
    for (int i=0; i<ui->treeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *topItem = ui->treeWidget->topLevelItem(i);
        topItem->setCheckState(0, Qt::Unchecked);
    }
}

// Plot
void ResultSelector::onPlotClicked()
{
    // Params
    PlotParams plotParams;
    
    plotParams.type = (PlotChartType)ui->comboBoxType->currentData().toInt();
    
    // Axes
    if (ui->comboBoxX->currentIndex() >= 0) {
        plotParams.xType = (PlotParamType)ui->comboBoxX->currentData().toList()[0].toInt();
        plotParams.xIdx  = ui->comboBoxX->currentData().toList()[1].toInt();
    }
    else {
        plotParams.xType = PlotEmptyType;
        plotParams.xIdx  = -1;
    }
    
    plotParams.yType = (PlotValueType)ui->comboBoxY->currentData().toInt();
    
    if ( ui->comboBoxZ->isEnabled() && ui->comboBoxZ->currentIndex() >= 0) {
        plotParams.zType = (PlotParamType)ui->comboBoxZ->currentData().toList()[0].toInt();
        plotParams.zIdx  = ui->comboBoxZ->currentData().toList()[1].toInt();
    }
    else {
        plotParams.zType = PlotEmptyType;
        plotParams.zIdx  = -1;
    }
    
    // Selected items
    const auto &bchIdxs = getSelectedBenchmarks(ui->treeWidget);

    //
    // Call plotter
    switch (plotParams.type)
    {
        case ChartLineType:
        case ChartSplineType:
        {
            PlotterLineChart *plotLines = new PlotterLineChart(bchResults, bchIdxs,
                                                               plotParams, this->windowTitle());
            plotLines->show();
            break;
        }
        case ChartBarType:
        case ChartHBarType:
        {
            PlotterBarChart *plotBars = new PlotterBarChart(bchResults, bchIdxs,
                                                             plotParams, this->windowTitle());
            plotBars->show();
            break;
        }
        case ChartBoxType:
        {
            PlotterBoxChart *plotBoxes = new PlotterBoxChart(bchResults, bchIdxs,
                                                             plotParams, this->windowTitle());
            plotBoxes->show();
            break;
        }
        case Chart3DBarsType:
        {
            Plotter3DBars *plot3DBars = new Plotter3DBars(bchResults, bchIdxs,
                                                          plotParams, this->windowTitle());
            plot3DBars->show();
            break;
        }
        case Chart3DSurfaceType:
        {
            Plotter3DSurface *plot3DSfce = new Plotter3DSurface(bchResults, bchIdxs,
                                                                plotParams, this->windowTitle());
            plot3DSfce->show();
            break;
        }
    }
}
