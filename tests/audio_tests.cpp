#include "Analyzer.h"
#include "Processor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
using namespace Steinberg;
using namespace Steinberg::Vst;
void require(bool condition,const char* message){if(!condition){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
template<class T>void passthrough(bool inPlace,int channels,double rate,int block){
  auto processor=new Meme::Processor;
  require(processor->initialize(nullptr)==kResultOk,"initialize");
  SpeakerArrangement layout=channels==1?SpeakerArr::kMono:SpeakerArr::kStereo;
  require(processor->setBusArrangements(&layout,1,&layout,1)==kResultOk,"bus negotiation");
  ProcessSetup setup{};setup.processMode=kRealtime;setup.sampleRate=rate;setup.maxSamplesPerBlock=block;setup.symbolicSampleSize=sizeof(T)==4?kSample32:kSample64;
  require(processor->setupProcessing(setup)==kResultOk,"setup");processor->setActive(true);
  std::array<std::vector<T>,2> input,output,expected;
  std::mt19937 random(123);std::uniform_real_distribution<double> distribution(-2,2);
  for(int c=0;c<channels;++c){input[c].resize(block);output[c].resize(block);for(auto& v:input[c])v=T(distribution(random));
    if(block>3){input[c][0]=-T(0);input[c][1]=std::numeric_limits<T>::infinity();input[c][2]=std::numeric_limits<T>::quiet_NaN();}
    expected[c]=input[c];}
  T* ip[]={input[0].data(),input[1].data()};T* op[]={inPlace?input[0].data():output[0].data(),inPlace?input[1].data():output[1].data()};
  AudioBusBuffers in{},out{};in.numChannels=out.numChannels=channels;
  if constexpr(sizeof(T)==4){in.channelBuffers32=ip;out.channelBuffers32=op;}else{in.channelBuffers64=ip;out.channelBuffers64=op;}
  ProcessData data{};data.processMode=kRealtime;data.symbolicSampleSize=setup.symbolicSampleSize;data.numSamples=block;data.numInputs=data.numOutputs=1;data.inputs=&in;data.outputs=&out;
  require(processor->process(data)==kResultOk,"processing");
  for(int c=0;c<channels;++c)require(std::memcmp(op[c],expected[c].data(),sizeof(T)*block)==0,"bit identical passthrough");
  ParameterChanges bypass;
  int32 queueIndex=0, pointIndex=0;
  auto* queue=bypass.addParameterData(Meme::bypassId,queueIndex);
  queue->addPoint(0,1,pointIndex);data.inputParameterChanges=&bypass;
  require(processor->process(data)==kResultOk,"bypass processing");
  for(int c=0;c<channels;++c)require(std::memcmp(op[c],expected[c].data(),sizeof(T)*block)==0,"bit identical bypass");
  MemoryStream state;require(processor->getState(&state)==kResultOk,"save bypass state");
  state.seek(0,IBStream::kIBSeekSet,nullptr);require(processor->setState(&state)==kResultOk,"restore bypass state");
  in.silenceFlags=(1u<<channels)-1;require(processor->process(data)==kResultOk,"silence processing");
  require(out.silenceFlags==in.silenceFlags,"silence propagation");
  for(int c=0;c<channels;++c)for(int n=0;n<block;++n)require(op[c][n]==0,"silent buffers cleared");
  data.numSamples=0;require(processor->process(data)==kResultOk,"zero samples flush");
  processor->setActive(false);processor->terminate();processor->release();
}
int main(){
  const auto started=std::chrono::steady_clock::now();
  for(double rate:{22050.,44100.,48000.,96000.,192000.})for(int channels:{1,2})for(int block:{1,32,511,2048,8192})for(bool inPlace:{false,true}){passthrough<float>(inPlace,channels,rate,block);passthrough<double>(inPlace,channels,rate,block);}
  std::cout<<"PASS: 200 processor configurations, float/double, mono/stereo, in-place/separate, bypass/state, silence and flush\n";
  for(double rate:{22050.,44100.,48000.,96000.})for(int band=0;band<5;++band){
    Meme::Analyzer analyzer;analyzer.prepare(rate);
    const double frequencies[]={55,250,1000,3500,9000};const int n=int(rate*2);
    std::vector<double> left(n),right(n);
    for(int s=0;s<n;++s){left[s]=0.5*std::sin(2*3.141592653589793*frequencies[band]*s/rate);right[s]=-left[s];}
    const double* input[]={left.data(),right.data()};analyzer.feed(input,2,n);
    const auto levels=analyzer.levels();const int found=int(std::max_element(levels.begin(),levels.end())-levels.begin());
    require(found==band,"tone belongs to correct frequency band");require(levels[band]>0.33&&levels[band]<0.37,"calibrated RMS and anti-phase stereo");
    for(int other=0;other<5;++other)if(other!=band)require(levels[other]<0.015,"low band leakage");
    std::fill(left.begin(),left.end(),0);std::fill(right.begin(),right.end(),0);analyzer.feed(input,2,n);
    for(auto level:analyzer.levels())require(level<0.001,"silence release");
  }
  std::cout<<"PASS: 20 frequency/RMS/anti-phase and silence-release cases\n";
  Meme::Analyzer analyzer;analyzer.prepare(48000);
  std::vector<double> mixture(96000);
  for(size_t n=0;n<mixture.size();++n)for(double f:{55.,250.,1000.,3500.,9000.})mixture[n]+=0.1*std::sin(2*3.141592653589793*f*n/48000.);
  const double* signals[]={mixture.data()};analyzer.feed(signals,1,int(mixture.size()));
  for(auto level:analyzer.levels())require(level>0.065&&level<0.078,"all bands simultaneous");
  std::fill(mixture.begin(),mixture.end(),std::numeric_limits<double>::quiet_NaN());analyzer.feed(signals,1,int(mixture.size()));
  for(auto level:analyzer.levels())require(std::isfinite(level),"non-finite graphics protection");
  std::cout<<"PASS: simultaneous five-tone signal and non-finite input protection\n";
  auto processor=new Meme::Processor;require(processor->initialize(nullptr)==kResultOk,"meter processor initialize");
  ProcessSetup setup{};setup.sampleRate=48000;setup.maxSamplesPerBlock=4096;setup.symbolicSampleSize=kSample64;
  require(processor->setupProcessing(setup)==kResultOk,"meter setup");processor->setActive(true);
  std::array<double,4096> audio{},output{};
  for(int n=0;n<4096;++n)for(double f:{55.,250.,1000.,3500.,9000.})audio[n]+=0.1*std::sin(2*3.141592653589793*f*n/48000.);
  double* source[]={audio.data(),audio.data()};double* sink[]={output.data(),output.data()};
  AudioBusBuffers inputBus{},outputBus{};inputBus.numChannels=outputBus.numChannels=2;inputBus.channelBuffers64=source;outputBus.channelBuffers64=sink;
  ParameterChanges meters(5),changes(1);ProcessData data{};data.symbolicSampleSize=kSample64;data.numSamples=4096;data.numInputs=data.numOutputs=1;data.inputs=&inputBus;data.outputs=&outputBus;data.outputParameterChanges=&meters;
  require(processor->process(data)==kResultOk,"meter audio processing");require(meters.getParameterCount()==5,"five host output meters");
  for(int band=0;band<5;++band){auto* q=meters.getParameterData(band);int32 offset=0;ParamValue value=0;require(q->getParameterId()==Meme::meterBase+band&&q->getPoint(0,offset,value)==kResultOk&&value>0&&value<=1&&offset==4095,"host meter value and offset");}
  int32 qi=0,pi=0;changes.addParameterData(Meme::bypassId,qi)->addPoint(0,1,pi);data.inputParameterChanges=&changes;meters.clearQueue();
  require(processor->process(data)==kResultOk,"meter bypass processing");
  for(int band=0;band<5;++band){int32 offset=0;ParamValue value=1;require(meters.getParameterData(band)->getPoint(0,offset,value)==kResultOk&&value==0,"bypass hides all meters");}
  require(std::memcmp(audio.data(),output.data(),sizeof(audio))==0,"meter output preserves sound");
  processor->setActive(false);processor->terminate();processor->release();
  std::cout<<"PASS: five actual host output meter queues, bypass suppression and unchanged sound\n";
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
  std::cout<<"ALL AUDIO TESTS PASSED in "<<seconds<<" seconds\n";
}
