# Jack of my trade

Automation and visualization tool for Google benchmark.

Built upon Qt5 *Charts* and *DataVisualization* modules.

![Screenshot_00](docs/screenshots/00.png)
![Screenshot_01](docs/screenshots/01.png)
![Screenshot_02](docs/screenshots/02.png)
![Screenshot_03](docs/screenshots/03.png)
![Screenshot_04](docs/screenshots/04.png)
![Screenshot_05](docs/screenshots/05.png)

### Features

- Only visualization part is currently available
- Parse Google benchmark results as json files
- Support old naming format and aggregate data (min, median, mean)
- Multiple 2D and 3D chart types
- Benchmarks and axes selection
- Plotting options (theme, ranges, logarithm, ...)

### Command line

Direct chart plotting with parameters is available through command line options.

```
Options:
  -?, -h, --help                   Displays this help.
  -v, --version                    Displays version information.
  --ct, --chart-type <chart_type>  Chart type (e.g. Lines, Boxes, 3DBars)
  --cx, --chart-x <chart_x>        Chart X-axis (e.g. a1, t2)
  --cy, --chart-y <chart_y>        Chart Y-axis (e.g. CPUTime, Bytes,
                                   RealMeanTime, ItemsMin)
  --cz, --chart-z <chart_z>        Chart Z-axis (e.g. auto, a2, t1)
  --ap, --append <files...>        Files to append by renaming (uses ';' as
                                   separator)
  --ow, --overwrite <files...>     Files to append by overwriting (uses ';' as
                                   separator)

Arguments:
  file                             Benchmark results file in json to parse.
```

### License

As the Qt modules it uses, this application is licensed under *GNU GPL-3.0-or-later*.
