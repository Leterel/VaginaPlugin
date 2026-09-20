#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include "Editor.h"
#include "Controller.h"
#include "vstgui/lib/coffscreencontext.h"
#include "vstgui/lib/platform/platformfactory.h"
#include <fstream>
#include <iostream>

void* moduleHandle=nullptr;
extern bool InitModule();
extern bool DeinitModule();
int main() {
  CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
  moduleHandle=GetModuleHandleW(nullptr);
  if(!InitModule())return 1;
  auto* controller=new Meme::Controller;
  if(controller->initialize(nullptr)!=Steinberg::kResultOk)return 1;
  {
    auto view=VSTGUI::owned(Meme::createVisualView(controller));
    auto context=VSTGUI::COffscreenContext::create({960,600});
    if(!context)return 1;
    for(int active=0;active<2;++active){
      for(int band=0;band<5;++band)controller->setParamNormalized(Meme::meterBase+band,active?0.87:0.0);
      for(int frame=0;frame<40;++frame){context->beginDraw();view->draw(context);context->endDraw();}
      auto png=VSTGUI::getPlatformFactory().createBitmapMemoryPNGRepresentation(context->getBitmap()->getPlatformBitmap());
      if(png.empty())return 1;
      std::ofstream file(active?"preview-active.png":"preview-silent.png",std::ios::binary);
      file.write(reinterpret_cast<const char*>(png.data()),png.size());
      if(!file)return 1;
    }
  }
  controller->terminate();controller->release();
  DeinitModule();CoUninitialize();
  std::cout<<"PASS: actual VSTGUI drawing rendered silent/active PNGs offscreen\n";
}
