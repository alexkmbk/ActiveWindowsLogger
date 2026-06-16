# Active windows logger

A lightweight background app gathering information about active programs ( or rather active windows) opened by user and how much time user spent with specific app.
The logs are stored in CSV files with 4 columns:

-Time stamp   
-Name of the program   
-Window title   
-Time in milliseconds   

![CSV_Screenshot.png](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/CSV_Screenshot.png)

![example.csv](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/example.csv)


The CSV files location is %LOCALAPPDATA%\ActiveWindowsLogger\logs

For each day will be created separate log file:

![logs_screenshot.png](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/logs_screenshot.png)


## Requirements

Active Windows Logger supports Windows 7 and later.

## Settings.ini

The application reads settings from `settings.ini` in the logs directory. If the file does not exist, it can be created from the tray menu by opening the settings command.

Available settings:

```ini
[Filters]
ProgramsFilter=chrome,foobar2000,firefox

[Tracking]
StopLoggingwhenInactiveInterval=5
PollingIntervalMs=500

[LogsFormat]
Separator=,
```

- `ProgramsFilter` is a comma-separated list of process names that should be excluded from logging.
- `StopLoggingwhenInactiveInterval` suspends tracking after the specified number of inactive minutes. Use `0` to disable inactivity suspension. The default value is `5`.
- `PollingIntervalMs` controls how often the active window is checked. The default value is `500`. If the value is `0` or missing, `500` is used. Values below `100` are rounded up to `100`.
- `Separator` defines the CSV column separator. The default value is `,`.

## Building

Open `ActiveWindowsLogger.sln` in Visual Studio and build the `Release|x86` configuration.

The executable will be created at:

```text
Release\ActiveWindowsLogger.exe
```

You can also build it from a Visual Studio Developer Command Prompt:

```text
msbuild ActiveWindowsLogger.sln /p:Configuration=Release /p:Platform=x86
```

