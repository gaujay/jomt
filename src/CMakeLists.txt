
find_package(Qt5 COMPONENTS Widgets Charts DataVisualization REQUIRED)

# Files
set(HEADERS
	mainwindow.h
	benchmark_results.h
	result_parser.h
	plot_parameters.h
	commandline_handler.h
	result_selector.h
	plotter_linechart.h
	plotter_barchart.h
	plotter_boxchart.h
	plotter_3dbars.h
	plotter_3dsurface.h
)
set(SOURCES
	main.cpp
	mainwindow.cpp
	benchmark_results.cpp
	result_parser.cpp
	commandline_handler.cpp
	result_selector.cpp
	plotter_linechart.cpp  
	plotter_barchart.cpp
	plotter_boxchart.cpp
	plotter_3dbars.cpp
	plotter_3dsurface.cpp
)
set(FORMS
  mainwindow.ui
  result_selector.ui
  plotter_linechart.ui
  plotter_barchart.ui
  plotter_boxchart.ui
  plotter_3dbars.ui
  plotter_3dsurface.ui
)

# Target
add_executable(JackOfMyTrade
  ${HEADERS}
  ${SOURCES}
  ${FORMS}
)

target_link_libraries(JackOfMyTrade PRIVATE Qt5::Widgets Qt5::Charts Qt5::DataVisualization)
