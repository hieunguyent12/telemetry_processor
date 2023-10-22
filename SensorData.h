#ifndef TELEMETRY_PROCESSOR_SENSORDATA_H
#define TELEMETRY_PROCESSOR_SENSORDATA_H

#endif //TELEMETRY_PROCESSOR_SENSORDATA_H

#include <string>
using namespace std;
#pragma once

struct SensorData {
    int time;
    string label;
    string units;
    int value;
    SensorData(int t, string l, string u, int v) {
        time = t;
        label = l;
        units = u;
        value = v;
    }
};