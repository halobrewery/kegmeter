#ifndef KEGMETERCONTROLLER_KEGMETERDATA_H
#define KEGMETERCONTROLLER_KEGMETERDATA_H

#include <cassert>

class KegMeterData {
public:
    explicit KegMeterData(int idx) : idx(idx), percent(0.0), hasPercent(false),
        hasFullMass(false), hasEmptyMass(false), hasLoad(false), hasVariance(false) {}
    ~KegMeterData() {}

    int getIndex() const { return this->idx; }
    int getId() const { return this->idx+1; }

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

private:
    int idx;
    float percent, fullMass, emptyMass, load, variance;
    bool hasPercent, hasFullMass, hasEmptyMass, hasLoad, hasVariance;
};

#endif

