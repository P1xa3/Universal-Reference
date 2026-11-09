#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace ur {
constexpr double pi = 3.14159265358979323846;
inline float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
inline float gainToDb(float x) { return 20.0f * std::log10(std::max(x, 1.0e-9f)); }

// Direct-form II transposed biquad; coefficients generated off the audio thread.
struct Biquad {
    double b0=1, b1=0, b2=0, a1=0, a2=0, z1=0, z2=0;
    void reset() { z1=z2=0; }
    void normalise(double nb0, double nb1, double nb2, double na0, double na1, double na2) {
        b0=nb0/na0; b1=nb1/na0; b2=nb2/na0; a1=na1/na0; a2=na2/na0;
    }
    void peak(double sampleRate, double hz, double q, double gainDb) {
        const auto w = 2*pi*std::clamp(hz, 20.0, sampleRate*0.45)/sampleRate;
        const auto alpha=std::sin(w)/(2*std::max(q,0.1));
        const auto a=std::pow(10.0,gainDb/40.0);
        normalise(1+alpha*a,-2*std::cos(w),1-alpha*a,1+alpha/a,-2*std::cos(w),1-alpha/a);
    }
    void lowpass(double sampleRate, double hz) {
        const auto w=2*pi*std::clamp(hz,20.0,sampleRate*0.45)/sampleRate;
        const auto c=std::cos(w), alpha=std::sin(w)/std::sqrt(2.0);
        normalise((1-c)/2,1-c,(1-c)/2,1+alpha,-2*c,1-alpha);
    }
    void highpass(double sampleRate, double hz) {
        const auto w=2*pi*std::clamp(hz,20.0,sampleRate*0.45)/sampleRate;
        const auto c=std::cos(w), alpha=std::sin(w)/std::sqrt(2.0);
        normalise((1+c)/2,-(1+c),(1+c)/2,1+alpha,-2*c,1-alpha);
    }
    float process(float x) {
        const auto y=b0*x+z1;
        z1=b1*x-a1*y+z2; z2=b2*x-a2*y;
        if (std::abs(z1)<1e-25) z1=0;
        if (std::abs(z2)<1e-25) z2=0;
        return static_cast<float>(y);
    }
};

struct StereoStats {
    double sumL=0,sumR=0,sumL2=0,sumR2=0,sumLR=0;
    float peak=0;
    int count=0;
    void push(float l,float r) {
        sumL+=l;sumR+=r;sumL2+=double(l)*l;sumR2+=double(r)*r;sumLR+=double(l)*r;
        peak=std::max({peak,std::abs(l),std::abs(r)}); ++count;
    }
    float rmsDb() const { return gainToDb(static_cast<float>(std::sqrt((sumL2+sumR2)/(std::max(1,count)*2.0)))); }
    float correlation() const {
        const auto denom=std::sqrt(sumL2*sumR2);
        return denom>1e-15?static_cast<float>(std::clamp(sumLR/denom,-1.0,1.0)):0.0f;
    }
};
}
