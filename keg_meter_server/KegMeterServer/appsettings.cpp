#include "appsettings.h"

const char* AppSettings::KEG_METER_DIR = "keg_meter_data";

const char* AppSettings::KEG_METER_KEGTYPE      = "kegtype";
const char* AppSettings::KEG_METER_PERCENT      = "percent";
const char* AppSettings::KEG_METER_CAL_FULLAMT  = "cal_full";

const char* AppSettings::KEG_METER_CAL_EMPTY_SENSOR_VAL    = "cal_empty_sensor";
const char* AppSettings::KEG_METER_CAL_NONEMPTY_SENSOR_VAL = "cal_nonempty_sensor";
const char* AppSettings::KEG_METER_CAL_NONEMPTY_MASS_VAL   = "cal_nonempty_mass";

const char* AppSettings::KEG_METER_EMPTY_CAL_COMPLETE    = "empty_cal_complete";
const char* AppSettings::KEG_METER_NONEMPTY_CAL_COMPLETE = "nonempty_cal_complete";

QString AppSettings::buildKegMeterKey(int meterIdx, const char* subFieldKey) {
    return QString(QString(KEG_METER_DIR) + QString("/%1/").arg(meterIdx) + QString(subFieldKey));
}
