//
// Created by QianL on 2020/4/16.
//

#ifndef ELYSIUMNET_TIME_HPP
#define ELYSIUMNET_TIME_HPP

#include <string>
#include <iostream>
#include <time.h>

using namespace std;

static const string GetNowTime() {
    time_t setTime;
    time(&setTime);
    tm* ptm = localtime(&setTime);
    std::string time = std::to_string(ptm->tm_year + 1900)
                       + "/"
                       + std::to_string(ptm->tm_mon + 1)
                       + "/"
                       + std::to_string(ptm->tm_mday)
                       + " "
                       + std::to_string(ptm->tm_hour) + ":"
                       + std::to_string(ptm->tm_min) + ":"
                       + std::to_string(ptm->tm_sec);
    return time;
}

#endif //ELYSIUMNET_TIME_HPP
