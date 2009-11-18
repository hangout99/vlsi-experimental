#ifndef __PLACEMENT_QUALITY_ANALYZER__
#define __PLACEMENT_QUALITY_ANALYZER__

#include "HDesign.h"
#include "HDPGrid.h"
#include "PlacementStorage.h"

class PlacementQualityAnalyzer
{
public:
  PlacementQualityAnalyzer(HDesign& design);
  ~PlacementQualityAnalyzer();

  void AnalyzeQuality(int id);
  void Report();

  void SaveCurrentPlacementAsBestAchieved();
  void RestoreBestAchievedPlacement();

  bool IsAcceptableImprovementAchieved();
  bool IsNextIterationApproved();

private:
  struct PlacementQuality
  {
    int id;
    double hpwl;
    double hpwl_legalized;
    double tns;
    double tns_legalized;
    double wns;
    double wns_legalized;
  };
  typedef TemplateTypes<PlacementQuality>::list QualityList;

  HDPGrid* m_grid;
  HDesign& m_design;
  QualityList m_experiments;
  int nIterationsWithoutGain;
  double m_BestPlacementHPWLLegalized;
  
  PlacementStorage m_Placement; //NOTE: made class member to minimize memory allocations count
  PlacementStorage m_BestPlacement;

  double LastIterImprovement();
};

#endif //__PLACEMENT_QUALITY_ANALYZER__