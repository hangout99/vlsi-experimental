#include "TimingPointMus.h"

int CalcNumberOfLRArc(HDesign& design, HNet net)
{
  HPin source = design[net].Source();
  HPinType sourceType = design.Get<HPin::Type, HPinType>(source);

  int i = 0;
  for (HPinType::ArcsEnumeratorW arc = design[sourceType].GetArcsEnumeratorW(); arc.MoveNext(); )
    if (arc.TimingType() == TimingType_Combinational)
      i++;

  return i;
}

//FIXME: check if we need to find topological order by ourselves
TimingPointMus::TimingPointMus(HDesign& design): MuS(0), MuIn(0)
{
  size = design._Design.NetList.nPinsLimit;
  ::Grow(&MuS, 0, size);
  ::Grow(&MuIn, 0, size);

  for (HTimingPointWrapper sp = design[design.TimingPoints.TopologicalOrderRoot()]; !::IsNull(sp.GoNext()); )
    InitPoint(design, sp);
}

TimingPointMus::~TimingPointMus()
{
  ::Grow(&MuS, size, 0);
  ::Grow(&MuIn, size, 0);
}

void TimingPointMus::ReportMus(HDesign& design)
{
  for (HTimingPointWrapper pt = design[design.TimingPoints.TopologicalOrderRoot()];
    !::IsNull(pt.GoNext()); )
  {
    WRITELINE("MuS %f", GetMuS(pt));
    int count = GetMuInCount(pt);
    for (int i = 0; i < count; i++)
      WRITELINE("MuInA %f    MuInR %f", GetMuInA(pt, i), GetMuInR(pt, i));
  }
}

void TimingPointMus::GetMuIn( HDesign& design, HTimingPoint pt, std::vector<double>& inMus)
{
  inMus.clear();
  for (int i = 0; i < GetMuInCount(pt); i++)
  {
    inMus.push_back(GetMuInA(pt, i) + GetMuInR(pt, i));
  }
}

void TimingPointMus::GetMuOut(HDesign& design, HTimingPoint pt, std::vector<double>& muOutVector)
{
  GetMuOutVector muOut(muOutVector);
  IterateOutMu(design, pt, muOut);
}

double TimingPointMus::SumOutMuA(HDesign& design, HTimingPoint pt)
{
  AccumulateMu acc(true);
  IterateOutMu(design, pt, acc);
  return acc.sum;
}

double TimingPointMus::SumOutMuR(HDesign& design, HTimingPoint pt)
{
  AccumulateMu acc(false);
  IterateOutMu(design, pt, acc);
  return acc.sum;
}

void TimingPointMus::UpdateOutMuR(HDesign& design, HTimingPoint pt, double multiplier)
{
  IterateOutMu(design, pt, UpdateMuR(multiplier));
}

template<class Action>
void TimingPointMus::IterateOutMu(HDesign& design, HTimingPoint pt, Action& todo)
{
  HPin pin = design.Get<HTimingPoint::Pin, HPin>(pt);
  if (design.Get<HPin::Direction, PinDirection>(pin) == PinDirection_OUTPUT)
  {
    for (HNet::SinksEnumeratorW sink = design.Get<HNet::Sinks, HNet::SinksEnumeratorW>(design.Get<HPin::Net, HNet>(pin));
      sink.MoveNext();)
    {
      todo(*this, design.TimingPoints[sink], 0);
    }
  }
  else
  {
    HCell cell = design.Get<HPin::Cell, HCell>(pin);
    for(HCell::PinsEnumeratorW p = design.Get<HCell::Pins, HCell::PinsEnumeratorW>(cell); p.MoveNext(); )
    {
      if (p.Direction() != PinDirection_OUTPUT || ::IsNull(p.Net())) continue;
      HTimingPoint point = design.TimingPoints[p];
      int idx = 0;
      for (HPinType::ArcsEnumeratorW arc 
        = design.Get<HPinType::ArcTypesEnumerator, HPinType::ArcsEnumeratorW>(p.Type());
        arc.MoveNext(); idx++)
      {
        if (arc.GetStartPin(p) == pin)
          todo(*this, point, idx);
      }
    }
  }
}

void TimingPointMus::InitPoint(HDesign& design, HTimingPoint pt)
{
  double defaultMu = design.cfg.ValueOf("GlobalPlacement.LagrangianRelaxation.muLR", 0.99);

  SetMuS(pt, defaultMu);
  HPin pin = design.Get<HTimingPoint::Pin, HPin>(pt);

  if (design.Get<HPin::Direction, PinDirection>(pin) == PinDirection_INPUT)
  {
    MuIn[::ToID(pt)].resize(2, defaultMu);
  }
  else
  {
    if (!design.GetBool<HPin::IsPrimary>(pin))
      MuIn[::ToID(pt)].resize(2 * CalcNumberOfLRArc(design, design.Get<HPin::Net, HNet>(pin)), defaultMu);
  }
}

void TimingPointMus::UpdateMus(HDesign& design)
{
  //FIXME: create implementation
}

void TimingPointMus::GetNetMus(HDesign& design, HNet net, 
                               std::vector<double>& cellArcMus, 
                               std::vector<double>& netArcMus)
{
  HPin sourcePin = design[net].Source();
  HTimingPoint point = design.TimingPoints[sourcePin];

  GetMuIn(design, point, cellArcMus);
  GetMuOut(design, point, netArcMus);
}