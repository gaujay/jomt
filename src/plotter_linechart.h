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

#ifndef PLOTTER_LINECHART_H
#define PLOTTER_LINECHART_H

#include <QWidget>

namespace Ui {
class PlotterLineChart;
}
namespace QtCharts {
class QChartView;
}
class BenchResults;
class PlotParams;


class PlotterLineChart : public QWidget
{
    Q_OBJECT
    
public:
    explicit PlotterLineChart(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                              const PlotParams &plotParams, const QString &filename,
                              QWidget *parent = nullptr);
    ~PlotterLineChart();

public slots:
    void onComboThemeChanged(int index);
    
    void onCheckLegendVisible(int state);
    void onComboLegendAlignChanged(int index);
    void onSpinLegendFontSizeChanged(int i);
    
    void onComboAxisChanged(int index);
    void onCheckAxisVisible(int state);
    void onCheckTitleVisible(int state);
    void onCheckLog(int state);
    void onSpinLogBaseChanged(int i);
    void onEditTitleChanged(const QString& text);
    void onSpinTitleSizeChanged(int i);
    void onEditFormatChanged(const QString& text);
    void onSpinLabelSizeChanged(int i);
    void onSpinMinChanged(double d);
    void onSpinMaxChanged(double d);
    void onSpinTicksChanged(int i);
    void onSpinMTicksChanged(int i);
    
    void onSnapshotClicked();
    
    
private:
    struct ValAxisParam {
        ValAxisParam() : visible(true), title(true), log(false), logBase(10) {}
        
        bool visible, title, log;
        QString titleText, labelFormat;
        int titleSize, labelSize;
        double min, max;
        int ticks, mticks, logBase;
    };
    
    Ui::PlotterLineChart *ui;
    QtCharts::QChartView *mChartView;
    ValAxisParam axesParams[2];
    bool mIgnoreEvents = false;
};


#endif // PLOTTER_LINECHART_H
