#include "DSP.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    ur::Biquad f;
    f.peak(48000,1000,1,0);
    for(int i=0;i<200000;++i) {
        auto x=i==0?1.0f:0.0f;
        assert(std::isfinite(f.process(x)));
    }
    f.lowpass(48000,1000);f.reset();
    double sum=0;
    for(int i=0;i<100000;++i)sum+=f.process(1.0f);
    assert(sum>90000 && sum<101000);
    f.highpass(48000,1000);f.reset();
    double tail=0;
    for(int i=0;i<100000;++i){auto y=f.process(1.0f);if(i>90000)tail+=std::abs(y);}
    assert(tail<0.1);
    ur::StereoStats s;
    for(int i=0;i<1000;++i){const auto v=std::sin(i*0.4f);s.push(v,v);}
    assert(s.correlation()>0.999f);
    ur::StereoStats anti;
    for(int i=0;i<1000;++i){const auto v=std::sin(i*0.4f);anti.push(v,-v);}
    assert(anti.correlation()<-0.999f);
    assert(std::abs(ur::gainToDb(ur::dbToGain(-6))+6)<0.001f);
    std::cout<<"DSP sanity tests passed\n";
}
