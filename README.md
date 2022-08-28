# Active windows logger

A lightweight background app gathering information about active programs ( or rather active windows) opened by user and how much time user spent with specific app.
The logs are stored in CSV files with 4 columns:

-Time stamp   
-Name of the program   
-Window title   
-Time in milliseconds   

![CSV_Screenshot.png](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/CSV_Screenshot.png)

![example.csv](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/example.csv)


The CSV files location is [%APPDATA%]\ActiveWindowsLogger\logs

for each day will be created separate log file:

![logs_screenshot.png](https://github.com/alexkmbk/ActiveWindowsLogger/blob/master/logs_screenshot.png)


