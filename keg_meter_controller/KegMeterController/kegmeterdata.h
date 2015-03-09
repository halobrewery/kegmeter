#ifndef KEGMETERCONTROLLER_KEGMETERDATA_H
#define KEGMETERCONTROLLER_KEGMETERDATA_H

#include <cassert>

#include <QVariant>
#include <QDebug>
#include <QString>

class KegMeterData {
    friend QDataStream& operator<<(QDataStream& out, const KegMeterData& data);
    friend QDataStream& operator>>(QDataStream& in, KegMeterData& data);

public:
    KegMeterData();
    explicit KegMeterData(int idx);
    KegMeterData(const KegMeterData& copy);

    ~KegMeterData() {}

    int getIndex() const { return this->idx; }
    int getId() const { return this->idx+1; }

    void setIndex(int idx) { this->idx = idx; }

    void setPercent(float percent) {
        this->hasPercent = true;
        this->percent = percent;
    }
    void setFullMass(float fullMass) {
        this->hasFullMass = true;
        this->fullMass = fullMass;
    }
    void setEmptyMass(float emptyMass) {
        this->hasEmptyMass = true;
        this->emptyMass = emptyMass;
    }
    void setLoad(float load) {
        this->hasLoad = true;
        this->load = load;
    }
    void setVariance(float variance) {
        this->hasVariance = true;
        this->variance = variance;
    }

    float getPercent(bool& hasInfo) const {
        hasInfo = this->hasPercent;
        return this->percent;
    }
    float getFullMass(bool& hasInfo) const {
        hasInfo = this->hasFullMass;
        return this->fullMass;
    }
    float getEmptyMass(bool& hasInfo) const {
        hasInfo = this->hasEmptyMass;
        return this->emptyMass;
    }
    float getLoad(bool& hasInfo) const {
        hasInfo = this->hasLoad;
        return this->load;
    }
    float getVariance(bool& hasInfo) const {
        hasInfo = this->hasVariance;
        return this->variance;
    }

    QString buildUpdateSerialStr(int meterIdx) const;

    KegMeterData& operator=(const KegMeterData& copy);

private:
    int idx;
    float percent, fullMass, emptyMass, load, variance;
    bool hasPercent, hasFullMass, hasEmptyMass, hasLoad, hasVariance;
};

Q_DECLARE_METATYPE(KegMeterData);

inline QDebug operator<<(QDebug dbg, const KegMeterData &data) {
    dbg.nospace() << "KegMeterData(";

    bool hasInfoBefore = false;
    bool hasInfo = false;
    float temp = data.getPercent(hasInfo);
    if (hasInfo) {
        dbg.nospace() << "Percent: " << temp;
        hasInfoBefore = true;
    }

    hasInfo = false;
    temp = data.getEmptyMass(hasInfo);
    if (hasInfo) {
        if (hasInfoBefore) {
            dbg.nospace() << ", ";
        }
        dbg.nospace() << "Empty Amt: " << temp;
        hasInfoBefore = true;
    }

    hasInfo = false;
    temp = data.getFullMass(hasInfo);
    if (hasInfo) {
        if (hasInfoBefore) {
            dbg.nospace() << ", ";
        }
        dbg.nospace() << "Full Amt: " << temp;
        hasInfoBefore = true;
    }

    hasInfo = false;
    temp = data.getLoad(hasInfo);
    if (hasInfo) {
        if (hasInfoBefore) {
            dbg.nospace() << ", ";
        }
        dbg.nospace() << "Load: " << temp;
        hasInfoBefore = true;
    }

    hasInfo = false;
    temp = data.getVariance(hasInfo);
    if (hasInfo) {
        if (hasInfoBefore) {
            dbg.nospace() << ", ";
        }
        dbg.nospace() << "Variance: " << temp;
    }

    dbg.nospace() << ")";
    return dbg.maybeSpace();
}

inline QDataStream& operator<<(QDataStream& out, const KegMeterData& data) {
    out << data.idx;

    out << data.percent;
    out << data.fullMass;
    out << data.emptyMass;

    out << data.hasPercent;
    out << data.hasFullMass;
    out << data.hasEmptyMass;

    return out;
}

inline QDataStream& operator>>(QDataStream& in, KegMeterData& data) {
    in >> data.idx;

    in >> data.percent;
    in >> data.fullMass;
    in >> data.emptyMass;

    in >> data.hasPercent;
    in >> data.hasFullMass;
    in >> data.hasEmptyMass;

    return in;
}

#endif

