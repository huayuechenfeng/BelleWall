#ifndef BELLEWALL_VIDEO_TIMING_H
#define BELLEWALL_VIDEO_TIMING_H
namespace BelleVideoTiming {
inline int DisplayNumerator(int numerator,int denominator){
    return numerator<30*denominator?numerator:30*denominator;
}
inline long long NextDueUs(long long activeUs,int numerator,int denominator){
    const int display=DisplayNumerator(numerator,denominator);
    const long long base=1000000LL*denominator;
    const long long ordinal=activeUs*display/base;
    return ((ordinal+1)*base+display-1)/display;
}
inline int SourceFrame(long long activeUs,int numerator,int denominator,int frames){
    return int((activeUs*numerator/(1000000LL*denominator))%frames);
}
}
#endif
