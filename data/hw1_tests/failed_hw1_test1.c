#include <stdio.h>

void main(){
    int i1;
    int i2;
    int i3;
    float f1;
    float f2;
    double d1;
    double d2;

    i1 = 10;
    i2 = 20;
    f1 = 15.0;
    f2 = 12.0;
    d1 = 13.0;
    d2 = 33.0;


    d1 = ((d1 == d2)?d1:(d1+1.0)) + ((d1 <= d2)? (2.5*4.0) :-3.2);
    printf("%f\n", d1);
    printf("%f\n", d2);
    printf("%f\n", f1);
    printf("%f\n", f2);

    i3 = (i1++) + (--i2);
    printf("%d\n", i1);
    printf("%d\n", i2);
    printf("%d\n", i3);

    i1 = 1;
    i2 = 0;
    i3 = (i1 && 0) || (!i2) || i3;
    printf("%d\n", i1);
    printf("%d\n", i2);
    printf("%d\n", i3);

    f1 *= (8.2+3.8);
    f2 /= (24.0/2.0);
    f2 += f1;
    f2 -= f1;
    printf("%f\n", f1);
    printf("%f\n", f2);
    return;
}
