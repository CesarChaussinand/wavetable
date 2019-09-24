#include <stdlib.h>
#include <math.h>
//-lm
#include <stdio.h>


double midi2freq(double midi){
double temp = (midi-69)/12;
double freq = pow(2,temp);
freq = freq*440;
return freq;
}

double log2(double x){
double log2 = log10(x)/log10(2);
return log2;
}

double freq2midi(double freq){
double midi = 69 + 12*log2(freq/440.0d);
return midi;
}
