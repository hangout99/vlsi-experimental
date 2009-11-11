#include "Reporting.h"
#include "Utils.h"

void ReportNetPins(HDesign& design, HNet net)
{
  WRITELINE("Net %s details:", design.GetString<HNet::Name>(net).c_str());
  for (HNet::PinsEnumeratorW pin = design.Get<HNet::Pins, HNet::PinsEnumeratorW>(net); pin.MoveNext(); )
  {
    WRITELINE("  %s(%s) %s %s",
      design.GetString<HCell::Name>(pin.Cell()).c_str(),
      design.GetString<HMacroType::Name>(design.Get<HCell::MacroType, HMacroType>(pin.Cell())).c_str(),
      pin.Name().c_str(),
      Utils::GetPinDirectionStr(pin.Direction())
      );
  }
}

void ReportNetPinsCoordinates(HDesign& design, HNet net)
{
  WRITELINE("Net %s pins:", design.GetString<HNet::Name>(net).c_str());
  for (HNet::PinsEnumeratorW pin = design.Get<HNet::Pins, HNet::PinsEnumeratorW>(net); pin.MoveNext(); )
  {
    WRITELINE("  %s\tx = %f\ty = %f",
      pin.Name().c_str(),
      design.GetDouble<HPin::X>(pin),
      design.GetDouble<HPin::Y>(pin)
      );
  }
}

void ReportNetsInfo(HDesign& design) 
{
  int maxPinCount = 0;
  for (HNets::NetsEnumeratorW nIter = design.Nets.GetFullEnumeratorW(); nIter.MoveNext(); )
  {
    if (nIter.PinsCount() > maxPinCount)
      maxPinCount = nIter.PinsCount();
  }

  int *nNetsWithPins = new int [maxPinCount + 1];

  for (int j = 0; j < maxPinCount + 1; j++)
  {

    nNetsWithPins[j] = 0;
  }

  int index = 0;

  for (HNets::NetsEnumeratorW nIter = design.Nets.GetFullEnumeratorW(); nIter.MoveNext(); )
  {
    nNetsWithPins[nIter.PinsCount()]++;
  }

  WRITELINE("");
  WRITELINE("Reporting: Number pins in nets\n");
  for (int i = 0; i < maxPinCount + 1; i++)
    if (nNetsWithPins[i] != 0)
      WRITELINE("  Number of nets with\t%d\tpin =\t%d", i, nNetsWithPins[i]);

   delete [] nNetsWithPins;
}

void ReportCountNetsWithTypes(HDesign& design)
{
  WRITELINE("");
  WRITELINE("Reporting: count nets with type : all, skipped, buffered, removed, Active\n");
  WRITELINE("  Nets count all = %d", design.Nets.Count());
  WRITELINE("  Nets with active type   = %d", design.Nets.Count(NetKind_Active));
  WRITELINE("  Nets with skipped type  = %d", design.Nets.Count(NetKind_Skipped));
  WRITELINE("  Nets with buffered type = %d", design.Nets.Count(NetKind_Buffered));
  WRITELINE("  Nets with removed type  = %d", design.Nets.Count(NetKind_Removed));
}

void ReportNetTiming(HDesign& design, HNet net)
{
  HNetWrapper netW = design[net]; 
  double RAT, AAT;
  string pinName;
  ALERTFORMAT(("Reporting net %s timing", netW.Name().c_str()));
  
  for (HNetWrapper::PinsEnumeratorW currPin = netW.GetPinsEnumeratorW(); currPin.MoveNext();)
  {
    HTimingPointWrapper tp = design[design.TimingPoints[currPin]];
    RAT = tp.RequiredTime();
    AAT = tp.ArrivalTime();

    if (currPin.IsPrimary())
      pinName = currPin.Name();
    else
      pinName = design.Cells.GetString<HCell::Name>(currPin.Cell()) + "." + currPin.Name();

    ALERTFORMAT(("RAT at %s\t= %.10f", pinName.c_str(), RAT));
    ALERTFORMAT(("AAT at %s\t= %.10f", pinName.c_str(), AAT));
  }
}