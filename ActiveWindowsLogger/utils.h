#pragma once
#include <time.h>
#include <ctime>
#include <stdio.h>
#include <string>

using namespace std;

wstring CurrentDate();
std::wstring time_stamp(const std::wstring&  fmt = L"%F %T");
