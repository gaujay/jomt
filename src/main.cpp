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

#include "benchmark_results.h"
#include "result_parser.h"
#include "commandline_handler.h"
#include "result_selector.h"

#include <QApplication>
//#include <QDir>

#define APP_VER "0.9b"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(APP_VER);
    
    
    //
    // Command line options
    CommandLineHandler cmdHandler;
    bool isCmd = cmdHandler.process(app);
    
    if (!isCmd)
    {
//        // Parse Test
//        QDir jmtDir("E:/Work/JMT/");
//        QString fileName = "bench_results.json";
//        
//        BenchResults bchResults = ResultParser::parseJsonFile( jmtDir.filePath(fileName) );
//        
//        if ( bchResults.benchmarks.isEmpty() ) {
//            qCritical("Error: unable to open benchmark results file");
//            return 1;
//        }
//        // Selector Test
//        ResultSelector rs(bchResults, jmtDir.filePath(fileName));
        
        // Show empty selector
        ResultSelector* rs = new ResultSelector();
        rs->show();
    }
    
    //
    // Execute
    return app.exec();
}
