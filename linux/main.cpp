// -*- coding:utf-8-unix; mode:c; -*-
//
// get the active window on X window system
//

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <iostream>
#include <string>

#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/time.h>

#include <stdio.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "./lib/SimpleIni/SimpleIni.h"
#include "utils.h"
#include "xutils.h"
#include "messagebox.h"

//#include <glib-2.0/glib-object.h>
//#include <libupower-glib/upower.h>


//#include <upower.h>

// Bool xerror = False;

namespace fs = std::filesystem;

using namespace std;

string sCurrentWindowName = "";
string sCurrentProcessName = "";
bool bPaused = false;

std::chrono::high_resolution_clock::time_point iCurrentWindowStartTime = std::chrono::high_resolution_clock::now();
auto lastFlush = std::chrono::high_resolution_clock::now();
auto lastWrite = std::chrono::high_resolution_clock::now();

std::ofstream oFile;

fs::path currentDir = std::filesystem::current_path();
fs::path logsDir = "";
fs::path settingsFile = "";
string separator = ",";
string fileName = time_stamp("%F") + ".csv";

vector<std::string> filters;
int iStopLoggingwhenInactiveInterval = 5;

auto currentDay = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
auto lastMove = std::chrono::system_clock::now();
int lastCursotPos;

void write(const chrono::high_resolution_clock::time_point current_time, chrono::milliseconds::rep time);

void handle_signal(int sig)
{
  if (sig == SIGTERM || sig == SIGABRT || sig == SIGTSTP || sig == SIGINT || sig ==  SIGSTOP || sig ==  SIGKILL)
  {
    Button buttons[1];
    buttons[0].label = L"Ok";
    buttons[0].result = 0;
    int res = Messagebox("Active windows logger", L"SIGTERM!", buttons, 1);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count();
    write(end_time, time);
    iCurrentWindowStartTime = end_time;
  }
  else // timer
  {
    string nextWindowName = "";
    string processName = "";
    if (getActiveWindowAndProcessName(nextWindowName, processName))
    {

      if (sCurrentProcessName.length() > 0 || sCurrentWindowName.length() > 0)
      {
        const auto end_time = std::chrono::high_resolution_clock::now();                                               // current timestamp
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - iCurrentWindowStartTime).count(); // period

        const auto sinceLastWritePeriod = std::chrono::duration_cast<std::chrono::seconds>(end_time - lastWrite).count();
        if (time > 0 && ((sinceLastWritePeriod > 60) || (nextWindowName.compare(sCurrentWindowName) != 0)))
        {
          write(end_time, time);
        }
      }
      else
      {
        if (sCurrentWindowName.length() == 0 && nextWindowName.length() > 0)
        {
          sCurrentWindowName.assign(nextWindowName);
        }
        if (sCurrentProcessName.length() == 0 && processName.length() > 0)
        {
          fs::path p(processName);
          sCurrentProcessName.assign(p.stem().string());
        }
      }
    }
  }
}

void readSettings()
{

  if (!fs::exists(settingsFile)) {
		return;
	}
	CSimpleIniA ini;
	ini.SetUnicode(true);
	ini.LoadFile(settingsFile.c_str());
	filters = split(ini.GetValue("Filters", "ProgramsFilter", ""), ',');
	separator = ini.GetValue("LogsFormat", "Separator", ",");
	if (separator.size() == 0) {
		separator = ",";
	}
	iStopLoggingwhenInactiveInterval = ini.GetLongValue("Tracking", "StopLoggingwhenInactiveInterval", 5);
}

void write(const chrono::high_resolution_clock::time_point current_time, chrono::milliseconds::rep time) {

	// sometimes, when the PC is waking up after sleep mode, the timer could have value from the beginning of sleeping
	if (time > 62000) {
		time = 62000;
	}

	if (sCurrentProcessName.length() != 0 || sCurrentWindowName.length() != 0) {
		stringstream line;
		line << time_stamp() << separator;

		replaceAll(sCurrentProcessName, "\"", "\"\"");
		line << "\"" << sCurrentProcessName << "\"" << separator;

		replaceAll(sCurrentWindowName, "\"", "\"\"");
		line << "\"" << sCurrentWindowName << "\"" << separator;

		line << time;
		line << endl;

		// if it is the next day, we should start a new file
		auto day = localtime_xp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())).tm_mday;
		if (day != currentDay) {
			currentDay = day;
			oFile.close();
			fileName = (logsDir / (time_stamp("%F") + ".csv")).string();
			oFile.open("fileName", std::ios::app);
			//string sep_utf8 = to_utf8(separator);
			oFile << "Timestamp" << separator << "Program" << separator << "WindowTitle" << separator << "Time" << endl;
		}

		if (oFile.bad() || !oFile.good() || oFile.fail()) {
			oFile.close();
			oFile.open("fileName", std::ios::app);
		}
		if (!oFile.is_open()) {
			oFile.open("fileName", std::ios::app);
		}

		if (std::find(filters.begin(), filters.end(), sCurrentProcessName) == filters.end()) {
			oFile << line.str();
			lastWrite = current_time;
		}
	}
	// flush the data to the file every 5 seconds
	if (std::chrono::duration_cast<std::chrono::seconds>(current_time - lastFlush).count() > 5) {
		oFile.flush();
		lastFlush = current_time;
	}

}


int main(int argc, char* argv[])
{
  setlocale(LC_ALL, ""); // see man locale

  auto lockFileId = processExist(argv[0]);
  if (lockFileId == 0)
  {
    Button buttons[2];
    buttons[0].label = L"Ok";
    buttons[0].result = 0;
    buttons[1].label = L"Cancel";
    buttons[1].result = 1;
    int res = Messagebox("Active windows logger", L"This application is already running, continue anyway?", buttons, 2);
    if (res == 1)
    {
      return -1;
    }
    releaseLockFile(lockFileId, argv[0]);
  }

auto AppDataFolder = GetAppDataFolderPath();
	if (AppDataFolder.native().size() > 0) {
		AppDataFolder = AppDataFolder / L"ActiveWindowsLogger";

		if (!fs::exists(AppDataFolder)) {
			fs::create_directory(AppDataFolder);
		}
	}

	if (fs::exists(AppDataFolder)) {
		logsDir = AppDataFolder / "logs";
	}
	else {
		logsDir = currentDir / L"logs";
	}

	if (!fs::exists(logsDir)) {
		if (!fs::create_directory(logsDir)) {
			return -1;
		}
	}
	settingsFile = logsDir / L"settings.ini";
	readSettings();

	fileName = (logsDir / fileName).string();
	oFile.open(fileName, std::ios::app);

	if (!oFile.is_open()) {
		return -1;
	}
	if (std::filesystem::file_size(fileName) == 0) {
		//string sep_utf8 = to_utf8(separator);
		oFile << "Timestamp" << separator << "Program" << separator << "WindowTitle" << separator << "Time" << endl;
	}
	
  Display *display = XOpenDisplay(NULL);

  if (!display)
  {
    fprintf(stderr, "Failed to open display\n");
    return 1;
  }



  // getActiveWindowAndProcessName(windowTitle, processName);

  // Install signal handler for SIGALRM
  signal(SIGALRM, handle_signal);

  // terminate process signal
  signal(SIGTERM, handle_signal);

  // Create a timer that will expire every 1 second
  struct itimerval timer;
  // Установка интервала в 500 миллисекунд (0.5 секунды)
  timer.it_interval.tv_sec = 0;       // Секунды
  timer.it_interval.tv_usec = 500000; // Микросекунды (500 миллисекунд)
  // Установка начального таймера (первый выстрел) на тот же интервал
  timer.it_value = timer.it_interval;

  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
  {
    std::cerr << "Failed to set timer." << std::endl;
    exit(EXIT_FAILURE);
  }

Window window;
    int screen_num = DefaultScreen(display);
    unsigned long background = WhitePixel(display, screen_num);
    unsigned long border = BlackPixel(display, screen_num);

XSetWindowAttributes attributes;
attributes.background_pixel = background;
attributes.border_pixel = border;;

    window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 400, 300, 0,
                           DefaultDepth(display, screen_num), InputOutput,
                           DefaultVisual(display, screen_num),
                           CWBackPixel | CWBorderPixel,
                           &attributes);

    //XMapWindow(display, window);
    //XFlush(display);
    // sleep(10);
    // XCloseDisplay(display);


  XScreenSaverInfo *info = XScreenSaverAllocInfo();


  // Wait for signals to arrive
  while (true)
  {

    XEvent event;
    XNextEvent(display, &event);

    switch (event.type)
    {
    case ScreenSaverActive:
      std::cout << "ScreenSaverActive event" << std::endl;
      break;
    case ScreenSaverNotify:
      std::cout << "ScreenSaverNotify" << std::endl;
      // if (event.xscreensaver.state == ScreenSaverOn) {
      //     std::cout << "Screen saver is active." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverOff) {
      //     std::cout << "Screen saver is inactive." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverCycle) {
      //     std::cout << "Screen saver is cycling." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverBlanked) {
      //     std::cout << "Screen saver is blanked." << std::endl;
      // }
      // else {
      //     // X server is locked
      //     std::cout << "X server is locked." << std::endl;
      // }
      break;
    default:
      break;
    }

    // XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);

    // if (info->state == ScreenSaverOn)
    // {
    //   printf("Screen saver is active\n");
    // }
    // else if (info->state == ScreenSaverOff)
    // {
    //   printf("Screen saver is inactive\n");
    // }
    // else if (info->state == ScreenSaverBlanked)
    // {
    //   printf("Screen is locked\n");
    //   // Do something when screen is locked
    // }

    //sleep(1);
  }

  XFree(info);
  XCloseDisplay(display);

// Снятие блокировки и закрытие файла
releaseLockFile(lockFileId, argv[0]);
 

  return 0;
}