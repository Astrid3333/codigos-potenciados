#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static void gK(const double*x,size_t n,double w,double*re,double*im){
    double K=2.0*cos(w),s1=0,s2=0,s;
    for(size_t i=0;i<n;i++){s=x[i]+K*s1-s2;s2=s1;s1=s;}
    s=K*s1-s2; *re=s-cos(w)*s1; *im=sin(w)*s1;
}
static void ga(const double*x,size_t n,double w,double*re,double*im){
    double al=4.0*sin(w/2.0)*sin(w/2.0),s=0,d=0;
    for(size_t i=0;i<n;i++){d=d+x[i]-al*s;s=s+d;}
    d=d-al*s;s=s+d; double s1=s-d; *re=s-cos(w)*s1; *im=sin(w)*s1;
}
static void gb(const double*x,size_t n,double w,double*re,double*im){
    double be=4.0*cos(w/2.0)*cos(w/2.0),s=0,d=0;
    for(size_t i=0;i<n;i++){d=x[i]+be*s-d;s=d-s;}
    d=be*s-d;s=d-s; double s1=d-s; *re=s-cos(w)*s1; *im=sin(w)*s1;
}
int main(void){
    size_t n=131072; int R=4;
    double*x=malloc(n*sizeof(double));
    /* barrido denso de w en [0,2pi) */
    int steps=120;
    double worstK[steps+1],worstA[steps+1],worstB[steps+1];
    for(int s=0;s<=steps;s++){
        worstK[s]=worstA[s]=worstB[s]=0;
    }
    for(int r=0;r<R;r++){
        uint64_t st=12345+7919*r;
        for(size_t i=0;i<n;i++){ st=st*6364136223846793005ULL+1442695040888963407ULL; x[i]=(double)((int64_t)(st>>11))/(double)(1ULL<<52)*2.0-1.0; }
        for(int s=0;s<=steps;s++){
            double w=2*M_PI*s/steps;
            long double re=0,im=0;
            for(size_t i=0;i<n;i++){ long double ph=-(long double)w*(long double)i; re+=(long double)x[i]*cosl(ph); im+=(long double)x[i]*sinl(ph);}
            double Rm=hypotl((long double)0+re,im);
            double r1,r2;
            gK(x,n,w,&r1,&r2); double e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>worstK[s])worstK[s]=e;
            ga(x,n,w,&r1,&r2); e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>worstA[s])worstA[s]=e;
            gb(x,n,w,&r1,&r2); e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>worstB[s])worstB[s]=e;
        }
    }
    printf("%6s %10s %11s %11s %11s  %s\n","w/pi","w","K","alpha","beta","mejor");
    for(int s=0;s<=steps;s+=4){
        double w=2*M_PI*s/steps;
        const char*best="K"; double bv=worstK[s];
        if(worstA[s]<bv){best="alpha";bv=worstA[s];}
        if(worstB[s]<bv){best="beta";bv=worstB[s];}
        printf("%6.3f %10.4f %11.3e %11.3e %11.3e  %s\n",w/M_PI,w,worstK[s],worstA[s],worstB[s],best);
    }
    free(x); return 0;
}
