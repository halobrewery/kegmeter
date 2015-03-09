#include "kegmeterdata.h"

#include <cassert>

KegMeterData::KegMeterData() :
    idx(-1),
    percent(0), fullMass(0), emptyMass(0), load(0), variance(0),
    hasPercent(false), hasFullMass(false), hasEmptyMass(false), hasLoad(false), hasVariance(false) {
}

KegMeterData::KegMeterData(int idx) :
    idx(idx),
    percent(0), fullMass(0), emptyMass(0), load(0), variance(0),
    hasPercent(false), hasFullMass(false), hasEmptyMass(false), hasLoad(false), hasVariance(false) {
}

KegMeterData::KegMeterData(const KegMeterData& copy) {
    *this = copy;
}

QString KegMeterData::buildUpdateSerialStr(int meterIdx) const {
    assert(meterIdx >= 0);

    QString serialStr("|Um");
    serialStr += QString("%1").arg(meterIdx, 3, 10, QChar('0'));
    serialStr += QString(",");

    if (this->hasPercent) {
        serialStr += QString("%1").arg(this->percent, 4, 'f', 2, QChar('0')) + QString(",");
    }
    else {
        return QString();
    }
    serialStr += QString("%1").arg(this->fullMass, 6, 'f', 2, QChar('0')) + QString(",");
    serialStr += QString("%1").arg(this->emptyMass, 6, 'f', 2, QChar('0'));

    return serialStr;
}

KegMeterData& KegMeterData::operator=(const KegMeterData& copy) {
    this->idx = copy.idx;
    this->percent = copy.percent;
    this->fullMass = copy.fullMass;
    this->emptyMass = copy.emptyMass;
    this->load = copy.load;
    this->variance = copy.variance;
    this->hasPercent = copy.hasPercent;
    this->hasFullMass = copy.hasFullMass;
    this->hasEmptyMass = copy.hasEmptyMass;
    this->hasLoad = copy.hasLoad;
    this->hasVariance = copy.hasVariance;

    return *this;
}
