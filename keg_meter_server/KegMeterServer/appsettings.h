#ifndef KEGMETERCONTROLLER_APPSETTINGS_H
#define KEGMETERCONTROLLER_APPSETTINGS_H

#include <QString>

class AppSettings {
public:
    static const char* KEG_METER_DIR;

    static const char* KEG_METER_KEGTYPE;
    static const char* KEG_METER_PERCENT;
    static const char* KEG_METER_CAL_FULLAMT;

    static const char* KEG_METER_CAL_EMPTY_SENSOR_VAL;
    static const char* KEG_METER_CAL_NONEMPTY_SENSOR_VAL;
    static const char* KEG_METER_CAL_NONEMPTY_MASS_VAL;

    static const char* KEG_METER_EMPTY_CAL_COMPLETE;
    static const char* KEG_METER_NONEMPTY_CAL_COMPLETE;

    static QString buildKegMeterKey(int meterIdx, const char* subFieldKey);

};

#endif // KEGMETERCONTROLLER_APPSETTINGS_H

