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
/* hibrida 3 zonas SIN plegar (lo que propongo en C ahora) */
static void h3(const double*x,size_t n,double w,double*re,double*im){
    if(w<M_PI/4){ga(x,n,w,re,im);}
    else if(w>3*M_PI/4){gb(x,n,w,re,im);}
    else gK(x,n,w,re,im);
}
/* hibrida 3 zonas CON plegado: w_f=min(w,2pi-w), conjuga si w>pi */
static void h3f(const double*x,size_t n,double w,double*re,double*im){
    int conj=0; double wf=w;
    if(w>M_PI){ wf=2*M_PI-w; conj=1; }
    if(wf<M_PI/4) ga(x,n,wf,re,im);
    else if(wf>3*M_PI/4) gb(x,n,wf,re,im);
    else gK(x,n,wf,re,im);
    if(conj){ *im=-*im; }
}
/* hibrida 2 zonas original */
static void h2(const double*x,size_t n,double w,double*re,double*im){
    if(w<M_PI/2) ga(x,n,w,re,im); else gb(x,n,w,re,im);
}
int main(void){
    size_t n=131072; int R=4;
    double*x=malloc(n*sizeof(double));
    int steps=120;
    double w2=0,w3=0,w3f=0,a2=0,a3=0,a3f=0;
    double sum2=0,sum3=0,sum3f=0; long cnt=0;
    for(int r=0;r<R;r++){
        uint64_t st=12345+7919*r;
        for(size_t i=0;i<n;i++){ st=st*6364136223846793005ULL+1442695040888963407ULL; x[i]=(double)((int64_t)(st>>11))/(double)(1ULL<<52)*2.0-1.0; }
        for(int s=0;s<=steps;s++){
            double w=2*M_PI*s/steps;
            long double re=0,im=0;
            for(size_t i=0;i<n;i++){ long double ph=-(long double)w*(long double)i; re+=(long double)x[i]*cosl(ph); im+=(long double)x[i]*sinl(ph);}
            double Rm=hypotl((long double)0+re,im);
            double r1,r2,e;
            h2(x,n,w,&r1,&r2); e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>w2){w2=e;a2=w;} sum2+=e;
            h3(x,n,w,&r1,&r2); e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>w3){w3=e;a3=w;} sum3+=e;
            h3f(x,n,w,&r1,&r2); e=fabs(hypot(r1,r2)-Rm)/Rm; if(e>w3f){w3f=e;a3f=w;} sum3f+=e;
            cnt++;
        }
    }
    printf("peor error relativo en [0,2pi), N=131072, %d realizaciones:\n",R);
    printf("  hibrida 2 zonas (original, corte en pi/2) = %.3e  (peor en w=%.4f, w/pi=%.3f)\n",w2,a2,a2/M_PI);
    printf("  hibrida 3 zonas (sin plegar)              = %.3e  (peor en w=%.4f, w/pi=%.3f)\n",w3,a3,a3/M_PI);
    printf("  hibrida 3 zonas (con plegado 2pi-w)      = %.3e  (peor en w=%.4f, w/pi=%.3f)\n",w3f,a3f,a3f/M_PI);
    printf("error medio sobre la rejilla:              2z=%.3e  3z=%.3e  3z+pleg=%.3e\n",sum2/cnt,sum3/cnt,sum3f/cnt);
    free(x); return 0;
}
