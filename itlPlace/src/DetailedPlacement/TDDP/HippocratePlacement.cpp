#include <math.h>
#include <float.h>
#include "HippocratePlacement.h"
#include "Legalization.h"
#include "Parser.h"
#include <time.h>
#include "Timing.h"

bool CheckHippocrateConstraint(HDesign& design, HNet net, HPin sink)
{
  ASSERT(((sink,design).Net() == net));
  ASSERT(((sink,design).Direction() == PinDirection_INPUT));

  HPin source = (net,design).Source();

  double driverResistance = Utils::GetDriverTimingPhisics(design, source, SignalDirection_None).R;
  double hpwl = Utils::CalcNetHPWL(design, net);
  double l = abs((source,design).X() - (sink,design).X())
    + abs((source,design).Y() - (sink,design).Y());
  double Cs = Utils::GetSinkCapacitance(design, sink, SignalDirection_None);

  double sourceArrival = (design.TimingPoints[source],design).ArrivalTime();
  double sourceOldC = (design.SteinerPoints[source],design).ObservedC();

  double sinkArrival = DBL_MIN;

  for (HCell::PinsEnumeratorW pin = (design[sink].Cell(),design).GetPinsEnumeratorW();
    pin.MoveNext(); )
  {
    if (!::IsNull(pin.Cell()) && pin.Direction() == PinDirection_INPUT)
    {
      double AT = (design.TimingPoints[pin],design).ArrivalTime();
      if (AT > sinkArrival)
        sinkArrival = AT;
    }
  }

  double load = Utils::GetNetLoad(design, net, SignalDirection_None);
  
  return sourceArrival
    + driverResistance * (load + design.RoutingLayers.Physics.LinearC * hpwl - sourceOldC)
    + design.RoutingLayers.Physics.RPerDist * l * (Cs + 0.5 * l * design.RoutingLayers.Physics.LinearC)
    <= sinkArrival;
}

bool CheckHippocrateConstraint(HDesign& design, HCell cell)
{
  for (HCell::PinsEnumeratorW pin = (cell,design).GetPinsEnumeratorW(); pin.MoveNext(); )
    if (!::IsNull(pin))
    {
      HNetWrapper net = design[pin.Net()];
      for (HNet::SinksEnumeratorW sink = net.GetSinksEnumeratorW(); sink.MoveNext(); )
        if (!CheckHippocrateConstraint(design, net, sink))
          return false;
    }
  return true;
}


LWindow::LWindow() 
{
	int maxcount = (RADIUS_OF_WINDOW*2+1)*(RADIUS_OF_WINDOW*2+1);
	cells = new HCell[maxcount];
}		
LWindow::~LWindow() {
	delete [] cells;
}

void HippocratePlacementInit()
{
	printf("\nH I P P O C R A T E   I N I T\n");
}

void HippocratePlacementFinalize()
{
	printf("\nH I P P O C R A T E  D E L E T E\n");
}

void HippocratePlacementMOVE(HDPGrid& hdpp, HDesign& hd, HCell& curCell, LWindow& curWnd, StatisticsAnalyser& stat)
{

	Utils::CalculateHPWL(hd, true);
	std::vector<HNet> curNets;//����������, � ������� ��������� ������� �������
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}
	double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
	int k = 0;  //����� ��-�� �� ����, ��� �������� ������. ������������ ���������� WTWL
	int PosInGap=0;
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);
	std::vector<PseudoCell> VHole=getPseudoCellsFromWindow(hdpp, hd, curWnd);
	//ALERT("holes %d",VHole.size());

	for (unsigned int i = 0; i < VHole.size(); i++) 
	{		
		if(hdpp.CellSitesNum(curCell) <= VHole[i].GapSize) //������ ��� �������� �������
		{
			for(int counter=0;counter<=(VHole[i].GapSize-hdpp.CellSitesNum(curCell));counter++)
			{
				hdpp.PutCell(curCell, VHole[i].row,VHole[i].col+counter);

				//��� ������� ����������� ���������� ��������� ���������� �����������
				bool constraintsOK = true;
				double deltaWTWL = 0;
				for(int j = 0; j < curNets.size(); j++) {
					/*if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
						constraintsOK = false;
					*/
					if(!CheckHippocrateConstraint(hd, curCell)) constraintsOK = false;
				}

				if(constraintsOK) { //���� ��������� �����������
					//������� ��������� WTWL (deltaWTWL)
					deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
					if (deltaWTWL < maxIncrOfWTWL){
						maxIncrOfWTWL = deltaWTWL;
						k = i;
						PosInGap=counter;
					}
				}

				hdpp.PutCell(curCell, oldCurRow, oldCurCol);
				stat.incrementAttempts(MOVE);

			}
		}	
	}
	if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
		//������ ��-��
		hd.Plotter.SaveMilestoneImage("MOVE_B");
		hd.Plotter.PlotCell(curCell, Color_Brown);
		hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

		hdpp.PutCell(curCell, VHole[k].row,VHole[k].col+PosInGap);

		//ALERT("!!!k=%d pos=%d",k,PosInGap);
		hd.Plotter.PlotCell(curCell, Color_Brown);
		hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);	
		hd.Plotter.SaveMilestoneImage("MOVE_A");
		stat.incrementAttemptsSuccessful(MOVE);
		for(int j = 0; j < curNets.size(); j++) 
		{
			HPinWrapper source = hd[hd.Get<HNet::Source, HPin>(curNets[j])];
			if (source.Cell() == curCell) {ALERT("IS SOURCE!"); }
		}
	} 

}

void HippocratePlacementCOMPACT(HDPGrid& hdpp, HDesign& hd, HCell& curCell, BBox curBBox, StatisticsAnalyser& stat)
{
	double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
	int k = 0;  //����� ��-�� �� ����, ��� �������� ������. ������������ ���������� WTWL
	int PosInGap=0;
	int oldCurRow;
	int oldCurCol;
	std::vector<HCell> gotSpecialWindow;//
	std::vector<PseudoCell> VHole;

	for(std::vector<HCell>::iterator IterCellInNet=curBBox.cells.begin();IterCellInNet!=curBBox.cells.end();IterCellInNet++)
	{
		Utils::CalculateHPWL(hd, true);
		std::vector<HNet> curNets;//����������, � ������� ��������� ������� �������
		for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(*IterCellInNet); pin.MoveNext(); )
		{
			HNet net=hd.Get<HPin::Net,HNet>(pin);
			if(!::IsNull(net)) curNets.push_back(net);
		}
		maxIncrOfWTWL = 0;  //������������ ���������� WTWL
		k = 0;  //����� ��-�� �� ����, ��� �������� ������. ������������ ���������� WTWL
		PosInGap=0;
		oldCurRow = hdpp.CellRow(*IterCellInNet);
		oldCurCol = hdpp.CellColumn(*IterCellInNet);
		//gotSpecialWindow=getSpecialWindow(hdpp, hd, *IterCellInNet, curBBox);
		VHole.clear();
		//VHole = getPseudoCellsFromSpecWindow(hdpp, hd, gotSpecialWindow);	
		VHole=getGapsForCell(hdpp,hd,*IterCellInNet,curBBox);
		//unsigned int i = 0; if (VHole.size()<=0) continue;
		for(unsigned int i = 0; i < VHole.size(); i++) 
		{		
			if(hdpp.CellSitesNum(*IterCellInNet) <= VHole[i].GapSize) //������ ��� �������� �������
			{
				for(int counter=0;counter<=(VHole[i].GapSize-hdpp.CellSitesNum(*IterCellInNet));counter++)
				{
					hd.Plotter.PlotCell(*IterCellInNet, Color_GrayText);
					hdpp.PutCell(*IterCellInNet, VHole[i].row,VHole[i].col+counter);
					hd.Plotter.PlotCell(*IterCellInNet, Color_Black);

					//��� ������� ����������� ���������� ��������� ���������� �����������
					bool constraintsOK = true;
					double deltaWTWL = 0;
					/*for(int j = 0; j < curNets.count; j++) {
						if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, *IterCellInNet))
							constraintsOK = false;
					}*/
					if(!CheckHippocrateConstraint(hd, *IterCellInNet)) constraintsOK = false;

					if(constraintsOK) { //���� ��������� �����������
						//������� ��������� WTWL (deltaWTWL)
						deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
						if (deltaWTWL < maxIncrOfWTWL){
							maxIncrOfWTWL = deltaWTWL;
							k = i;
							PosInGap=counter;
						}
					}

					hdpp.PutCell(*IterCellInNet, oldCurRow, oldCurCol);
					stat.incrementAttempts(COMPACT);
				}
			}	
		}
		if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
			//������ ��-��
			//hd.Plotter.SaveMilestoneImage("CompactSuccess1");
			hd.Plotter.PlotCell(*IterCellInNet, Color_Brown);
			hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

			hdpp.PutCell(*IterCellInNet, VHole[k].row,VHole[k].col+PosInGap);

			//ALERT("!!!k=%d pos=%d",k,PosInGap);
			hd.Plotter.PlotCell(*IterCellInNet, Color_Red);
			hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);	
			hd.Plotter.SaveMilestoneImage("Compact_success2");
			stat.incrementAttemptsSuccessful(COMPACT);
		hd.Plotter.ShowPlacement();
		} 
	}	
}

/*
void HippocratePlacement::SECCOMPACT()
{
for(std::vector<HCell>::iterator IterCellInNet=curBBox.cells.begin();IterCellInNet!=curBBox.cells.end();IterCellInNet++)
{
Utils::CalculateHPWL(hd, true);
NetsVector curNets(*IterCellInNet, hd);  //����������, � ������� ��������� ������� �������
double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
int oldCurRow = hdpp.CellRow(*IterCellInNet);
int oldCurCol = hdpp.CellColumn(*IterCellInNet);
std::vector<HCell> gotSpecialWindow=getSpecialWindow(*IterCellInNet);
std::vector<PseudoCell> VHole;
VHole = getPseudoCellsFromSpecWindow(gotSpecialWindow);	

PossiblePos NewPos = GetCenterNearestPos(*IterCellInNet,VHole);

if (NewPos.col == -1) continue;

hdpp.PutCell(*IterCellInNet, NewPos.row, NewPos.col);

//��� ������� ����������� ���������� ��������� ���������� �����������
bool constraintsOK = true;
double deltaWTWL = 0;
for(int j = 0; j < curNets.count; j++) {
if(!ControlOfConstraints(curNets.net[j], oldCurRow, oldCurCol))
constraintsOK = false;
}

if(constraintsOK) { //���� ��������� �����������
//������� ��������� WTWL (deltaWTWL)
deltaWTWL = CalculateDiffWTWL(&curNets, false);
if (deltaWTWL < maxIncrOfWTWL){
maxIncrOfWTWL = deltaWTWL;
}
}

hdpp.PutCell(*IterCellInNet, oldCurRow, oldCurCol);

if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
//������ ��-��
hd.Plotter.SaveMilestoneImage("MOVE_B");
hd.Plotter.PlotCell(*IterCellInNet, Color_Brown);
hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

hdpp.PutCell(*IterCellInNet, NewPos.row, NewPos.col);

hd.Plotter.PlotCell(*IterCellInNet, Color_Red);
hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);	
hd.Plotter.SaveMilestoneImage("MOVE_A");
ALERT("!!!ELEMENTS MOVED (SECONDCOMPACT)!!!");
} 
}	
}	
*/

PossiblePos GetCenterNearestPos(HDPGrid& hdpp, HCell& NetCell, std::vector<PseudoCell>& VGap, BBox& curBBox)
{
	int targetRow=curBBox.rowCenter, targetCol=curBBox.colCenter;
	PossiblePos CentPos;
	CentPos.col=-1;
	CentPos.row=-1;
	int dist = -1;
	for(unsigned int i = 0; i < VGap.size(); i++) 
	{		
		if(hdpp.CellSitesNum(NetCell) <= VGap[i].GapSize)
		{
			for(int j=0; j<VGap[i].GapSize-hdpp.CellSitesNum(NetCell)+1;j++)
			{
				int temp = abs(VGap[i].col+j-targetCol)+abs(VGap[i].row-targetRow);
				if ((dist>temp)||(dist == -1)) 
				{
					dist=temp;
					CentPos.col=VGap[i].col+j;
					CentPos.row=VGap[i].row;
				}
			}
		}
	}
	return CentPos;
}

int GetLeftFreeSpace(HCell cell, HDPGrid& hdpp) {	
	int m = 1;
	int row = hdpp.CellRow(cell);
	int col = hdpp.CellColumn(cell);
	if (col >= 1) {

		//while ((col - m >= 0)&&(::IsNull(hdpp(row, col - m)))) {
		while((col - m >= 0)&&!isCellExists(hdpp, row, col-m)){
			m++;
		}
	}
	return m-1;
}

int GetRightFreeSpace(HCell cell, HDPGrid& hdpp) {						
	int row = hdpp.CellRow(cell);
	int col = hdpp.CellColumn(cell);
	int nSites = hdpp.CellSitesNum(cell);
	int m = 0;
	if (col + nSites < hdpp.NumCols()) {
		//while((col + nSites + m < hdpp.NumCols())&&::IsNull(hdpp(row, col + nSites + m))) {
		while(col + nSites + m < hdpp.NumCols() && !isCellExists(hdpp, row, col + nSites + m)){
				m++;
		}
	}
	return m;
}

void HippocratePlacementCENTER(HDPGrid& hdpp, HDesign& hd, HCell& curCell, LWindow& curWnd, StatisticsAnalyser& stat)
{
	Utils::CalculateHPWL(hd, true);
	std::vector<HNet> curNets;//����������, � ������� ��������� ������� �������
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}

	double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
	int k = 0;
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);

	hd.Plotter.PlotCell(curCell, Color_Black);
	hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

	std::vector<HCell> ConnectedCells; //��-��, ����������� � �������
	int connectedCellsCount = 0;   //�-�� ��-���, ����������� � �������

	ConnectedCells = GetCellsConnectedWithCurrent(hdpp, hd, curCell);
	connectedCellsCount = ConnectedCells.size();

	std::vector<HPinWrapper> primaryPins;
	for(int i=0; i<curNets.size(); i++) {
		for (HNet::PinsEnumeratorW pin = hd[curNets[i]].GetPinsEnumeratorW(); pin.MoveNext(); ) {
			if(pin.IsPrimary()) {
				primaryPins.push_back(pin);
			}
		}
	}
	//���������� �������. �������������
	double minX, minY, maxX, maxY;
	minX = maxX = hd.GetDouble<HCell::X>(ConnectedCells[0]);
	minY = maxY = hd.GetDouble<HCell::Y>(ConnectedCells[0]);

	for(int i=0; i<connectedCellsCount; i++) {
		if(hd.GetDouble<HCell::X>(ConnectedCells[i]) > maxX) 
			maxX = hd.GetDouble<HCell::X>(ConnectedCells[i]);
		if(hd.GetDouble<HCell::X>(ConnectedCells[i]) < minX) 
			minX = hd.GetDouble<HCell::X>(ConnectedCells[i]);

		if(hd.GetDouble<HCell::Y>(ConnectedCells[i]) > maxY) 
			maxY = hd.GetDouble<HCell::Y>(ConnectedCells[i]);
		if(hd.GetDouble<HCell::Y>(ConnectedCells[i]) < minY) 
			minY = hd.GetDouble<HCell::Y>(ConnectedCells[i]);			
	}
	if(primaryPins.size()) {
		for(unsigned int i=0; i<primaryPins.size(); i++) {
			if(maxX < (primaryPins[i].X())) 
				maxX = (primaryPins[i].X());
			if(maxY < (primaryPins[i].Y()))
				maxY = (primaryPins[i].Y());
			if(minX > (primaryPins[i].X())) 
				minX = (primaryPins[i].X());
			if(minY > (primaryPins[i].Y())) 
				minY = (primaryPins[i].Y());
		}
	}
	double centerX = (maxX + minX)/2;
	double centerY = (maxY + minY)/2;
	int centerCol = hdpp.FindColumn(centerX);
	int centerRow = hdpp.FindRow(centerY);

	hd.Plotter.DrawCircle(centerX,centerY,3,Color_Yellow);
	hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

	bool putCurCell = false;
	int left = 0, right = 0; //�-�� ��������� ������ ����� � ������ �� ������
	//������� ����������� �������
	//if(!::IsNull(hdpp(centerRow, centerCol))) { //���� � ������ ���� �������
	if(isCellExists(hdpp, centerRow, centerCol)){ //���� � ������ ���� �������
		curCell = hdpp(centerRow, centerCol);
	}
	else { //���� � ������ ��� ��������
		if (centerCol > 0) {
			//while((centerCol - left - 1 >= 0)&&(::IsNull(hdpp(centerRow, centerCol - left - 1)))) 
			while((centerCol - left - 1 >= 0) && !isCellExists(hdpp, centerRow, centerCol - left - 1))
				left++;
			/////
			//TODO: WHY without "!" ???
			/////
			
		}
		if (centerCol < hdpp.NumCols()) {
			//while ((centerCol + right < hdpp.NumCols())&&(::IsNull(hdpp(centerRow, centerCol + right)))) 
			while((centerCol + right < hdpp.NumCols())&&!isCellExists(hdpp, centerRow, centerCol + right))
				right++;
				/////
				//TODO: WHY without "!" ???
				/////
			
			
		}
		if (left + right < hdpp.CellSitesNum(curCell)) { //���� ��� �����
			if(left <= right)
				curCell = hdpp(centerRow, centerCol - left - 1);
			else
				curCell = hdpp(centerRow, centerCol + right);
		}
		else if(left + right == hdpp.CellSitesNum(curCell)  ) { //���� ���� �����
			hdpp.PutCell(curCell, centerRow, centerCol - left);
			putCurCell = true;
		}
		else {
			if(right <= hdpp.CellSitesNum(curCell))
				hdpp.PutCell(curCell, centerRow, centerCol + right - hdpp.CellSitesNum(curCell));
			else if(left <= hdpp.CellSitesNum(curCell))
				hdpp.PutCell(curCell, centerRow, centerCol - left);
			else 
				hdpp.PutCell(curCell, centerRow, centerCol);
			putCurCell = true;
		}
	}
	hd.Plotter.PlotCell(curCell, Color_Green);
	hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

	GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);

	std::vector<PseudoCell> VHole;
	VHole = getPseudoCellsFromWindow(hdpp, hd, curWnd);
	PseudoCell cntHole;

	std::vector<PseudoCell> VHoles;
	int numHoles = 0;
	int PosInGap = 0;

	if(!putCurCell){ //���� �� ������������� ������� �������
		curCell = hdpp(oldCurRow, oldCurCol);
	}
	else { //����� ������� �������
		hdpp.PutCell(curCell, oldCurRow, oldCurCol);
		cntHole.row = centerRow;
		cntHole.col = centerCol - left;
		cntHole.GapSize = left + right;
	}
	for(unsigned int i = 0; i < VHole.size(); i++) {
		VHoles.push_back(VHole[i]);
	}
	if(putCurCell){
		VHoles.push_back(cntHole);
	}
	numHoles = VHoles.size();

	hd.Plotter.PlotCell(curCell, Color_Red);
	hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);
	stat.incrementAttempts(CENTER);

	//���� ��� �������� ������ ����� � ����������� ����
	for(int i = 0; i < numHoles; i++) {		
		if(hdpp.CellSitesNum(curCell) <= VHoles[i].GapSize) {
			for(int counter=0;counter<=(VHoles[i].GapSize-hdpp.CellSitesNum(curCell));counter++)	{
				hdpp.PutCell(curCell, VHoles[i].row,VHoles[i].col+counter);

				//��� ������� ����������� ���������� ��������� ���������� �����������
				bool constraintsOK = true;
				double deltaWTWL = 0;

				if(!CheckHippocrateConstraint(hd, curCell)) constraintsOK = false;
				/*for(int j = 0; j < curNets.count; j++) {
					if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
						constraintsOK = false;
				}*/
				if(constraintsOK) { //���� ��������� �����������
					//������� ��������� WTWL (deltaWTWL)
					deltaWTWL = CalculateDiffWTWL(hd, curNets, false);
					if (deltaWTWL < maxIncrOfWTWL){
						maxIncrOfWTWL = deltaWTWL;
						k = i;
						PosInGap=counter;
					}
				}
				hdpp.PutCell(curCell, oldCurRow, oldCurCol);
			}
		}	
	}
	if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
		hdpp.PutCell(curCell, VHoles[k].row,VHoles[k].col+PosInGap);
		stat.incrementAttemptsSuccessful(CENTER);
	}
	hd.Plotter.Clear();
	hd.Plotter.PlotPlacement();
	hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
}


void HippocratePlacementSWAP(HDPGrid& hdpp, HDesign& hd, HCell& curCell, LWindow& curWnd, StatisticsAnalyser& stat)
{	
	if ((curCell,hd).IsSequential()) return;
	Utils::CalculateHPWL(hd, true);
	//double oldWTWL = Utils::CalculateWeightedHPWL(hd,true);

	std::vector<HNet> curNets;//����������, � ������� ��������� ������� �������
	for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); )
	{
		HNet net=hd.Get<HPin::Net,HNet>(pin);
		if(!::IsNull(net)) curNets.push_back(net);
	}
	double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
	int k = 0;  //����� ��-�� �� ����, ��� �������� ������. ������������ ���������� WTWL
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);

    for(int i = 0; i < curWnd.count; i++) 
	{
		if ((curWnd.cells[i],hd).IsSequential()) continue;
		std::vector<HNet> Nets; //����������, � ������� ��������� ������� �� ����
		for(HCell::PinsEnumeratorW pin=hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curWnd.cells[i]); pin.MoveNext(); )
		{
			HNet net=hd.Get<HPin::Net,HNet>(pin);
			if(!::IsNull(net)) Nets.push_back(net);
		}

		HNet dubNet; //���, � ������� ��������� ������� ��-� � ��-� �� ���� (���� ����)
		bool isConnected = false; 
		for(int m=0; m<Nets.size(); m++)
			for(int j=0; j<curNets.size(); j++)
				if(curNets[j]==Nets[m]&&(!::IsNull(curNets[j]))) {
					dubNet = curNets[j];
					isConnected = true;
				}

		int oldRow = hdpp.CellRow(curWnd.cells[i]);
		int oldCol = hdpp.CellColumn(curWnd.cells[i]);

		int row = oldCurRow, //������� ��-� ������
			col = oldCurCol; 
		int minNS = hdpp.CellSitesNum(curCell);
		int maxNS = hdpp.CellSitesNum(curWnd.cells[i]);
		if( minNS > maxNS ) {
			minNS = hdpp.CellSitesNum(curWnd.cells[i]);
			maxNS = hdpp.CellSitesNum(curCell);
			row = oldRow; //��-� �� ���� ������
			col = oldCol;						 
		}
		//���� ��������� ����� ����� � ������ �� �������� ��-��
		int LeftFreeSpace = GetLeftFreeSpace(hdpp(row, col), hdpp);
		int RightFreeSpace = GetRightFreeSpace(hdpp(row, col), hdpp);
		if( (LeftFreeSpace + RightFreeSpace + minNS) >= maxNS ) //������ ��� �������� �������
		{
			int delta1 = 0;
			int delta2 = 0;
			if( (RightFreeSpace + minNS) < maxNS ) { //������ �� ������� �����
				if(curCell==hdpp(row, col)) { //������� ������
					delta1 = maxNS - RightFreeSpace - minNS;
				}
				else { //������� ������
					delta2 = maxNS - RightFreeSpace - minNS;
				}	
			}
			hdpp.PutCell(curCell, oldRow, oldCol-delta2);						
			hdpp.PutCell(curWnd.cells[i], oldCurRow, oldCurCol-delta1);						

			//��� ������� ����������� ���������� ��������� ���������� �����������
			bool constraintsOK = true;
			double deltaWTWL = 0, RdeltaWTWL = 0;
			
			if(!(CheckHippocrateConstraint(hd, curCell)&&CheckHippocrateConstraint(hd, curWnd.cells[i]))) constraintsOK = false;
			/*
			if(!isConnected) {			
				for(int j = 0; j < curNets.count; j++) {
					if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
						constraintsOK = false;
				}
				for(int j = 0; j < Nets.count; j++) {
					if(!ControlOfConstraints(hdpp, hd, Nets.net[j], oldRow, oldCol, curWnd.cells[i]))
						constraintsOK = false;
				}
			}
			else {					
				for(int j = 0; j < curNets.count; j++) {
					if(curNets.net[j]!=dubNet)
						if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
							constraintsOK = false;
				}
				for(int j = 0; j < Nets.count; j++) {
					if(Nets.net[j]!=dubNet)
						if(!ControlOfConstraints(hdpp, hd, Nets.net[j], oldRow, oldCol, curWnd.cells[i]))
							constraintsOK = false;
				}
				for(HNet::SinksEnumeratorW sink = hd[dubNet].GetSinksEnumeratorW(); sink.MoveNext(); ) {
						if(!CheckHippocrateConstraint(hd, dubNet, sink))
								constraintsOK = false;
				}
			}
			*/

			//for(int j = 0; j < curNets.count; j++) {
			//		for(HNet::SinksEnumeratorW sink = hd[curNets.net[j]].GetSinksEnumeratorW(); sink.MoveNext(); )
			//				if(!CheckHippocrateConstraint(hd, curNets.net[j], sink))
			//						constraintsOK = false;
			//}
			//for(int j = 0; j < Nets.count; j++) {
			//		for(HNet::SinksEnumeratorW sink = hd[Nets.net[j]].GetSinksEnumeratorW(); sink.MoveNext(); )
			//				if(!CheckHippocrateConstraint(hd, Nets.net[j], sink))
			//						constraintsOK = false;
			//}
			if(constraintsOK) { //���� ��������� �����������
				//������� ��������� WTWL (deltaWTWL)
				deltaWTWL = CalculateDiffWTWL(hd, curNets, false) + CalculateDiffWTWL(hd, Nets, false);
				deltaWTWL = deltaWTWL - GetNetWeight(hd, dubNet)*Utils::CalculateHPWLDiff(hd, &dubNet, 1, false);
				//RdeltaWTWL = Utils::CalculateWeightedHPWL(hd,false) - oldWTWL;

				if (deltaWTWL < maxIncrOfWTWL){
					maxIncrOfWTWL = deltaWTWL;
					k = i;
				}
			}
			//������ �������
			hdpp.PutCell(curWnd.cells[i], oldRow, oldCol);		
			hdpp.PutCell(curCell, oldCurRow, oldCurCol);
			stat.incrementAttempts(SWAP);
		}	
		//i++;
		//ALERT("%d",i);
	}
	if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
		int row = hdpp.CellRow(curCell), //������� ��-� ������
			col = hdpp.CellColumn(curCell); 
		int minNS = hdpp.CellSitesNum(curCell);
		int maxNS = hdpp.CellSitesNum(curWnd.cells[k]);
		if( minNS > maxNS ) {
			minNS = hdpp.CellSitesNum(curWnd.cells[k]);
			maxNS = hdpp.CellSitesNum(curCell);
			row = hdpp.CellRow(curWnd.cells[k]);//��-� �� ���� ������
			col = hdpp.CellColumn(curWnd.cells[k]);				    		 
		}
		//���� ��������� ����� ����� � ������ �� �������� ��-��
		int LeftFreeSpace = GetLeftFreeSpace(hdpp(row, col), hdpp);
		int RightFreeSpace = GetRightFreeSpace(hdpp(row, col), hdpp);
		int delta1 = 0;
		int delta2 = 0;
		if( (RightFreeSpace + minNS) < maxNS ) { //������ �� ������� �����
			if(curCell==hdpp(row, col)) { //������� ������
				delta1 = maxNS - RightFreeSpace - minNS;
			}
			else { //������� ������
				delta2 = maxNS - RightFreeSpace - minNS;
			}	
		}
		//������ ��-��
		//hd.Plotter.PlotCell(curCell, Color_Indigo);
		//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
		//hd.Plotter.PlotCell(curWnd.cells[k], Color_LemonChiffon);
		//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);	
		hdpp.PutCell(curCell, hdpp.CellRow(curWnd.cells[k]),hdpp.CellColumn(curWnd.cells[k])-delta2);
		//hd.Plotter.PlotCell(curCell, Color_Indigo);
		//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);	
		hdpp.PutCell(curWnd.cells[k], oldCurRow, oldCurCol-delta1);
		//hd.Plotter.PlotCell(curWnd.cells[k], Color_LemonChiffon);
		//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);

		stat.incrementAttemptsSuccessful(SWAP);

		//---��� ��� ������������ ��������� ������� ������ ��������� � �����			
		HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);
		HippocratePlacementLOCALMOVE(hdpp, hd, curWnd.cells[k], stat);
		//--------------------------
	//	STA(hd,false);
	} 
}

void HippocratePlacementLOCALMOVE(HDPGrid& hdpp, HDesign& hd, HCell& curCell, StatisticsAnalyser& stat)
{
  int basecol = hdpp.CellColumn(curCell);
  int baserow = hdpp.CellRow(curCell);
  double hpwl_diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);//= 0.0;//but I don't trust input.
  int bestcol = basecol;
  
  //go left
	for (int lcol = basecol - 1; lcol >= 0 && !isCellExists(hdpp,baserow, lcol); --lcol)
  //for (int lcol = basecol - 1; lcol >= 0 && ::IsNull(hdpp(baserow, lcol)); --lcol)
  {
    hdpp.PutCellIntoSiteFast(curCell, lcol);
    double diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);
    if (diff < hpwl_diff)
      if (CheckHippocrateConstraint(hd, curCell))
      {
        bestcol = lcol;
        hpwl_diff = diff;
      }
  }

  //go right
  int len = hdpp.CellSitesNum(curCell);
  int maxPos = hdpp.NumCols() - len;
  //for (int rcol = basecol + 1; rcol < maxPos && ::IsNull(hdpp(baserow, rcol + len - 1)); ++rcol)
	/////
	//TODO: WHY without "!" ???
	/////
	for (int rcol = basecol + 1; rcol < maxPos && !isCellExists(hdpp,baserow, rcol + len - 1); ++rcol)
  {
    hdpp.PutCellIntoSiteFast(curCell, rcol);
    double diff = Utils::CalculateHPWLDiff(hd, &curCell, 1, false);
    if (diff < hpwl_diff)
      if (CheckHippocrateConstraint(hd, curCell))
      {
        bestcol = rcol;
        hpwl_diff = diff;
      }
  }

  //choose best position
  if (bestcol == basecol)
  {
    hdpp.PutCellIntoSiteFast(curCell, basecol);
    stat.incrementAttempts(LOCALMOVE);
  }
  else
  {
    hdpp.PutCellIntoSite(curCell, bestcol);
    Utils::CalculateHPWLDiff(hd, &curCell, 1, true);
    stat.incrementAttemptsSuccessful(LOCALMOVE);
  }

  return;
/*
	//OUR OLD LOCALMOVE
	Utils::CalculateHPWL(hd, true);
	NetsVector curNets(curCell, hd);  //����������, � ������� ��������� ������� �������
	double maxIncrOfWTWL = 0;  //������������ ���������� WTWL
	int k = 0;  //����� ��-�� �� ����, ��� �������� ������. ������������ ���������� WTWL
	int oldCurRow = hdpp.CellRow(curCell);
	int oldCurCol = hdpp.CellColumn(curCell);

	std::vector<PossiblePos> VPossibleHorizontalPos=getVPossibleHorizontalPos(hdpp, curCell);
	for(int i = 0; i < VPossibleHorizontalPos.size(); i++) 
	{		
		hdpp.PutCell(curCell, VPossibleHorizontalPos[i].row,VPossibleHorizontalPos[i].col);

		//��� ������� ����������� ���������� ��������� ���������� �����������
		bool constraintsOK = true;
		double deltaWTWL = 0;
		for(int j = 0; j < curNets.count; j++) {
			if(!ControlOfConstraints(hdpp, hd, curNets.net[j], oldCurRow, oldCurCol, curCell))
				constraintsOK = false;
		}

		if(constraintsOK) { //���� ��������� �����������
			//������� ��������� WTWL (deltaWTWL)
			deltaWTWL = CalculateDiffWTWL(hd, &curNets, false);
			if (deltaWTWL < maxIncrOfWTWL){
				maxIncrOfWTWL = deltaWTWL;
				k = i;
			}
		}
		hdpp.PutCell(curCell, oldCurRow, oldCurCol);
		//numOfLOCALMOVEAttepmts++;
		stat.incrementAttempts(LOCALMOVE);
	}
	if (maxIncrOfWTWL < 0) { //���� ����� �������� �����������
		//������ ��-��
		hd.Plotter.SaveMilestoneImage("localMOVE_B");
		hd.Plotter.PlotCell(curCell, Color_Brown);
		hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

		hdpp.PutCell(curCell, VPossibleHorizontalPos[k].row,VPossibleHorizontalPos[k].col);

		hd.Plotter.PlotCell(curCell, Color_Brown);
		hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);	
		hd.Plotter.SaveMilestoneImage("localMOVE_A");
		stat.incrementAttemptsSuccessful(LOCALMOVE);
		//STA(hd,false);
	} 

*/

}
//
//bool ControlOfConstraints(HDPGrid& hdpp, HDesign& hd, HNet net, int oldRow, int oldCol, HCell& curCell) 
//{
//
//	HNetWrapper wnet = hd[net];
//	if (hd.cfg.ValueOf("HippocratePlacement.KamaevCheckConstraints", false))
//	{
//		for (HNet::SinksEnumeratorW sink = wnet.GetSinksEnumeratorW(); sink.MoveNext(); ) 
//			if(!CheckHippocrateConstraint(hd,net,sink)) return false;
//	}else{
//		double deltaHPWL = Utils::CalculateHPWLDiff(hd, &net, 1, false);
//
//		double alpha = CalculateAlpha(hd, net);//Rd * c
//		HPinWrapper source = hd[hd.Get<HNet::Source, HPin>(net)]; //����� sourse ����
//
//		double srcx = source.X();
//		double srcy = source.Y();
//		if(source.Cell()==curCell) {  //���� sourse ���� ����������� �������� ��-��
//			double oldx = hdpp.ColumnX(oldCol) + source.OffsetX();
//			double oldy = hdpp.RowY(oldRow) + source.OffsetY();
//			//��� �� ���� ������ ���������� � ��������� �������
//			for (HNet::SinksEnumeratorW sink = wnet.GetSinksEnumeratorW(); sink.MoveNext(); ) {
//				//�������� �������
//				double deltaMDist;   //�����-� �������. ���������� �-�� source � sink
//				double sinkx = sink.X();
//				double sinky = sink.Y();
//				deltaMDist = abs(sinkx - srcx) + abs(sinky - srcy) - abs(oldx - sinkx) - abs(oldy - sinky);
//				double deltaArrTime; //�����-� AT ��� sink
//				deltaArrTime = CalculateDeltaArrivalTime(hd, sink);
//				//double oldMDist = abs(sinkx - srcx) + abs(sinky - srcy);//������ �������. ���������� �-�� source � sink
//				double oldMDist = abs(oldx - sinkx) + abs(oldy - sinky);
//				double betta = CalculateBetta(hd, sink, oldMDist); //r * (c * hpwl + Cs)
//				//���� ������� �� ���������
//				double gamma=CalculateGamma(hd);
//				if(alpha*deltaHPWL+betta*deltaMDist+gamma*deltaMDist*deltaMDist > deltaArrTime)
//					return false;
//
//			}
//		}					
//		else {  //���� source ���� �� ����������� �������� ��-��
//			//��� �� ���� ����� ���������� � ������� ������� ��� ��� �������� ��-��
//			for (HNet::SinksEnumeratorW sink = wnet.GetSinksEnumeratorW(); sink.MoveNext(); ) 
//			{
//				if(sink.Cell()==curCell){
//					double oldx = hdpp.ColumnX(oldCol) + sink.OffsetX();
//					double oldy = hdpp.RowY(oldRow) + sink.OffsetY();
//					//�������� �������
//					double deltaMDist;   //�����-� �������. ���������� �-�� source � sink
//					double newx = sink.X();
//					double newy = sink.Y();
//					deltaMDist = abs(newx - srcx) + abs(newy - srcy) - abs(oldx - srcx) - abs(oldy - srcy);
//					double deltaArrTime; //�����-� AT ��� sink
//					deltaArrTime = CalculateDeltaArrivalTime(hd, sink);
//					double oldMDist = abs(oldx - srcx) + abs(oldy - srcy);//������ �������. ���������� �-�� source � sink
//					double betta = CalculateBetta(hd, sink, oldMDist);
//					double gamma=CalculateGamma(hd);
//					//���� ������� �� ���������
//					if(alpha*deltaHPWL+betta*deltaMDist+gamma*deltaMDist*deltaMDist > deltaArrTime)
//						return false;
//				}
//			}	
//		}
//	}
//	return true;
//}
//

double CalculateDeltaArrivalTime(HDesign& hd, HPinWrapper sink)
{
	double deltaArrivalTime; //�����-� AT ��� sink
	HCell cell = sink.Cell();
	double cellAT; //AT ��� ��������
	double sinkAT; //AT ��� sink
	HTimingPointWrapper tpsink = hd[hd.TimingPoints[sink]]; 
	sinkAT = tpsink.ArrivalTime();
	cellAT = sinkAT;
	//������� ������� ���� ��������
	for (HCell::PinsEnumeratorW pin = hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(cell); pin.MoveNext(); ) {
		if (pin.Direction() == PinDirection_INPUT) {
			HTimingPointWrapper tp = hd[hd.TimingPoints[pin]];
			if(cellAT < tp.ArrivalTime()) {
				cellAT = tp.ArrivalTime();
			}
		}
	}
	deltaArrivalTime = cellAT - sinkAT;
	return deltaArrivalTime;
}

double CalculateAlpha(HDesign& hd, HNet changedNet) {
	HPin source = hd.Get<HNet::Source, HPin>(changedNet);
	double Rdriver = Utils::GetDriverWorstPhisics(hd, source, SignalDirection_None).R;
	double c = hd.RoutingLayers.Physics.LinearC;  //�������� �������������
	//double alpha = Rdriver*c;
	//return alpha;
	return Rdriver*c;
}

double CalculateBetta(HDesign& hd, HPinWrapper sink, double oldMDist)
{
	double betta;
	double r = hd.RoutingLayers.Physics.RPerDist; //�������� �������������
	double c = hd.RoutingLayers.Physics.LinearC;  //�������� �������������
	double csink = Utils::GetSinkCapacitance(hd, sink, SignalDirection_None);
	betta = CONST_Kd*r*(c*oldMDist*0.5 + csink);
	return betta;
}

inline double CalculateGamma(HDesign& hd)
{
	static double r = hd.RoutingLayers.Physics.RPerDist; //�������� �������������
	static double � = hd.RoutingLayers.Physics.LinearC;  //�������� �������������
	static double gamma=CONST_Kd*r*�*0.5;
	return gamma;
}		

double GetNetSlack(HDesign& hd, HNet& net)
{
	HPin source = hd.Get<HNet::Source, HPin>(net);
	HTimingPoint sPoint = hd.TimingPoints[source];
	return hd.GetDouble<HTimingPoint::Slack>(sPoint);
}

__inline double GetNetWeight(HDesign& hd, HNet& net)
{
	double slack = GetNetSlack(hd, net);
	if(slack < 0)
		return (CONST_A - CONST_B*slack);
	return CONST_A;
}

double CalculateDiffWTWL(HDesign& hd, std::vector<HNet>& netvector, bool updateCachedValues)
{
	double result = 0.0;
	for(int i = 0; i < netvector.size(); i++)
		result += GetNetWeight(hd, netvector[i]) * Utils::CalculateHPWLDiff(hd, (HNet*)&netvector[i], 1, updateCachedValues);
	return result;
}

/*double CalculateDiffWTWL(HDesign& hd, NetsVector* netvector, bool updateCachedValues)
{
	double result = 0.0;
	for(int i = 0; i < netvector->count; i++)
		result += GetNetWeight(hd, netvector->net[i]) * Utils::CalculateHPWLDiff(hd, (HNet*)&netvector->net[i], 1, updateCachedValues);
	return result;
}*/
BBox GetCurrentBBox(HDPGrid& hdpp, HDesign& hd, const HCriticalPath::PointsEnumeratorW& pointsEnumW)
{
	HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];
	HNetWrapper netW = hd[pin.Net()];
	//hd.Plotter.PlotNet(netW);
	//hd.Plotter.Refresh(HPlotter::NO_WAIT);

	HNetWrapper::PinsEnumeratorW pinsEnumW = netW.GetPinsEnumeratorW();
	HCell cell=hd[pinsEnumW].Cell();
	int c=hdpp.CellColumn(cell);
	int r=hdpp.CellRow(cell);
	int rmax=r, rmin=r, cmax=c, cmin=c;

	BBox tBBox;

	//for (;pinsEnumW.MoveNext();)
	while(pinsEnumW.MoveNext())
	{

		HCell cell=hd[pinsEnumW].Cell();

//		if((tBBox.cells.size()!=0)&&(tBBox.cells.back()==cell)){

			//ALERT("AAAAAAA CELL IS ALREADY HEREEEE!!!!!"));
	//	}else
		if((tBBox.cells.size()==0)||(tBBox.cells.back()!=cell)){
			if (!::IsNull(cell))
				tBBox.cells.push_back(cell);
			else 
				ALERT("Cell is null in CurBBox");
			int c=hdpp.CellColumn(cell);
			int r=hdpp.CellRow(cell);
			if (c>cmax) cmax=c;
			if (c<cmin) cmin=c;
			if (r>rmax) rmax=r;
			if (r<rmin) rmin=r;
		}
	}

	tBBox.col=cmin;
	tBBox.row=rmin;
	tBBox.width=cmax-cmin;
	tBBox.height=rmax-rmin;
	tBBox.colCenter=tBBox.col+(tBBox.width-tBBox.width%2)/2;
	tBBox.rowCenter=tBBox.row+(tBBox.height-tBBox.height%2)/2;

	double x1, y1, x2, y2, xc, yc;
	x1=hdpp.ColumnX(tBBox.col);
	y1=hdpp.RowY(tBBox.row);

	x2=hdpp.ColumnX(tBBox.col+tBBox.width);
	y2=hdpp.RowY(tBBox.row+tBBox.height);

	xc=hdpp.ColumnX(tBBox.colCenter);
	yc=hdpp.RowY(tBBox.rowCenter);

	hd.Plotter.DrawRectangle(x1,y1,x2,y2,Color_Red);
	hd.Plotter.DrawCircle(xc, yc, 6, Color_Red);
	hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
	hd.Plotter.SaveMilestoneImage("BBox");
	//hd.Plotter.ShowPlacement();

	return tBBox;						
}

/*void ProcessPath(HDPGrid &hdpp, HDesign &hd, HCriticalPaths::EnumeratorW &critPathEnumW, StatisticsAnalyser& stat, TimingHPWLWatcher& thpwlWatcher) 
{
	HCell curCell;
	LPoint curPoint;
	BBox curBBox;

	HCell prevCell;
	bool f = false;	
	bool isFirst = true;

	int numOfPoint=0;
	for (HCriticalPath::PointsEnumeratorW pointsEnumW = critPathEnumW.GetEnumeratorW(); pointsEnumW.MoveNext();)
	{
		numOfPoint++;
		f = false;
		HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];

		if (isFirst) { //���� ������ ������� ������������ ����
			prevCell = pin.Cell();
		}
		else prevCell = curCell;

		curCell = pin.Cell();

		if(curCell == prevCell) { //�������� ���������
			f = true;
		}
		curPoint.col = hdpp.FindColumn(hd.GetDouble<HCell::X>(curCell));
		curPoint.row = hdpp.FindRow(hd.GetDouble<HCell::Y>(curCell));

		if((!f)||isFirst) {  //������� � ���������� �������� ������
			if(!::IsNull<HCell>(curCell)) {
				//ALERT("--Taking next point # %d", numOfPoint++));

				if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementSWAP(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
					curBBox = GetCurrentBBox(hdpp, hd, pointsEnumW);
					HippocratePlacementCOMPACT(hdpp, hd, curCell, curBBox, stat);						
				}
				if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementMOVE(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementCENTER(hdpp, hd, curCell, curWnd, stat);						
				}
				if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);						
				}

				thpwlWatcher.doReport();
			}
		}
		isFirst = false;
	}
}*/

void DoHippocratePlacement( HDPGrid& hdpp, HDesign& hd, StatisticsAnalyser& stat, TimingHPWLWatcher& thpwlWatcher )
{

	if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
		ALERT("-------SWAP ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
		ALERT("-------COMPACT ENABLED");	
	}
	if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
		ALERT("-------MOVE ENABLED");	
	}
	if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
		ALERT("-------CENTER ENABLED");
	}
	if(hd.cfg.ValueOf("HippocratePlacement.SECCOMPACT", false)){
		ALERT("-------SECCOMPACT ENABLED");	
	}
	if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
		ALERT("-------LOCALMOVE ENABLED");	
	}	

	if(hd.cfg.ValueOf("HippocratePlacement.KamaevCheckConstraints", false)){
		ALERT("-------KamaevCheck ENABLED");	
	}else{
		ALERT("-------OurCheck ENABLED");	
	}

	if(hd.cfg.ValueOf("HippocratePlacement.PathsInCriticalOrder", true)){
		ALERT("-------PathInCriticalOrder ENABLED");	
	}else{
		ALERT("-------PathInRandomOrder ENABLED");	
	}


	//int pp = 0;
	STA(hd, true, true);
	FindCriticalPaths(hd);
	PathCallback callback(hdpp, stat, thpwlWatcher);

	if (hd.cfg.ValueOf("HippocratePlacement.PathsInCriticalOrder", true))
	{
		Utils::IterateMostCriticalPaths(hd, -1, Utils::CriticalPathHandler(&callback, &PathCallback::ProcessPath2));
	}
	else{
		hd.Plotter.ShowPlacement();
		hd.Plotter.SaveMilestoneImage("Before_Hippocrate");

		Utils::CalculateHPWL(hd, true);

		int numOfPath=0;
		
//		char temp[100];

		for (HCriticalPaths::EnumeratorW critPathEnumW = hd.CriticalPaths.GetEnumeratorW(); critPathEnumW.MoveNext();)
		{
			if(!(numOfPath%10)) ALERT("--Taking next path # %d", numOfPath);
			numOfPath++;

			hd.Plotter.PlotCriticalPath(critPathEnumW);
			hd.Plotter.Refresh(HPlotter::WAIT_3_SECONDS);

			callback.ProcessPath2(hd, critPathEnumW, -1);

			hd.Plotter.ShowPlacement();
		}
	}
}

int RightCounting(int row, int column, int R, int RL, HCell* hcell, HDPGrid& hdpp)	// ����� ������
{
	ASSERT(row >= 0 && column >= 0 && row < hdpp.NumRows() && column < hdpp.NumCols());
	
	int counter = 0;
	while(counter < R + RL + 1 && column < hdpp.NumCols())
	{
		if(isCellExists(hdpp, row, column))	
		{
				hcell[counter] = hdpp(row,column);
				counter++;
				if(isCellExists(hdpp, row, column-1))
						if(hdpp(row,column) == hdpp(row,column-1)) 
								counter--;
		}
		column++;
	}
	return counter;	
}

int FindFirstLeftCell(int row, int column, int R, HDPGrid& hdpp)	// ����� ������ ������� �����
{
	ASSERT(row >= 0 && column >= 0 && row < hdpp.NumRows() && column < hdpp.NumCols());
	
	int col = column;
	int counter = 0;	
	col--;
	while(counter < R+1 && col >= 0)
	{
			if(isCellExists(hdpp, row, col))
			{
					counter++;
					if(isCellExists(hdpp, row, col+1))
							if(hdpp(row,col) == hdpp(row,col+1))
									counter--;
			}
			col--;
	}	
	if(col < 0)		
			col = 0;
	while(!isCellExists(hdpp, row, col))
			col++;
	return col;	
}

void GetCurrentWindow(HDPGrid& hdpp, int R, HCell curCell, LWindow& curWnd)	// ������� ���� ��������
{
	hdpp.Design().Plotter.PlotCell(curCell, Color_Indigo);
	hdpp.Design().Plotter.Refresh(HPlotter::WAIT_1_SECOND);
	
	int RL = 0; //���-�� cell'�� ����� �� �������� cell'� (� ����)
	int row = hdpp.CellRow(curCell);        // ����� ���� �������� cell'�
	int column = hdpp.CellColumn(curCell);  // ����� ����� �������� cell'�

	int x_min = FindFirstLeftCell(row, column, R, hdpp);

	while(x_min < column)
	{
			if(isCellExists(hdpp, row, x_min)) {
					RL++;
					if(isCellExists(hdpp, row, x_min-1)) {
							if(hdpp(row,x_min) == hdpp(row,x_min-1))
									RL--;
					}
			}
			x_min++;
	}

  int maxcount = (R + RL + 1)*(2*R + 1);  // ������������ ���-�� cell'�� � ����
	HCell* hcell = new HCell[maxcount];	    // �������� ������ ��� ������ cell'��

	int y_min = row - R;
	int y_max = row + R;

	if(y_min < 0)
		y_min = 0;	

	if(y_max > hdpp.NumRows() - 1)
		y_max = hdpp.NumRows() - 1;

	int temp = 0;
	for(int i = y_min; i <= y_max; i++)	{	
		x_min = FindFirstLeftCell(i, column, R, hdpp);
		temp += RightCounting(i, x_min, R, RL, hcell + temp, hdpp);	
	}

	curWnd.R = R;
	curWnd.count = temp;
	for(int j = 0; j < curWnd.count; j++)	{	
		curWnd.cells[j] = hcell[j];	
	}
	delete[] hcell;
}
SpecWindow getSpecWindForCell(HDPGrid& hdpp, HDesign& hd, HCell& CellFromNet, BBox& curBBox )
{
	SpecWindow SpWind;
		if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter))) 
	{	
		SpWind.StartRow=hdpp.CellRow(CellFromNet);
		SpWind.StartCol=hdpp.CellColumn(CellFromNet);
		SpWind.FinishRow=curBBox.rowCenter;
		SpWind.FinishCol=curBBox.colCenter;
	}else
		if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter))) 
		{	
			SpWind.StartRow=hdpp.CellRow(CellFromNet);
			SpWind.StartCol=curBBox.colCenter;
			SpWind.FinishRow=curBBox.rowCenter;
			SpWind.FinishCol=hdpp.CellColumn(CellFromNet);
		}else
			if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter))) 
			{	
				SpWind.StartRow=curBBox.rowCenter;
				SpWind.StartCol=hdpp.CellColumn(CellFromNet);
				SpWind.FinishRow=hdpp.CellRow(CellFromNet);
				SpWind.FinishCol=curBBox.colCenter;
			}else
				if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter))) 
				{	
					SpWind.StartRow=curBBox.rowCenter;
					SpWind.StartCol=curBBox.colCenter;
					SpWind.FinishRow=hdpp.CellRow(CellFromNet);
					SpWind.FinishCol=hdpp.CellColumn(CellFromNet);
				}
	return(SpWind);
}
std::vector<PseudoCell> getGapsForCell(HDPGrid& hdpp, HDesign& hd, HCell& CellFromNet, BBox& curBBox )
{
	std::vector<PseudoCell> VGaps;
	PseudoCell newGap;
	SpecWindow SpWind = getSpecWindForCell(hdpp,hd,CellFromNet,curBBox);
	int extraSites=-1;
	for(int TempRow=SpWind.StartRow;TempRow>=SpWind.FinishRow;TempRow--)
	{
		int TempCol=SpWind.StartCol;
		if (hdpp.NumCols()-SpWind.FinishCol-1 < 10) extraSites=hdpp.NumCols()-SpWind.FinishCol-1;
		else extraSites = 10;

		while ((TempCol<=SpWind.FinishCol)||((TempCol<=SpWind.FinishCol+extraSites)&&(!isCellExists(hdpp, TempRow, TempCol))))
		{
			if (!isCellExists(hdpp, TempRow, TempCol))
			{
				newGap.col=TempCol;
				newGap.row=TempRow;
				newGap.GapSize=0;
				while ((!isCellExists(hdpp, TempRow, TempCol))&&(TempCol<=SpWind.FinishCol+extraSites))
				{
					TempCol++;
					newGap.GapSize++;
				}
				VGaps.push_back(newGap);
			}
			else TempCol++;
		}

	}

	return VGaps;
}
std::vector<HCell> getSpecialWindow(HDPGrid& hdpp, HDesign& hd, HCell& CellFromNet, BBox& curBBox )
{
	int StartCol, StartRow, FinishCol, FinishRow;
	if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter))) 
	{	
		StartRow=hdpp.CellRow(CellFromNet);
		StartCol=hdpp.CellColumn(CellFromNet);
		FinishRow=curBBox.rowCenter;
		FinishCol=curBBox.colCenter;
	}else
		if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)>=curBBox.rowCenter))) 
		{	
			StartRow=hdpp.CellRow(CellFromNet);
			StartCol=curBBox.colCenter;
			FinishRow=curBBox.rowCenter;
			FinishCol=hdpp.CellColumn(CellFromNet);
		}else
			if ((hdpp.CellColumn(CellFromNet)<=curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter))) 
			{	
				StartRow=curBBox.rowCenter;
				StartCol=hdpp.CellColumn(CellFromNet);
				FinishRow=hdpp.CellRow(CellFromNet);
				FinishCol=curBBox.colCenter;
			}else
				if ((hdpp.CellColumn(CellFromNet)>curBBox.colCenter)&&((hdpp.CellRow(CellFromNet)<curBBox.rowCenter))) 
				{	
					StartRow=curBBox.rowCenter;
					StartCol=curBBox.colCenter;
					FinishRow=hdpp.CellRow(CellFromNet);
					FinishCol=hdpp.CellColumn(CellFromNet);
				}
				std::vector<HCell> VCellInWindow;
				HCell tempCell;
				bool NotFoundLastInRow=false;
				for(int TempRow=StartRow;TempRow>=FinishRow;TempRow--)
				{
					int TempCol=StartCol;
					while(TempCol<=FinishCol)
					{
						
						//if (! ( ::IsNull(hdpp(TempRow, TempCol)) ) )
						if(isCellExists(hdpp, TempRow, TempCol))
						{
							tempCell=hdpp(TempRow,TempCol);
						}	
						else 
						{
							TempCol++;
							NotFoundLastInRow=false;
							while(((TempCol<=FinishCol))&&!isCellExists(hdpp, TempRow, TempCol))
							//while(((TempCol<=FinishCol))&& (::IsNull(hdpp(TempRow, TempCol)) ))
								
							{
								TempCol++;
							}
							if (TempCol>FinishCol) NotFoundLastInRow=true;
							else tempCell=hdpp(TempRow,TempCol);
						}

						if(!NotFoundLastInRow){
							if((VCellInWindow.size()==0)||(tempCell!=VCellInWindow.back()))
								VCellInWindow.push_back(tempCell);
						}
						TempCol++;

					}
				}

				//std::vector<HCell>::iterator iter=VCellInWindow.begin();
				//hd.Plotter.PlotCell(CellFromNet, Color_YellowGreen);
				//hd.Plotter.Refresh(HPlotter::NO_WAIT);
				//for(;iter!=VCellInWindow.end();iter++){
				//hd.Plotter.PlotCell(*iter, Color_YellowGreen);
				//hd.Plotter.Refresh(HPlotter::NO_WAIT);
				//}
				//double x1, y1, x2, y2;
				//x1=hdpp.ColumnX(StartCol);
				//y1=hdpp.RowY(StartRow);
				//x2=hdpp.ColumnX(FinishCol);
				//y2=hdpp.RowY(FinishRow);
				//hd.Plotter.DrawRectangle(x1,y1,x2,y2,Color_Gray);
				////hd.Plotter.Refresh(HPlotter::NO_WAIT);
				//hd.Plotter.SaveMilestoneImage("SpecWind");

				//hdpp.HDPGrid.RowY()
				return (VCellInWindow);
}

std::vector<PossiblePos> getVPossibleHorizontalPos(HDPGrid& hdpp, HCell curCell)
{
	PseudoCell LocalGap;
	LocalGap.row=hdpp.CellRow(curCell);
	LocalGap.col=hdpp.CellColumn(curCell)-GetLeftFreeSpace(curCell, hdpp);
	LocalGap.GapSize=hdpp.CellSitesNum(curCell)+GetRightFreeSpace(curCell, hdpp)+GetLeftFreeSpace(curCell, hdpp);

	std::vector<PossiblePos> VPossibleHorizontalPos;
	for (int i=0; i<LocalGap.GapSize-hdpp.CellSitesNum(curCell)+1;i++)
	{
		PossiblePos tmp;
		tmp.row=LocalGap.row;
		tmp.col=LocalGap.col + i;
		VPossibleHorizontalPos.push_back(tmp);
	}


	return VPossibleHorizontalPos;
}

std::vector<PseudoCell> getPseudoCellsFromSpecWindow(HDPGrid& hdpp, HDesign& hd, std::vector<HCell>& VCellInSpWind )
{
	std::vector<PseudoCell> VPsCellInSpWindow;

	int imax=VCellInSpWind.size()-1;
	for (int i=0;i<imax;i++)
	{
		HCell currentCell=VCellInSpWind[i];

		HCell nextcell=VCellInSpWind[i+1];
		if (hdpp.CellRow(nextcell) == hdpp.CellRow(currentCell))
		{
			int holeSize=hdpp.CellColumn(VCellInSpWind[i+1])-
				(hdpp.CellColumn(VCellInSpWind[i])+
				hdpp.CellSitesNum(VCellInSpWind[i]));
			if (holeSize>0)
			{
				PseudoCell temp;
				temp.col=hdpp.CellColumn(VCellInSpWind[i])+hdpp.CellSitesNum(VCellInSpWind[i]);
				temp.row=hdpp.CellRow(VCellInSpWind[i]);
				temp.GapSize=holeSize;
				VPsCellInSpWindow.push_back(temp);
			}
		}
	}
	return (VPsCellInSpWindow);
}

std::vector<PseudoCell> getPseudoCellsFromWindow(HDPGrid& hdpp, HDesign& hd, LWindow& curWnd){
	static int windownum=0;
	std::vector<HCell> VCellsWindow;
	VCellsWindow.reserve(curWnd.count);
	//ALERT("count: %d", curWnd.count);
	for(int j = 0; j < curWnd.count; j++)	{	
		//ALERT("cellb# %d", j);
		VCellsWindow.push_back(curWnd.cells[j]);

		//hd.Plotter.PlotCell(curWnd.cells[j], Color_Indigo);

		//int c=hdpp.CellColumn(curWnd.cells[j]);
		//int r=hdpp.CellRow(curWnd.cells[j]);
		//ALERT("cell#, r, c: %d %d %d", j, r, c);
	}

	std::vector<HCell>::iterator iterVCellsWindow=VCellsWindow.begin();

	//hd.Plotter.PlotCell(curCell, Color_Red);
	//hd.Plotter.Refresh(HPlotter::NO_WAIT);
	//hd.Plotter.SaveMilestoneImage("CELLS_IN_WINDOW");


	//ALERT("SORT1");
	sort(VCellsWindow.begin(), VCellsWindow.end(), CellRowComparator(hdpp));

	//ALERT("SORT2");
	int numOfLastRow=hdpp.CellRow(*(VCellsWindow.end()-1));
	iterVCellsWindow=VCellsWindow.begin();
	std::vector<HCell>::iterator tmp=iterVCellsWindow;
	for(;iterVCellsWindow!=VCellsWindow.end();iterVCellsWindow++){
		//ALERT("PROCESSING ELEMENT %d",hdpp.CellRow(*(iterVCellsWindow)));
		if (iterVCellsWindow+1 == VCellsWindow.end())
		{
			sort(tmp,VCellsWindow.end(),CellColComparator(hdpp));
			break;
		}else{
			if (hdpp.CellRow(*(iterVCellsWindow+1)) != hdpp.CellRow(*(iterVCellsWindow))){
				//ALERT("PROCESSING ROW %d",hdpp.CellRow(*(iterVCellsWindow)));
				sort(tmp,iterVCellsWindow+1,CellColComparator(hdpp));
				tmp=iterVCellsWindow+1;
			}
		}
	}

	//-----------------filling std::vector<PseudoCell>-------------------

	iterVCellsWindow=VCellsWindow.begin();
	std::vector<PseudoCell> VPsCellWindow;
	for(;iterVCellsWindow!=VCellsWindow.end()-1;iterVCellsWindow++)
	{

		if (hdpp.CellRow(*(iterVCellsWindow+1)) == hdpp.CellRow(*(iterVCellsWindow)))	
		{
			int holeSize=hdpp.CellColumn(*(iterVCellsWindow+1))-
				(hdpp.CellColumn(*iterVCellsWindow)+
				hdpp.CellSitesNum(*iterVCellsWindow));
			if (holeSize>0)
			{
				PseudoCell temp;
				temp.col=hdpp.CellColumn(*iterVCellsWindow)+hdpp.CellSitesNum(*iterVCellsWindow);
				temp.row=hdpp.CellRow(*(iterVCellsWindow));
				temp.GapSize=holeSize;
				VPsCellWindow.push_back(temp);
			}
		}
	}
	//ALERT("Num of Holes: %d", VPsCellWindow.size());

	hd.Plotter.ShowPlacement();

	VCellsWindow.clear();
	return (VPsCellWindow);
}


std::vector<HCell> GetCellsConnectedWithCurrent(HDPGrid& hdpp, HDesign& hd, HCell curCell) 
{
	std::vector<HCell> ConnectedCells; //��-��, ����������� � �������		
	int connectedCellsCount = 0, j = 0;

	//hd.Plotter.PlotCell(curCell, Color_Red);
	//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);

	//��������� ������ ��-���, ����������� � �������
	for(HCell::PinsEnumeratorW pin = hd.Get<HCell::Pins, HCell::PinsEnumeratorW>(curCell); pin.MoveNext(); ) {
		HNet net = hd.Get<HPin::Net,HNet>(pin);
		HNetWrapper netw = hd[hd.Get<HPin::Net,HNet>(pin)];
		if(!::IsNull(net)) {
			//hd.Plotter.PlotNet(netw);
			//hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
			if(pin == hd[hd.Get<HNet::Source, HPin>(netw)]) { //���� source
				for (HNet::PinsEnumeratorW npin = netw.GetPinsEnumeratorW(); npin.MoveNext(); ) {
					if(npin!=hd[hd.Get<HNet::Source, HPin>(netw)])
						if(!::IsNull(npin.Cell())){
							ConnectedCells.push_back(npin.Cell());
						}	
				}
			}
			else {//���� sink
				HCell cell = hd[hd.Get<HNet::Source, HPin>(hd.Get<HPin::Net,HNet>(pin))].Cell();
				if(!::IsNull(cell))
					ConnectedCells.push_back(cell);
			}	
		}
	}
	/*for(int i=0; i<ConnectedCells.size(); i++) {	
	hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
	hd.Plotter.PlotCell(ConnectedCells[i], Color_Black);
	hd.Plotter.Refresh(HPlotter::WAIT_1_SECOND);
	}*/
	return ConnectedCells;
}


void StatisticsAnalyser::incrementAttempts( HippocrateMethods method )
{
	switch (method)
	{
	case MOVE:
		numOfMOVEAttepmts++;
		break;
	case COMPACT:
		numOfCOMPACTAttepmts++;
		break;
	case CENTER:
		numOfCENTERAttepmts++;
		break;
	case LOCALMOVE:
		numOfLOCALMOVEAttepmts++;
		break;
	case SWAP:
		numOfSWAPAttepmts++;
		break;
	}
}

void StatisticsAnalyser::incrementAttemptsSuccessful( HippocrateMethods method )
{
	switch (method)
	{
	case MOVE:
		if (reportSuccessful)	ALERT("!!!ELEMENTS MOVED!!!");
		numOfMOVESuccessfulAttempts++;
		break;
	case COMPACT:
		if (reportSuccessful)	ALERT("!!!ELEMENTS MOVED (COMPACT)!!!");
		numOfCOMPACTSuccessfulAttempts++;
		break;
	case CENTER:
		if (reportSuccessful)	ALERT("!!!ELEMENTS CENTERED!!!");
		numOfCENTERSuccessfulAttempts++;
		break;
	case LOCALMOVE:
		if (reportSuccessful)	ALERT("!!!ELEMENTS localMOVED!!!");
		numOfLOCALMOVESuccessfulAttempts++;
		break;
	case SWAP:
		if (reportSuccessful)	ALERT("!!!ELEMENTS SWAPPED!!!");
		numOfSWAPSuccessfulAttempts++;
		break;
	}
}

StatisticsAnalyser::StatisticsAnalyser(){
	reportSuccessful=true;
	numOfSWAPAttepmts=0;
	numOfSWAPSuccessfulAttempts=0;
	numOfMOVEAttepmts=0;
	numOfMOVESuccessfulAttempts=0;
	numOfLOCALMOVEAttepmts=0;
	numOfLOCALMOVESuccessfulAttempts=0;
	numOfCOMPACTAttepmts=0;
	numOfCOMPACTSuccessfulAttempts=0;
	numOfCENTERAttepmts=0;
	numOfCENTERSuccessfulAttempts=0;
}

void StatisticsAnalyser::doReport(){
	ALERT("SWAP attempts: %d", numOfSWAPAttepmts);
	ALERT("SWAP successful attempts: %d", numOfSWAPSuccessfulAttempts);
	ALERT("MOVE attempts: %d", numOfMOVEAttepmts);
	ALERT("MOVE successful attempts: %d", numOfMOVESuccessfulAttempts);
	ALERT("LOCALMOVE attempts: %d", numOfLOCALMOVEAttepmts);
	ALERT("LOCALMOVE successful attempts: %d", numOfLOCALMOVESuccessfulAttempts);
	ALERT("COMPACT attempts: %d", numOfCOMPACTAttepmts);
	ALERT("COMPACT successful attempts: %d", numOfCOMPACTSuccessfulAttempts);
	ALERT("CENTER attempts: %d", numOfCENTERAttepmts);
	ALERT("CENTER successful attempts: %d", numOfCENTERSuccessfulAttempts);
}


TimingHPWLWatcher::TimingHPWLWatcher(HDesign& _hd, bool _watch):hd(_hd),watch(_watch)
{
	srand( (unsigned)time( NULL ) );
	int rnd=int(rand()*1000);
	sprintf(filename,"TimingHPWLWatcher%d.log",rnd);

	STA(hd,false);
	oldTNS=Utils::TNS(hd);
	oldWNS=Utils::WNS(hd);
	oldHPWL=Utils::CalculateHPWL(hd, true);
	callCounter=0;
	FILE* fOUT;
	fOUT=fopen(filename,"a");
	if (fOUT){
		fprintf(fOUT, "\n---------(%s)------------\n",(const char*)hd.cfg.ValueOf("benchmark.def"));
		fclose(fOUT);
	}else ALERT("Cant open %s in constructor", filename);
}

void TimingHPWLWatcher::doReport()
{
	if (watch){
		STA(hd,false);
		newTNS=Utils::TNS(hd);
		newWNS=Utils::WNS(hd);
		newHPWL=Utils::CalculateHPWL(hd, true);
		TNSImprovement=(oldTNS-newTNS)/oldTNS*100;
		WNSImprovement=(oldWNS-newWNS)/oldWNS*100;
		HPWLImprovement=(oldHPWL-newHPWL)/oldHPWL*100;
		if (WNSImprovement || TNSImprovement || HPWLImprovement){
			//sprintf(name, "move_s298_%d_%d_%d.def",iterationNumber,numOfPath,numOfPoint);
			//ExportDEF(hd, name);
			ALERT("%d:\tTNS:\t%f\tWNS:%f\tHPWL:%f",callCounter,TNSImprovement, WNSImprovement,HPWLImprovement);
			FILE* fOUT;
			fOUT=fopen(filename,"a");
			if (fOUT){
				//ALERT("Writing to file");
				fprintf(fOUT, "%d:\tTNS:\t%f\tWNS:%f\tHPWL:%f\n",callCounter,TNSImprovement, WNSImprovement,HPWLImprovement);
				fclose(fOUT);
			}else ALERT("Cant open %s",filename);
			callCounter++;
		}
		oldTNS=newTNS;
		oldWNS=newWNS;
		oldHPWL=newHPWL;
	}

}

void PathCallback::ProcessPath2(HDesign& hd, HCriticalPath path, int pathNumber)
{
	HCell curCell;
	LPoint curPoint;
	BBox curBBox;

	HCell prevCell;
	bool f = false;	
	bool isFirst = true;

	int numOfPoint=0;
	for (HCriticalPath::PointsEnumeratorW pointsEnumW = hd[path].GetEnumeratorW(); pointsEnumW.MoveNext();)
	{
		numOfPoint++;
		f = false;
		HPinWrapper pin  = hd[hd.Get<HTimingPoint::Pin, HPin>(pointsEnumW.TimingPoint())];

		if (isFirst) { //���� ������ ������� ������������ ����
			prevCell = pin.Cell();
		}
		else prevCell = curCell;

		curCell = pin.Cell();

		if(curCell == prevCell) { //�������� ���������
			f = true;
		}
		curPoint.col = hdpp.FindColumn(hd.GetDouble<HCell::X>(curCell));
		curPoint.row = hdpp.FindRow(hd.GetDouble<HCell::Y>(curCell));

		if((!f)||isFirst) {  //������� � ���������� �������� ������
			if(!::IsNull<HCell>(curCell)) {
				//ALERT("--Taking next point # %d", numOfPoint++));

				if(hd.cfg.ValueOf("HippocratePlacement.SWAP", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementSWAP(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.COMPACT", false)){
					curBBox = GetCurrentBBox(hdpp, hd, pointsEnumW);
					HippocratePlacementCOMPACT(hdpp, hd, curCell, curBBox, stat);						
				}
				if(hd.cfg.ValueOf("HippocratePlacement.MOVE", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementMOVE(hdpp, hd, curCell, curWnd, stat);
				}
				if(hd.cfg.ValueOf("HippocratePlacement.CENTER", false)){
					LWindow curWnd;
					GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementCENTER(hdpp, hd, curCell, curWnd, stat);						
				}
				if(hd.cfg.ValueOf("HippocratePlacement.LOCALMOVE", false)){
					//LWindow curWnd;
					//GetCurrentWindow(hdpp, RADIUS_OF_WINDOW, curCell, curWnd);
					HippocratePlacementLOCALMOVE(hdpp, hd, curCell, stat);
				}

				thpwlWatcher.doReport();
			}
		}
		isFirst = false;
	}
}

bool isCellExists(HDPGrid& hdpp, int row, int col)
{
	if ((row>=0) && (col>=0) && (row<hdpp.NumRows()) && (col<hdpp.NumCols()) && !(::IsNull(hdpp(row, col)))) return true;
	else return false;
}