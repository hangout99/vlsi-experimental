#include "HDesign.h"
#include "Parser.h"
#include "Utils.h"
#include <math.h>
#include <string>

string IntToString(int intValue)
{ 
  char myBuff[100]; 
  string strRetVal; 

  // Set it to empty 
  memset(myBuff, '\0', 100); 

  // Convert to string 
  _itoa(intValue, myBuff, 10); 

  // Copy the buffer into the string object 
  strRetVal = myBuff; 

  return(strRetVal); 
}

void ReportTNSWNSSequence(HDesign& hd, string &tnsStr, string &wnsStr)
{
  char tmpFloat[32];
  sprintf(tmpFloat, "%f\t", Utils::TNS(hd));
  tnsStr += tmpFloat;
  WRITELINE(("%s", tnsStr.c_str()));

  sprintf(tmpFloat, "%f\t", Utils::WNS(hd));
  wnsStr += tmpFloat;
  WRITELINE(("%s", wnsStr.c_str()));
}

double D(double s, double T, double beta)
{
  if (s < 0)
  {
    return pow(1 - s / T, beta);
  } 
  else
  {
    return 1.0;
  }
}

double FindMaxPathDelay(HDesign& hd)
{
  double maxPathDelay = 0.0;

  for (HCriticalPaths::EnumeratorW i = hd.CriticalPaths.GetEnumeratorW(); i.MoveNext();)
  {
    HTimingPointWrapper tp = hd[hd[i.EndPoint()].TimingPoint()];

    if (maxPathDelay < tp.FallArrivalTime())
    {
      maxPathDelay = tp.FallArrivalTime();
    }
    if (maxPathDelay < tp.RiseArrivalTime())
    {
      maxPathDelay = tp.RiseArrivalTime();
    }

   /* if (maxPathDelay < fabs(i.Criticality()))
    {
      maxPathDelay = fabs(i.Criticality());
    }*/
  }

  return maxPathDelay;
}

void ComputeNetWeights(HDesign& hd)
{
  double maxPathDelay = FindMaxPathDelay(hd);
  double u = 0.3;
  double beta = 2;
  double T = (1 - u) * maxPathDelay;
  double sum = 0.0;

  for (HCriticalPaths::EnumeratorW critPathEnumW = hd.CriticalPaths.GetEnumeratorW(); critPathEnumW.MoveNext();)
  {
    for (HCriticalPath::PointsEnumeratorW pointsEnumW = critPathEnumW.GetEnumeratorW(); pointsEnumW.MoveNext();)
    {
      HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];
      HNetWrapper netW = hd[pin.Net()];
      HTimingPointWrapper tp = hd[hd[critPathEnumW.EndPoint()].TimingPoint()];

      //netWeight = netW.Weight();
      hd.Set<HNet::Weight>(netW, netW.Weight() + 0.5 * (D(T - tp.FallArrivalTime(), T, beta) - 1));
      hd.Set<HNet::Weight>(netW, netW.Weight() + 0.5 * (D(T - tp.RiseArrivalTime(), T, beta) - 1));
      //WRITELINE("%f", netW.Weight());
      pointsEnumW.MoveNext();
    }
    //WRITELINE("\n");
  }
}

void ExportNetWeights(HDesign& hd, const char* fileName)
{
  FILE *netWeightsFile;
  char currString[128];

  netWeightsFile = fopen(fileName, "w");
  if (netWeightsFile)
  {
    ALERTFORMAT(("Exporting net-weights to %s", fileName));

    for (HNets::NetsEnumeratorW netW = hd.Nets.GetFullEnumeratorW(); netW.MoveNext(); )
    {
      sprintf(currString, "%s\t%f\n", netW.Name().c_str(), netW.Weight());
      fputs(currString, netWeightsFile);
    }

    fclose(netWeightsFile);
  }
}

void ImportNetWeights(HDesign& hd, const char* fileName)
{
  FILE *netWeightsFile;
  char currString[32];
  char netName[32];
  float tempVal;
  int i = 0;
  int nNetsEnd = hd._Design.NetList.nNetsEnd;

  netWeightsFile = fopen(fileName, "r");
  if (netWeightsFile)
  {
    ALERTFORMAT(("Importing net-weights from %s", fileName));

    for (i = 1; i < nNetsEnd; ++i)
    {
      fgets(currString, 32, netWeightsFile);
      sscanf(currString, "%s\t%f\n", netName, &tempVal);
      hd._Design.NetList.netWeight[i] = tempVal;
    }

    fclose(netWeightsFile);
  }
  else
    ALERTFORMAT(("Error opening import net-weights file %s", fileName));
}

int GetnIter(const char* nwtsFileName)
{
  char tmp[2];

  if (nwtsFileName == "")
  {
    tmp[0] = '0';
  } 
  else
  {
    // We assume that we make not more than 9 iterations
    tmp[0] = nwtsFileName[strlen(nwtsFileName) - 6];
  }
  tmp[1] = 0;
  
  return atoi(tmp);
}

void GetNewCommandLine(string& newCMD, const string& nwtsFileName, int argc, char** argv)
{
  int flagNW  = 1;

  newCMD = "";
  for(int i = 0; i < argc; i++)
  {
    std::string name(argv[i]);
    size_t pos = name.find('=', 2);

    if (name.substr(2, pos - 2) == "NetWeighting.netWeightsImportFileName")
    {
      name = "--NetWeighting.netWeightsImportFileName=" + nwtsFileName;
      flagNW = 0;
    }
    newCMD += name + " ";
  }

  if (argc == 1)
  {
    newCMD += "default.cfg ";
  }
  if (flagNW)
  {
    newCMD += "--NetWeighting.netWeightsImportFileName=" + nwtsFileName;
  }
  printf("new command line is %s\n", newCMD.c_str());
}

void PrepareNextNetWeightingLoop(HDesign& hd, int& nCyclesCounter)
{
  //int nIter;  // number of current iteration
  int nLoops = hd.cfg.ValueOf("DesignFlow.nMacroIterations", 10);
  string nwtsFileName;
  string defFileName;
  static string tnsStr = "";
  static string wnsStr = "";

  if (nLoops > 9)
  {
    ALERT("The number of net weights iterations must be less than 10");
    //return;
  }

  nCyclesCounter = GetnIter(hd.cfg.ValueOf("NetWeighting.netWeightsImportFileName", ""));
  ALERTFORMAT(("Current iteration of net weighting is %d", nCyclesCounter));

  nwtsFileName = hd.Circuit.Name() + "_" + IntToString(nCyclesCounter+1) + ".nwts";
  defFileName  = hd.Circuit.Name() + "_" + IntToString(nCyclesCounter) + ".def";
  ComputeNetWeights(hd);
  ExportNetWeights(hd, nwtsFileName.c_str());
  ExportDEF(hd, defFileName);
  ReportTNSWNSSequence(hd, tnsStr, wnsStr);

  if (nCyclesCounter+1 == nLoops)
  {
    ALERTFORMAT(("The specified number of net weights iterations (%d) is performed", nLoops));
    return;
  }

  hd.cfg.SetCfgValue("NetWeighting.netWeightsImportFileName", nwtsFileName); 
}