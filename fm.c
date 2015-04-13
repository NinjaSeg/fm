#include <stdlib.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <stdint.h>
#include <math.h>

typedef float sample_t;
typedef int samplecount_t;
typedef float freq_t;
typedef float msec_t;

samplecount_t samplerate;

static freq_t basefreq=55;
static freq_t freq;

// Converts a sample count into a time in milliseconds
static msec_t time(samplecount_t samples){
	return (1000.0/samplerate)*samples;
}

// Convert time in milliseconds to a sample count
static samplecount_t samples(msec_t time){
	return (time/1000.0)*samplerate;
}

// Fast fixed point sine approximation
int32_t isin_S3(int32_t x){
    int qN=13; // Q-pos for quarter circle
    int qA=29; // Q-pos for output
    int qP=15; // Q-pos for parentheses intermediate
    int qR=2*qN-qP;
    int qS=qN+qP+1-qA;

    x=x<<(30-qN);    // shift to full s32 range (Q13->Q30)
    if((x^(x<<1))<0) // test for quadrant 1 or 2
        x=(1<<31)-x;
    x=x>>(30-qN);
    return x*((3<<qP)-(x*x>>qR))>>qS;
}

#define f2Q(x) ( (x) * (1.0/(2*M_PI)) * pow(2,15) )
#define Q2f(x) ( (int32_t)(x) * (1.0/(float)(1<<29)) )
static sample_t sinQ(sample_t in){
	//static float max=0.0;
	int32_t out=isin_S3(f2Q(in));
	//float error=fabs(sin(in)-Q2f(out));
	//if(error>max){
	//	max=error;
	//	printf("%f\n",max);
	//}
//	printf("%f\n",error);
	return Q2f(out);
}

// Sine oscillator with FM. 
void sin_osc(sample_t* in, sample_t* out, int len, freq_t freq, samplecount_t samplepos){
	int i;
	float pos;
	for(i=0;i<len;i++){
		pos=samplepos/(samplerate/freq);
		out[i]=sinQ(2*M_PI*(pos+in[i]));
		samplepos++;
	}
}

// Linear AR envelope generator
void ar_env(sample_t* restrict in, sample_t* restrict out, int len, samplecount_t samplepos, samplecount_t envstart, sample_t mod, msec_t attack, msec_t release){
	int i;
	float envpos;
	sample_t s;
	for(i=0;i<len;i++){
		envpos=time(samplepos-envstart);
		if(envpos<attack){
			s=envpos*(mod/attack);
		}else{
			s=mod-(envpos-attack)/release;
		}
		if(s<0.0){
			s=0.0;
		}
		out[i]=in[i]*s;
		samplepos++;
	}
}

// Two operator FM patch
static samplecount_t synth(sample_t* buf, int len, freq_t freq, samplecount_t samplepos, samplecount_t envstart){
	int i;
	for(i=0;i<len;i++){
		buf[i]=0.0;
	}
	sin_osc(buf,buf,len,freq*1.2,samplepos);
	ar_env(buf,buf,len,samplepos,envstart,time(samplepos)/50000,0,750);
	sin_osc(buf,buf,len,freq,samplepos);
	ar_env(buf,buf,len,samplepos,envstart,1,4,100);
	return samplepos+len;
}

void synthcall(SDL_AudioSpec* fmt, int16_t* stream, int len){
	static int envstart=0;
	static int samplepos=0;
	static float nexttime=0;
	static sample_t* buf=NULL;
	int i;
	len=len/sizeof(int16_t);
	if(buf==NULL){
		buf=calloc(len,sizeof(sample_t));
	}
	if(time(samplepos)>=nexttime){
		envstart=samplepos;
		nexttime=time(samplepos)+100;
		freq=freq*2;
		if(freq>440){
			basefreq=basefreq-(basefreq/4);
			if(basefreq<50){
				basefreq=220;
			}
			freq=basefreq;
		}
	}
	samplepos=synth(buf,len,freq,samplepos,envstart);
	for(i=0;i<len;i++){
		stream[i]=(int16_t)(buf[i]*INT16_MAX);
	}
}

int main(void){
	SDL_AudioSpec desiredfmt,actualfmt;
	desiredfmt.freq = 48000;
	desiredfmt.format = AUDIO_S16SYS;
	desiredfmt.channels = 1;
	desiredfmt.samples = 512;
	desiredfmt.callback = synthcall;
	desiredfmt.userdata = NULL;

	if(SDL_OpenAudio(&desiredfmt,&actualfmt)<0){
		fprintf(stderr, "Unable to open audio: %s\n",SDL_GetError());
		exit(1);
	}

	samplerate=actualfmt.freq;
	freq=basefreq;

	SDL_PauseAudio(0);

	while(1){
		sleep(5);
	}

	SDL_CloseAudio();
	return 0;
}
