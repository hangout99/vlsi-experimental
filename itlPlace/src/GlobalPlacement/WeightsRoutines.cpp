#include "ObjectivesConstraints.h"
#include "WeightsRoutines.h"
#include "Clustering.h"
#include "Timing.h"

void InitWeights(double* x, AppCtx* context)
{
    context->weights.lseW = 1.0;
    context->weights.sodW = 1.0;
    context->weights.lrW  = 1.0;
    context->weights.sprW = 1.0;

    TotalCostAndGradients(context, x);

    double lseInitialRatio = context->hd->cfg.ValueOf("GlobalPlacement.Weights.lseInitialRatio", 1.0);
    double sprInitialRatio = context->hd->cfg.ValueOf("GlobalPlacement.Weights.sprInitialRatio", 1.0);
    double lrInitialRatio = context->hd->cfg.ValueOf("GlobalPlacement.Weights.lrInitialRatio", 1.0);

    double bigValue = 1.0e+6;

    if (context->useLogSumExp)
        context->weights.lseW = bigValue * lseInitialRatio / context->criteriaValues.lse;
    else
        context->weights.lseW = 0.0;

    if (context->useSpreading)
        context->weights.sprW = context->sprData.sprWInitial = bigValue * sprInitialRatio / context->criteriaValues.spr;
    else
        context->weights.sprW = context->sprData.sprWInitial = 0.0;
    
    if (context->useLR)
        context->weights.lrW = bigValue * lrInitialRatio / context->criteriaValues.lr;
    else
        context->weights.lrW = 0.0;

    ReportWeights(*context);
}

void ApplyWeights(AppCtx* context)
{
    context->criteriaValues.lse *= context->weights.lseW;
    context->criteriaValues.sod *= context->weights.sodW;
    context->criteriaValues.lr  *= context->weights.lrW;
    context->criteriaValues.spr *= context->weights.sprW;

    for (int i = 0; i < context->nVariables; i++)
    {
        context->criteriaValues.gLSE[i] *= context->weights.lseW;
        context->criteriaValues.gSOD[i] *= context->weights.sodW;
        context->criteriaValues.gLR[i]  *= context->weights.lrW;
        context->criteriaValues.gQS[i]  *= context->weights.sprW;
    }
}

double GetMultiplierAccordingDesiredRatio(double ratio, double desiredRatio, double multiplier)
{
    if (ratio > desiredRatio)
        return multiplier;
    else
        return 1.0/multiplier;
}
double GetMultiplierAccordingDesiredRatio(HDesign& hd, double deltaRatio, int updateFunction)
{
	double multiplier = 1.0;
	
	double N = hd.cfg.ValueOf("GlobalPlacement.Weights.N", 1.0);
	double R = hd.cfg.ValueOf("GlobalPlacement.Weights.R", 1.1);
	double A = hd.cfg.ValueOf("GlobalPlacement.Weights.A", 1.0);
	double B = hd.cfg.ValueOf("GlobalPlacement.Weights.B", 1.0);
	double C = hd.cfg.ValueOf("GlobalPlacement.Weights.C", 1.0);
	
	if (updateFunction == 0)
	{
		double p = N;
		int s = deltaRatio > 0 ? 1 : (deltaRatio < 0) ? -1 : 0;
		double f = fabs(deltaRatio);
		multiplier = 1 + C*s*pow(f, p);
	}
	else if (updateFunction == 1)
		multiplier = 1 + A*atan(B*deltaRatio);

	else if (updateFunction == 2)
		multiplier = pow(R, deltaRatio);

	else if (updateFunction == 3)
		multiplier = pow(1 + deltaRatio, N);

	return multiplier;
}

double ChooseSpreadingMultiplier(HDesign& hd, double currentHPWL, double currentLHPWL)
{
    double sprRatio = currentLHPWL / currentHPWL;
    double sprDesiredRatio = hd.cfg.ValueOf("GlobalPlacement.Weights.sprDesiredRatio", 1.1);
	
	bool useSprUpdateFunction = hd.cfg.ValueOf("GlobalPlacement.Weights.useSprUpdateFunction", false);
	
	if (useSprUpdateFunction)
	{
		int sprUpdateFunction = hd.cfg.ValueOf("GlobalPlacement.Weights.sprUpdateFunction", 2);
		double deltaRatio = sprRatio - sprDesiredRatio;
	    return GetMultiplierAccordingDesiredRatio(hd, deltaRatio, sprUpdateFunction);
	}
	else
	{
	    double sprUpdateMultiplier = hd.cfg.ValueOf("GlobalPlacement.Weights.sprUpdateMultiplier", 2.0);
        return GetMultiplierAccordingDesiredRatio(sprRatio, sprDesiredRatio, sprUpdateMultiplier);
    }
}

double ChooseLSEMultiplier(HDesign& hd, double currentLHPWL, double initialLHPWL)
{
    double lseRatio = currentLHPWL / initialLHPWL;
    double lseDesiredRatio = hd.cfg.ValueOf("GlobalPlacement.Weights.lseDesiredRatio", 1.0);
    double lseUpdateMultiplier = hd.cfg.ValueOf("GlobalPlacement.Weights.lseUpdateMultiplier", 1.0);
	
	bool useLseUpdateFunction = hd.cfg.ValueOf("GlobalPlacement.Weights.useLseUpdateFunction", false);
	
	if (useLseUpdateFunction)
	{
		int lseUpdateFunction = hd.cfg.ValueOf("GlobalPlacement.Weights.lseUpdateFunction", 0);
		double deltaRatio = lseRatio - lseDesiredRatio;
		return GetMultiplierAccordingDesiredRatio(hd, deltaRatio, lseUpdateFunction);
	}

    return GetMultiplierAccordingDesiredRatio(lseRatio, lseDesiredRatio, lseUpdateMultiplier);
}

void UpdateWeights(HDesign& hd, AppCtx& context, PlacementQualityAnalyzer* QA, ClusteringInformation& ci)
{
    string objective = hd.cfg.ValueOf("params.objective");

    double initialLHPWL = QA->GetInitialMetricValue(PlacementQualityAnalyzer::QualityMetrics::MetricHPWLleg);
    double currentLHPWL = QA->GetCurrentMetricValue(PlacementQualityAnalyzer::QualityMetrics::MetricHPWLleg);
    double currentHPWL = QA->GetCurrentMetricValue(PlacementQualityAnalyzer::QualityMetrics::MetricHPWL);
    
    context.weights.sprW *= ChooseSpreadingMultiplier(hd, currentHPWL, currentLHPWL);

    if (objective == "LR")
    {
        context.LRdata.UpdateMultipliers(hd);        
        context.weights.lseW *= ChooseLSEMultiplier(hd, currentLHPWL, initialLHPWL);
    }

    if (hd.cfg.ValueOf("LSE.Clustering.useNetWeights", false))
    {
      ComputeAndExportWeights(hd); //do weighting
      //WriteWeightsToClusteredNets(hd, ci);
      //int netIdx = 0;

      for (int i = 0; i < static_cast<int>(ci.netList.size()); i++)
      {
        AssignWeightForClusteredNet(hd, ci, i);
      }
    }
}

void ReportWeights(AppCtx& context) 
{
    ALERT("Cost terms weights:");
    ALERT("  lseW = " + Aux::SciFormat, context.weights.lseW);
    ALERT("  lrW  = " + Aux::SciFormat, context.weights.lrW);
    ALERT("  sprW = " + Aux::SciFormat, context.weights.sprW);
}