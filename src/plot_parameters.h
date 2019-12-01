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

#ifndef PLOT_PARAMETERS_H
#define PLOT_PARAMETERS_H

#include "benchmark_results.h"


// Chart types
enum PlotChartType {
    ChartLineType,
    ChartSplineType,
    ChartBarType,
    ChartHBarType,
    ChartBoxType,
    Chart3DBarsType,
    Chart3DSurfaceType
};

// Parameter types
enum PlotParamType {
    PlotEmptyType,
    PlotArgumentType,
    PlotTemplateType
};

// Y-value types
enum PlotValueType {
    CpuTimeType,  CpuTimeMinType,  CpuTimeMeanType,  CpuTimeMedianType,
    RealTimeType, RealTimeMinType, RealTimeMeanType, RealTimeMedianType,
    IterationsType,
    BytesType, BytesMinType, BytesMeanType, BytesMedianType,
    ItemsType, ItemsMinType, ItemsMeanType, ItemsMedianType
};

// Y-value stats
struct BenchYStats {
    double min, max;
    double median;
    double lowQuart, uppQuart;
};


//
// Plot parameters
struct PlotParams {
    PlotChartType type;
    PlotParamType xType;
    int xIdx;
    PlotValueType yType;
    PlotParamType zType;
    int zIdx;
};


/*
 * Static functions
 */
// Get Y-value according to type
static double getYPlotValue(const BenchData &bchData, PlotValueType yType)
{
    switch (yType)
    {
        // CPU time
        case CpuTimeType: {
            return bchData.cpu_time_us;
        }
        case CpuTimeMinType: {
            return bchData.min_cpu;
        }
        case CpuTimeMeanType: {
            return bchData.mean_cpu;
        }
        case CpuTimeMedianType: {
            return bchData.median_cpu;
        }
        
        // Real time
        case RealTimeType: {
            return bchData.real_time_us;
        }
        case RealTimeMinType: {
            return bchData.min_real;
        }
        case RealTimeMeanType: {
            return bchData.mean_real;
        }
        case RealTimeMedianType: {
            return bchData.median_real;
        }
        
        // Iterations
        case IterationsType: {
            return bchData.iterations;
        }
        
        // Bytes/s
        case BytesType: {
            return bchData.kbytes_sec_dflt;
        }
        case BytesMinType: {
            return bchData.min_kbytes;
        }
        case BytesMeanType: {
            return bchData.mean_kbytes;
        }
        case BytesMedianType: {
            return bchData.median_kbytes;
        }
        
        // Items/s
        case ItemsType: {
            return bchData.kitems_sec_dflt;
        }
        case ItemsMinType: {
            return bchData.min_kitems;
        }
        case ItemsMeanType: {
            return bchData.mean_kitems;
        }
        case ItemsMedianType: {
            return bchData.median_kitems;
        }
    }
    
    return -1;
}

// Get Y-name according to type
static QString getYPlotName(PlotValueType yType)
{
    switch (yType)
    {
        // CPU time
        case CpuTimeType: {
            return "CPU time (us)";
        }
        case CpuTimeMinType: {
            return "CPU min time (us)";
        }
        case CpuTimeMeanType: {
            return "CPU mean time (us)";
        }
        case CpuTimeMedianType: {
            return "CPU median time (us)";
        }
        
        // Real time
        case RealTimeType: {
            return "Real time (us)";
        }
        case RealTimeMinType: {
            return "Real min time (us)";
        }
        case RealTimeMeanType: {
            return "Real mean time (us)";
        }
        case RealTimeMedianType: {
            return "Real median time (us)";
        }
        
        // Iterations
        case IterationsType: {
            return "Iterations";
        }
        
        // Bytes/s
        case BytesType: {
            return "Bytes/s (k)";
        }
        case BytesMinType: {
            return "Bytes/s min (k)";
        }
        case BytesMeanType: {
            return "Bytes/s mean (k)";
        }
        case BytesMedianType: {
            return "Bytes/s median (k)";
        }
        
        // Items/s
        case ItemsType: {
            return "Items/s (k)";
        }
        case ItemsMinType: {
            return "Items/s min (k)";
        }
        case ItemsMeanType: {
            return "Items/s mean (k)";
        }
        case ItemsMedianType: {
            return "Items/s median (k)";
        }
    }
    
    return "Unknown";
}

// Convert time value to micro-seconds
static double normalizeTimeUs(const BenchData &bchData, double value)
{
    double timeFactor = 1.;
    if      (bchData.time_unit == "ns") timeFactor = 0.001;
    else if (bchData.time_unit == "ms") timeFactor = 1000.;
    return value * timeFactor;
}

// Find median in vector subpart
static double findMedian(QVector<double> sorted, int begin, int end)
{
    int count = end - begin;
    if (count <= 0) return 0.;
    
    if (count % 2) {
        return sorted.at(count / 2 + begin);
    } else {
        qreal right = sorted.at(count / 2 + begin);
        qreal left = sorted.at(count / 2 - 1 + begin);
        return (right + left) / 2.0;
    }
}

// Get Y-value statistics (for Box chart)
static BenchYStats getYPlotStats(BenchData &bchData, PlotValueType yType)
{
    BenchYStats statRes;
    
    // No statistics
    if (!bchData.hasAggregate) {
        statRes.min      = 0.;
        statRes.max      = 0.;
        statRes.median   = 0.;
        statRes.lowQuart = 0.;
        statRes.uppQuart = 0.;
        
        return statRes;
    }
    
    switch (yType)
    {
        // CPU time
        case CpuTimeType:
        case CpuTimeMinType: case CpuTimeMeanType: case CpuTimeMedianType:
        {
            statRes.min    = bchData.min_cpu;
            statRes.max    = bchData.max_cpu;
            statRes.median = bchData.median_cpu;
            
            qSort(bchData.cpu_time);
            int count = bchData.cpu_time.count();
            statRes.lowQuart = normalizeTimeUs(bchData, findMedian(bchData.cpu_time, 0, count/2));
            statRes.uppQuart = normalizeTimeUs(bchData, findMedian(bchData.cpu_time, count/2 + (count%2), count));
            
            break;
        }
        // Real time
        case RealTimeType:
        case RealTimeMinType: case RealTimeMeanType: case RealTimeMedianType:
        {
            statRes.min    = bchData.min_real;
            statRes.max    = bchData.max_real;
            statRes.median = bchData.median_real;
            
            qSort(bchData.real_time);
            int count = bchData.real_time.count();
            statRes.lowQuart = normalizeTimeUs(bchData, findMedian(bchData.real_time, 0, count/2));
            statRes.uppQuart = normalizeTimeUs(bchData, findMedian(bchData.real_time, count/2 + (count%2), count));
            
            break;
        }
        // Bytes/s
        case BytesType:
        case BytesMinType: case BytesMeanType: case BytesMedianType:
        {
            statRes.min    = bchData.min_kbytes;
            statRes.max    = bchData.max_kbytes;
            statRes.median = bchData.median_kbytes;
            
            qSort(bchData.kbytes_sec);
            int count = bchData.kbytes_sec.count();
            statRes.lowQuart = findMedian(bchData.kbytes_sec, 0, count/2);
            statRes.uppQuart = findMedian(bchData.kbytes_sec, count/2 + (count%2), count);
            
            break;
        }
        // Items/s
        case ItemsType:
        case ItemsMinType: case ItemsMeanType: case ItemsMedianType:
        {
            statRes.min    = bchData.min_kitems;
            statRes.max    = bchData.max_kitems;
            statRes.median = bchData.median_kitems;
            
            qSort(bchData.kitems_sec);
            int count = bchData.kitems_sec.count();
            statRes.lowQuart = findMedian(bchData.kitems_sec, 0, count/2);
            statRes.uppQuart = findMedian(bchData.kitems_sec, count/2 + (count%2), count);
            
            break;
        }
        default:    //Error
        {
            statRes.min      = 0.;
            statRes.max      = 0.;
            statRes.median   = 0.;
            statRes.lowQuart = 0.;
            statRes.uppQuart = 0.;
            
            break;
        }
    }
    
    return statRes;
}

// Compare first common elements between string lists
static bool commonPartEqual(const QStringList &listA, const QStringList &listB)
{
    bool isEqual = true;
    int maxIdx = std::min(listA.size(), listB.size());
    if (maxIdx <= 0) return false;
    
    for (int idx=0; isEqual && idx<maxIdx; ++idx)
        isEqual = listA[idx] == listB[idx];
    
    return isEqual;
}


#endif // PLOT_PARAMETERS_H
