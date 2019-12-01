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

#ifndef PLOTTER_BARCHART_H
#define PLOTTER_BARCHART_H

#include <QWidget>

namespace Ui {
class PlotterBarChart;
}
namespace QtCharts {
class QChartView;
}
class BenchResults;
class PlotParams;


class PlotterBarChart : public QWidget
{
    Q_OBJECT
    
public:
    explicit PlotterBarChart(const BenchResults &bchResults, const QVector<int> &bchIdxs,
                             const PlotParams &plotParams, const QString &filename,
                             QWidget *parent = nullptr);
    ~PlotterBarChart();

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
    void onComboValuePositionChanged(int index);
    void onComboValueAngleChanged(int index);
    void onSpinLabelSizeChanged(int i);
    void onSpinMinChanged(double d);
    void onSpinMaxChanged(double d);
    void onComboMinChanged(int index);
    void onComboMaxChanged(int index);
    void onSpinTicksChanged(int i);
    void onSpinMTicksChanged(int i);
    
    void onSnapshotClicked();
    
    
private:
    struct AxisParam {
        AxisParam() : visible(true), title(true) {}
        
        bool visible, title;
        QString titleText;
        int titleSize, labelSize;
    };
    
    Ui::PlotterBarChart  *ui;
    QtCharts::QChartView *mChartView;
    AxisParam axesParams[2];
    bool mIsVert;
    bool mIgnoreEvents = false;
};


#endif // PLOTTER_BARCHART_H
