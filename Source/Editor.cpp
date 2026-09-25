#include "Editor.h"
#include "Controller.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cvstguitimer.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
using namespace VSTGUI;
namespace Meme {
namespace {
const CColor background{12,14,22,255}, panel{21,24,35,255}, line{39,45,62,255}, text{239,242,249,255}, muted{136,148,173,255}, mint{149,247,217,255};
const std::array<CColor,5> colors{{{178,121,80,255},{247,86,104,255},{254,211,94,255},{240,241,251,255},{151,207,241,180}}};
const char* names[] = {"CYLINDER", "FLOW", "ARC", "DROPS", "PARTICLES"};
const char* ranges[] = {"< 100 Hz", "100 - 500 Hz", "500 - 2k Hz", "2k - 6k Hz", "6k - 20k Hz"};
void label(CDrawContext* context, const char* content, CRect rect, double size, CColor color, CHoriTxtAlign alignment=kLeftText) {
  context->setFont(kNormalFont, size); context->setFontColor(color); context->drawString(content, rect, alignment);
}
double fract(double x) { return x - std::floor(x); }
void dot(CDrawContext* c,double x,double y,double r,CColor color,bool fill=true) {
  c->setFillColor(color); c->setFrameColor(color); c->setLineWidth(1.3); c->drawEllipse({x-r,y-r,x+r,y+r},fill?kDrawFilled:kDrawStroked);
}
}
class VisualView final : public CView {
public:
  explicit VisualView(Controller* owner) : CView({0,0,960,600}), owner_(owner), started_(std::chrono::steady_clock::now()) {
    owner_->requestMeterSnapshot();
    timer_ = owned(new CVSTGUITimer([this](CVSTGUITimer*) { owner_->requestMeterSnapshot(); invalid(); }, 33));
    setMouseEnabled(false);
  }
  ~VisualView() override { timer_ = nullptr; }
  void draw(CDrawContext* c) override {
    const auto now=std::chrono::steady_clock::now();
    const double seconds=std::chrono::duration<double>(now-started_).count();
    c->setDrawMode(kAntiAliasing);
    c->setFillColor(background); c->drawRect(getViewSize(),kDrawFilled);
    label(c,"LETEREL  /  EXPERIMENTAL AUDIO LAB",{30,18,670,43},11,muted);
    label(c,"VaginaPlugin",{28,44,700,97},36,text);
    label(c,"FIVE BANDS. ZERO SOUND CHANGES.",{30,100,690,125},12,muted);
    c->setFillColor({24,47,44,255}); c->drawRect({754,48,930,80},kDrawFilled);
    label(c,"AUDIO PASSTHROUGH",{763,50,921,78},11,mint,kCenterText);
    double strongest=0;
    for(int band=0;band<5;++band) {
      double value=owner_->meter(band);
      // UI interpolation is deliberately separate from the audio callback.
      displayed_[band]+=(value-displayed_[band])*0.25;
      strongest=std::max(strongest,value);
      lane(c,band,displayed_[band],seconds);
    }
    c->setFrameColor(line); c->setLineWidth(1); c->drawLine({30,539},{930,539});
    dot(c,36,566,3,strongest>0.01?mint:muted);
    label(c,strongest>0.01?"LISTENING TO YOUR AUDIO":"WAITING FOR AUDIO",{48,551,490,580},11,strongest>0.01?mint:muted);
    label(c,"VST3  /  0.1.1  /  ABSTRACT VISUALIZER",{500,551,930,580},10,muted,kRightText);
    setDirty(false);
  }
private:
  void lane(CDrawContext* c,int band,double meter,double t) {
    const double x=30+band*182, cx=x+86;
    c->setFillColor(panel); c->drawRect({x,151,x+172,517},kDrawFilled);
    label(c,names[band],{x+14,166,x+157,192},12,text);
    label(c,ranges[band],{x+14,193,x+157,214},11,muted);
    c->setFrameColor(line); c->setLineWidth(1);
    for(int row=0;row<7;++row)c->drawLine({x+13,239.0+row*29},{x+159,239.0+row*29});
    const double energy=std::clamp((meter-0.12)/0.88,0.0,1.0);
    const CColor color=colors[band];
    // All shapes occupy separate abstract instrument cells; there is no anatomy.
    if(energy>0.003) {
      if(band==0) {
        const double phase=fract(t*(0.6+1.6*energy));
        const double y=261+phase*127, r=15+energy*17, h=25+energy*45;
        c->setFillColor(color);c->drawRect({cx-r,y,cx+r,y+h},kDrawFilled);
        dot(c,cx,y+h,r,color);
        c->setFillColor({216,162,112,255});c->drawEllipse({cx-r,y-r*0.38,cx+r,y+r*0.38},kDrawFilled);
        c->setFrameColor({234,183,133,255});c->setLineWidth(2);c->drawLine({cx-r+5,y+8},{cx-r+5,y+h-7});
      } else if(band==1) {
        const double width=2+energy*10, wave=14*std::sin(t*2);
        auto path=owned(c->createGraphicsPath());path->beginSubpath(cx-width,250);
        path->addBezierCurve(cx-width-14,300,cx-width+wave,355,cx-width+7,414);
        path->addLine(cx+width+7,414);path->addBezierCurve(cx+width+wave,355,cx+width-14,300,cx+width,250);path->closeSubpath();
        c->setFillColor(color);c->drawGraphicsPath(path);
        for(int i=0;i<5;++i)dot(c,cx-18+i*9,415+3*std::sin(t*3+i),7+energy*5,color);
      } else if(band==2) {
        auto path=owned(c->createGraphicsPath());path->beginSubpath(x+29,375);
        path->addBezierCurve(x+33,204+55*(1-energy),x+133,220,x+145,410);
        c->setFrameColor(color);c->setLineWidth(2+energy*8);c->drawGraphicsPath(path,CDrawContext::kPathStroked);
        for(int i=0;i<4;++i){const double p=fract(t*0.65+i*0.25);dot(c,x+28+118*p,375-390*p+423*p*p,2+energy*2,color);}
      } else if(band==3) {
        // Sparse droplets: a high threshold and quiet gaps between releases.
        if(energy>0.40)for(int i=0;i<2;++i){const double p=fract(t*0.35+i*0.49);if(p<0.64){double y=257+235*p*p;double xx=cx+22*std::sin(i*3+1);dot(c,xx,y,4+energy*5,color);c->setFrameColor(color);c->setLineWidth(2);c->drawLine({xx,y-13},{xx,y-3});}}
      } else {
        for(int i=0;i<25;++i){const double p=fract(t*(0.26+energy*0.55)+i*0.618);const double angle=i*2.399;
          const double radius=p*(36+energy*44);double xx=cx+std::cos(angle)*radius, yy=328+std::sin(angle)*radius;
          CColor transparent=color;transparent.alpha=uint8_t(200*(1-p));dot(c,xx,yy,1.5+3*(1-p),transparent,false);
        }
        dot(c,cx,328,4,color,false);
      }
    }
    c->setFillColor({38,44,59,255});c->drawRect({x+14,456,x+158,460},kDrawFilled);
    c->setFillColor(color);c->drawRect({x+14,456,x+14+144*meter,460},kDrawFilled);
    char db[32]; if(meter<0.001)std::snprintf(db,sizeof(db),"SILENT");else std::snprintf(db,sizeof(db),"%.1f dBFS",meter*60-60);
    label(c,db,{x+14,476,x+158,499},11,color);
  }
  Controller* owner_;
  SharedPointer<CVSTGUITimer> timer_;
  std::chrono::steady_clock::time_point started_;
  std::array<double,5> displayed_{};
};
Editor::Editor(Controller* controller) : VSTGUIEditor(controller), owner_(controller) {
  rect={0,0,960,600};
}
VSTGUI::CView* createVisualView(Controller* controller){return new VisualView(controller);}
bool PLUGIN_API Editor::open(void* parent,const PlatformType& platform) {
  if(frame) return false;
  frame=new CFrame({0,0,960,600},this);
  if(!frame->open(parent,platform)){frame->forget();frame=nullptr;return false;}
  frame->addView(createVisualView(owner_));return true;
}
// CFrame::close releases its own final reference. The SDK also reads this frame.
void PLUGIN_API Editor::close(){if(frame){auto* closing=frame;frame=nullptr;closing->close();}}
Editor::~Editor(){close();}
}
