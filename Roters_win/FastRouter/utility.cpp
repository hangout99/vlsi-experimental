#include <stdio.h>
#include <stdlib.h>

#include "DataType.h"
#include "flute.h"
#include "DataProc.h"

void printEdge(int netID, int edgeID)
{
    int i;
    TreeEdge edge;
    TreeNode *nodes;
    
    edge = sttrees[netID].edges[edgeID];
    nodes = sttrees[netID].nodes;
    
    printf("edge %d: (%d, %d)->(%d, %d)\n", edgeID, nodes[edge.n1].x, nodes[edge.n1].y, nodes[edge.n2].x, nodes[edge.n2].y);
    for(i=0; i<=edge.route.routelen; i++)
    {
        printf("(%d, %d) ", edge.route.gridsX[i], edge.route.gridsY[i]);
    }
    printf("\n");
}

void plotTree(int netID)
{
    int i, j, grid, Zpoint, n1, n2, x1, x2, y1, y2, ymin, ymax, xmin, xmax;
    int *gridsX, *gridsY;
    RouteType routetype;
    TreeEdge *treeedge;
    TreeNode *treenodes;
    FILE *fp;

    
    xmin = ymin = 1e5;
    xmax = ymax = 0;
    
    fp=fopen("plottree", "w");
    if (fp == NULL) {
        printf("Error in opening file plottree\n");
        exit(1);
    }

    treenodes = sttrees[netID].nodes;
    for(i=0; i<sttrees[netID].deg; i++)
    {
        x1 = treenodes[i].x;
        y1 = treenodes[i].y;
        fprintf(fp, "%f %f\n", (float)x1-0.1, (float)y1);
        fprintf(fp, "%f %f\n", (float)x1, (float)y1-0.1);
        fprintf(fp, "%f %f\n", (float)x1+0.1, (float)y1);
        fprintf(fp, "%f %f\n", (float)x1, (float)y1+0.1);
        fprintf(fp, "%f %f\n", (float)x1-0.1, (float)y1);
        fprintf(fp, "\n");
    }
    for(i=sttrees[netID].deg; i<sttrees[netID].deg*2-2; i++)
    {
        x1 = treenodes[i].x;
        y1 = treenodes[i].y;
        fprintf(fp, "%f %f\n", (float)x1-0.1, (float)y1+0.1);
        fprintf(fp, "%f %f\n", (float)x1+0.1, (float)y1-0.1);
        fprintf(fp, "\n");
        fprintf(fp, "%f %f\n", (float)x1+0.1, (float)y1+0.1);
        fprintf(fp, "%f %f\n", (float)x1-0.1, (float)y1-0.1);
        fprintf(fp, "\n");
    }
        
    for(i=0; i<sttrees[netID].deg*2-3; i++)
    {
if(1)//i!=14)
{
        treeedge = &(sttrees[netID].edges[i]);
        
        n1 = treeedge->n1;
        n2 = treeedge->n2;
        x1 = treenodes[n1].x;
        y1 = treenodes[n1].y;
        x2 = treenodes[n2].x;
        y2 = treenodes[n2].y;
        xmin = min(xmin, min(x1, x2));
        xmax = max(xmax, max(x1, x2));
        ymin = min(ymin, min(y1, y2));
        ymax = max(ymax, max(y1, y2));
                
        routetype = treeedge->route.type;
        
        if(routetype==LROUTE) // remove L routing
        {
            if(treeedge->route.xFirst)
            {
                fprintf(fp, "%d %d\n", x1, y1);
                fprintf(fp, "%d %d\n", x2, y1);
                fprintf(fp, "%d %d\n", x2, y2);
                fprintf(fp, "\n");
            }
            else
            {
                fprintf(fp, "%d %d\n", x1, y1);
                fprintf(fp, "%d %d\n", x1, y2);
                fprintf(fp, "%d %d\n", x2, y2);
                fprintf(fp, "\n");
            }
        }
        else if(routetype==ZROUTE)
        {
            Zpoint = treeedge->route.Zpoint;
            if(treeedge->route.HVH)
            {
                fprintf(fp, "%d %d\n", x1, y1);
                fprintf(fp, "%d %d\n", Zpoint, y1);
                fprintf(fp, "%d %d\n", Zpoint, y2);
                fprintf(fp, "%d %d\n", x2, y2);
                fprintf(fp, "\n");
            }
            else
            {
                fprintf(fp, "%d %d\n", x1, y1);
                fprintf(fp, "%d %d\n", x1, Zpoint);
                fprintf(fp, "%d %d\n", x2, Zpoint);
                fprintf(fp, "%d %d\n", x2, y2);
                fprintf(fp, "\n");
            }
        }
        else if(routetype==MAZEROUTE)
        {
            gridsX = treeedge->route.gridsX;
            gridsY = treeedge->route.gridsY;
            for(j=0; j<=treeedge->route.routelen; j++)
            {
                fprintf(fp, "%d %d\n", gridsX[j], gridsY[j]);
            }
            fprintf(fp, "\n");
        }
}
    }
    
    fprintf(fp, "%d %d\n", xmin-2, ymin-2);
    fprintf(fp, "\n");
    fprintf(fp, "%d %d\n", xmax+2, ymax+2);
    fclose(fp);
}

void getlen()
{
    int i, edgeID, totlen=0;
    TreeEdge *treeedge;
    TreeNode *treenodes;
    
    for(i=0; i<numValidNets; i++)
    {
        treenodes = sttrees[i].nodes;
        for(edgeID=0; edgeID<2*sttrees[i].deg-3; edgeID++)
        {
            treeedge = &(sttrees[i].edges[edgeID]);
            if(treeedge->route.type<MAZEROUTE)
                printf("wrong\n");
//                totlen += ADIFF(treenodes[treeedge->n1].x, treenodes[treeedge->n2].x) + ADIFF(treenodes[treeedge->n1].y, treenodes[treeedge->n2].y);
            else
                totlen += treeedge->route.routelen;
        }
    }
    printf("Routed len: %d\n", totlen);
}


void ConvertToFull3DType2 () 
{
	int i, k, netID, edgeID, tmpX[MAXLEN], tmpY[MAXLEN],*gridsX,*gridsY,*gridsL, tmpL[MAXLEN], routeLen, n1a, n2a;
	int n1, n2,  newCNT, numVIA,deg, j;
	Route *route;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;

	numVIA = 0;
	

	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		treenodes=sttrees[netID].nodes;

		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {

				newCNT = 0;
				routeLen =  treeedge->route.routelen;
//				printf("netID %d, edgeID %d, len %d\n",netID, edgeID, routeLen);
				n1a = treeedge->n1a;
				n2a = treeedge->n2a;
				gridsX = treeedge->route.gridsX;
				gridsY = treeedge->route.gridsY;
				gridsL = treeedge->route.gridsL;
/*
				if (edgeID == treenodes[n1a].hID) {
					for (k = treenodes[n1a].botL; k < treenodes[n1a].topL; k++) {
						tmpX[newCNT] = gridsX[0];
						tmpY[newCNT] = gridsY[0];
						tmpL[newCNT] = k;
						newCNT++;
						numVIA++;
					}
				}
				*/

				//finish from n1->real route

				for (j = 0; j< routeLen; j ++)
				{
					tmpX[newCNT] = gridsX[j];
					tmpY[newCNT] = gridsY[j];
					tmpL[newCNT] = gridsL[j];
					newCNT++;
						
					if (gridsL[j] > gridsL[j+1]) {
						for (k = gridsL[j]; k > gridsL[j+1]; k--) {
							tmpX[newCNT] = gridsX[j+1];
							tmpY[newCNT] = gridsY[j+1];
							tmpL[newCNT] = k;
							newCNT++;
							numVIA++;
						}
					} else if (gridsL[j] < gridsL[j+1]){
						for (k = gridsL[j]; k < gridsL[j+1]; k++) {
							tmpX[newCNT] = gridsX[j+1];
							tmpY[newCNT] = gridsY[j+1];
							tmpL[newCNT] = k;
							newCNT++;
							numVIA++;
						}
					}
				}
				tmpX[newCNT] = gridsX[j];
				tmpY[newCNT] = gridsY[j];
				tmpL[newCNT] = gridsL[j];
				newCNT++;

				/*
				if (edgeID == treenodes[n2a].hID) {
					if (treenodes[n2a].topL != treenodes[n2a].botL)
					for (k = treenodes[n2a].topL-1; k >= treenodes[n2a].botL; k--) {
						tmpX[newCNT] = gridsX[routeLen];
						tmpY[newCNT] = gridsY[routeLen];
						tmpL[newCNT] = k;
						newCNT++;
						numVIA++;
					}
				}
				*/
				// last grid -> node2 finished

				if(treeedges[edgeID].route.type==MAZEROUTE)
				{
					free(treeedges[edgeID].route.gridsX);
					free(treeedges[edgeID].route.gridsY);
					free(treeedges[edgeID].route.gridsL);
				}
				treeedge->route.gridsX = (int*)calloc(newCNT, sizeof(int));
				treeedge->route.gridsY = (int*)calloc(newCNT, sizeof(int));
				treeedge->route.gridsL = (int*)calloc(newCNT, sizeof(int));
				treeedge->route.type = MAZEROUTE;
				treeedge->route.routelen = newCNT-1;

				for(k=0; k<newCNT; k++)
				{
					treeedge->route.gridsX[k] = tmpX[k];
					treeedge->route.gridsY[k] = tmpY[k];
					treeedge->route.gridsL[k] = tmpL[k];

				}
			}
//			printEdge3D(netID, edgeID);
		}
	}
	printf("Total number of via %d\n",numVIA);

}


static int comparePVMINX (const void *a, const void *b)
{
	if (((OrderNetPin*)a)->minX > ((OrderNetPin*)b)->minX) return 1;
	if (((OrderNetPin*)a)->minX == ((OrderNetPin*)b)->minX) return 0;
	if (((OrderNetPin*)a)->minX < ((OrderNetPin*)b)->minX) return -1;
}



static int comparePVPV (const void *a, const void *b)
{
	if (((OrderNetPin*)a)->npv > ((OrderNetPin*)b)->npv) return 1;
	if (((OrderNetPin*)a)->npv == ((OrderNetPin*)b)->npv) return 0;
	if (((OrderNetPin*)a)->npv < ((OrderNetPin*)b)->npv) return -1;
}


void netpinOrderInc()
{
	int i, j, d, n1, n2, x1, y1, x2, y2, ind, l, totalLength,xmin;
	TreeEdge *treeedges, *treeedge ;
    TreeNode *treenodes;
	StTree* stree;

	float npvalue;

	numTreeedges = 0;
	for(j=0; j<numValidNets; j++)
    {
		d = sttrees[j].deg;
		numTreeedges += 2*d -3;
	}

	if (treeOrderPV!=NULL) {
		free(treeOrderPV);
	}

	treeOrderPV = (OrderNetPin*) malloc(numValidNets*sizeof(OrderNetPin));


	i = 0;
	for(j=0; j<numValidNets; j++)
	{
		xmin = BIG_INT;
		totalLength = 0;
		treenodes=sttrees[j].nodes;
		stree = &(sttrees[j]);
		d = stree->deg;
		for(ind=0; ind<2*d-3; ind++)
		{
			totalLength += stree->edges[ind].len;
			if (xmin < treenodes[stree->edges[ind].n1].x) {
				xmin = treenodes[stree->edges[ind].n1].x;
			}

		}

		npvalue = (float) totalLength/d;

		treeOrderPV[j].npv = npvalue;
		treeOrderPV[j].treeIndex = j;
		treeOrderPV[j].minX = xmin;
	}

	qsort (treeOrderPV, numValidNets, sizeof(OrderNetPin), comparePVMINX);
	qsort (treeOrderPV, numValidNets, sizeof(OrderNetPin), comparePVPV);
}


void fillVIA()
{
	int i, k, netID, edgeID, tmpX[MAXLEN], tmpY[MAXLEN],*gridsX,*gridsY,*gridsL, tmpL[MAXLEN], routeLen, n1a, n2a;
	int n1, n2,  newCNT, numVIAT1, numVIAT2,deg, j;
	Route *route;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;

	numVIAT1 = 0;
	numVIAT2 = 0;
	

	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		treenodes=sttrees[netID].nodes;

		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {

				newCNT = 0;
				routeLen =  treeedge->route.routelen;
//				printf("netID %d, edgeID %d, len %d\n",netID, edgeID, routeLen);
				n1 = treeedge->n1;
				n2 = treeedge->n2;
				gridsX = treeedge->route.gridsX;
				gridsY = treeedge->route.gridsY;
				gridsL = treeedge->route.gridsL;

				
				n1a = treenodes[n1].stackAlias;

				
				n2a = treenodes[n2].stackAlias;

				n1a = treeedge->n1a;
				n2a = treeedge->n2a;

	
		
				if (edgeID == treenodes[n1a].hID || edgeID == treenodes[n2a].hID) {

					if (edgeID == treenodes[n1a].hID) {

						for (k = treenodes[n1a].botL; k < treenodes[n1a].topL; k++) {
							tmpX[newCNT] = gridsX[0];
							tmpY[newCNT] = gridsY[0];
							tmpL[newCNT] = k;
							newCNT++;
							if (n1a < deg) {
								numVIAT1++;
							} else {
								numVIAT2 ++;
							}
						}
					}

					//finish from n1->real route

					for (j = 0; j< routeLen; j ++)
					{
						tmpX[newCNT] = gridsX[j];
						tmpY[newCNT] = gridsY[j];
						tmpL[newCNT] = gridsL[j];
						newCNT++;
							
/*						if (gridsL[j] > gridsL[j+1]) {
							printf("fill via should not entered\n");
							for (k = gridsL[j]; k > gridsL[j+1]; k--) {
								tmpX[newCNT] = gridsX[j+1];
								tmpY[newCNT] = gridsY[j+1];
								tmpL[newCNT] = k;
								newCNT++;
								numVIA++;
							}
						} else if (gridsL[j] < gridsL[j+1]){
							printf("fill via should not entered\n");
							for (k = gridsL[j]; k < gridsL[j+1]; k++) {
								tmpX[newCNT] = gridsX[j+1];
								tmpY[newCNT] = gridsY[j+1];
								tmpL[newCNT] = k;
								newCNT++;
								numVIA++;
							}
						}
						*/
					}
					tmpX[newCNT] = gridsX[j];
					tmpY[newCNT] = gridsY[j];
					tmpL[newCNT] = gridsL[j];
					newCNT++;


				
					if (edgeID == treenodes[n2a].hID) {
						if (treenodes[n2a].topL != treenodes[n2a].botL)
						for (k = treenodes[n2a].topL-1; k >= treenodes[n2a].botL; k--) {
							tmpX[newCNT] = gridsX[routeLen];
							tmpY[newCNT] = gridsY[routeLen];
							tmpL[newCNT] = k;
							newCNT++;
							if (n2a < deg) {
								numVIAT1++;
							} else {
								numVIAT2 ++;
							}
						}
					}
					// last grid -> node2 finished

					if(treeedges[edgeID].route.type==MAZEROUTE)
					{
						free(treeedges[edgeID].route.gridsX);
						free(treeedges[edgeID].route.gridsY);
						free(treeedges[edgeID].route.gridsL);
					}
					treeedge->route.gridsX = (int*)calloc(newCNT, sizeof(int));
					treeedge->route.gridsY = (int*)calloc(newCNT, sizeof(int));
					treeedge->route.gridsL = (int*)calloc(newCNT, sizeof(int));
					treeedge->route.type = MAZEROUTE;
					treeedge->route.routelen = newCNT-1;

					for(k=0; k<newCNT; k++)
					{
						treeedge->route.gridsX[k] = tmpX[k];
						treeedge->route.gridsY[k] = tmpY[k];
						treeedge->route.gridsL[k] = tmpL[k];

					}
				}//if edgeID == treenodes[n1a].hID || edgeID == treenodes[n2a].hID
			}
//			printEdge3D(netID, edgeID);
		}
	}
	printf("via related to pin nodes %d\n",numVIAT1);
	printf("via related stiner nodes %d\n",numVIAT2);

}

int threeDVIA ()
{
	int netID, d, k, edgeID,nodeID,deg, numpoints, n1, n2, corN;;
	TreeEdge *treeedges, *treeedge;
	TreeNode *treenodes;
	int *gridsL, routeLen, n1a, n2a, numVIA, j;

	numVIA = 0;

	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		treenodes=sttrees[netID].nodes;

		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{

			treeedge = &(treeedges[edgeID]);

			if (treeedge->len > 0) {

				routeLen =  treeedge->route.routelen;
				gridsL = treeedge->route.gridsL;

				for (j = 0; j< routeLen; j ++)
				{
					if (gridsL[j]!= gridsL[j+1]) {
						numVIA++;
					}
				}
			}
		}
	}

	printf("num of vias %d\n",numVIA);
	return (numVIA);
}








void assignEdge(int netID, int edgeID, Bool processDIR) 
{

	int i, j, k , l, *gridsX, *gridsY, *gridsL, length, grid, min_x, min_y, routelen, n1a, n2a, last_layer;
	int min_result, endLayer = 0;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;

	treeedges = sttrees[netID].edges;
	treenodes = sttrees[netID].nodes;
	treeedge = &(treeedges[edgeID]);

	gridsX = treeedge->route.gridsX;
	gridsY = treeedge->route.gridsY;
	gridsL = treeedge->route.gridsL;

	
	routelen = treeedge->route.routelen;
	n1a = treeedge->n1a;
	n2a = treeedge->n2a;

	for (l = 0; l < numLayers; l ++) {
		for (k = 0; k <= routelen; k ++) {
			gridD[l][k] = BIG_INT;
			viaLink[l][k] = BIG_INT;
		}
	}


	for (k = 0; k < routelen; k ++) {
		if (gridsX[k] == gridsX[k+1]) {
			min_y = min(gridsY[k], gridsY[k+1]);
			for (l = 0; l < numLayers; l ++) {
				grid = l* gridV + min_y*xGrid+gridsX[k];
				layerGrid[l][k] = v_edges3D[grid].cap - v_edges3D[grid].usage;
			}
		} else {
			min_x =  min(gridsX[k], gridsX[k+1]);
			for (l = 0; l < numLayers; l ++) {
				grid = l* gridH + gridsY[k]*(xGrid -1)+min_x;
				layerGrid[l][k] = h_edges3D[grid].cap - h_edges3D[grid].usage;
			}
		}
	}

	if (processDIR) {
		if (treenodes[n1a].assigned) {
			for (l = treenodes[n1a].botL ; l <= treenodes[n1a].topL ; l ++) {
				gridD[l][0] = 0;
			}
		} else {
			printf("warning, start point not assigned\n");
			fflush(stdout);
		}

		for (k = 0; k < routelen; k ++) {
			for (l = 0 ; l < numLayers; l ++) {
				for (i = 0 ; i < numLayers; i++) {
					if (k == 0) {
						if (l != i) {
							if (gridD[i][k] > gridD[l][k]  + ADIFF(i,l)*2) {
								gridD[i][k] = gridD[l][k]  + ADIFF(i,l)*2;
								viaLink[i][k] = l;
							}
						}
					} else {
						if (l != i) {
							if (gridD[i][k] > gridD[l][k] + ADIFF(i,l) * 3 ) {
								gridD[i][k] = gridD[l][k] + ADIFF(i,l) * 3  ;
								viaLink[i][k] = l;
							}
						}
					}
				}
			}
			for (l = 0; l < numLayers; l ++) {
				if (layerGrid[l][k] > 0) {
					gridD[l][k+1] = gridD[l][k] + 1;
				}else {
					gridD[l][k+1] = gridD[l][k]+BIG_INT;
				}
			}
		}
		

		for (l = 0 ; l < numLayers; l ++) {
			for (i = 0 ; i < numLayers; i++) {
				if (l != i) {
					if (gridD[i][k] > gridD[l][k] + ADIFF(i,l)*1){//+ ADIFF(i,l) * 3 ) {
						gridD[i][k] = gridD[l][k] + ADIFF(i,l)*1;//+ ADIFF(i,l) * 3 ;
						viaLink[i][k] = l;
					}
				}
			}
		}

		k = routelen;


		if (treenodes[n2a].assigned) {
			min_result = BIG_INT;
			for (i = treenodes[n2a].topL; i >= treenodes[n2a].botL; i--) {
				if (gridD[i][routelen] < min_result) {
					min_result = gridD[i][routelen] ;
					endLayer = i;
				}
			}
		} else {
			min_result = gridD[0][routelen];
			endLayer = 0;
			for (i = 0; i < numLayers; i++) {
				if (gridD[i][routelen] < min_result) {
					min_result = gridD[i][routelen] ;
					endLayer = i;
				}
			}
		}

		k = routelen;
		

		if (viaLink[endLayer][routelen] == BIG_INT) {

			last_layer = endLayer;
		} else {
			last_layer = viaLink[endLayer][routelen];
		}


		for (k = routelen ; k >= 0; k --) {
			gridsL[k] = last_layer;
			if (viaLink[last_layer][k] == BIG_INT) {
				last_layer = last_layer;
			} else {
				last_layer = viaLink[last_layer][k];
			}
		}

		if (gridsL[0] < treenodes[n1a].botL ) {
			treenodes[n1a].botL = gridsL[0];
			treenodes[n1a].lID = edgeID;
		}
		if (gridsL[0] > treenodes[n1a].topL) {
			treenodes[n1a].topL = gridsL[0];
			treenodes[n1a].hID = edgeID;
		}


		k = routelen;
		if (treenodes[n2a].assigned) {
			
			if (gridsL[routelen] < treenodes[n2a].botL ) {
				treenodes[n2a].botL = gridsL[routelen];
				treenodes[n2a].lID = edgeID;
			}
			if (gridsL[routelen] > treenodes[n2a].topL) {
				treenodes[n2a].topL = gridsL[routelen];
				treenodes[n2a].hID = edgeID;
			}

		} else {
			//treenodes[n2a].assigned = TRUE;
			treenodes[n2a].topL = gridsL[routelen];//max(endLayer, gridsL[routelen]);
			treenodes[n2a].botL = gridsL[routelen];//min(endLayer, gridsL[routelen]);
			treenodes[n2a].lID = treenodes[n2a].hID = edgeID;
		}


		if (treenodes[n2a].assigned) {
			if (gridsL[routelen] > treenodes[n2a].topL || gridsL[routelen] < treenodes[n2a].botL ) {
				printf("target ending layer out of range\n");
			}			

		}

	} else {

		if (treenodes[n2a].assigned) {
			for (l = treenodes[n2a].botL ; l <= treenodes[n2a].topL ; l ++) {
				gridD[l][routelen] = 0;
			}
		} 

		for (k = routelen; k > 0; k --) {
			for (l = 0 ; l < numLayers; l ++) {
				for (i = 0 ; i < numLayers; i++) {
					if (k == routelen) {
						if (l != i) {
							if (gridD[i][k] > gridD[l][k] + ADIFF(i,l)*2) {
								gridD[i][k] = gridD[l][k] + ADIFF(i,l)*2 ;
								viaLink[i][k] = l;
							}
						}
					} else {
						if (l != i) {
							if (gridD[i][k] > gridD[l][k] + ADIFF(i,l) * 3 ) {
								gridD[i][k] = gridD[l][k] + ADIFF(i,l) * 3 ;
								viaLink[i][k] = l;
							}
						}
					}
				}
			}
			for (l = 0; l < numLayers; l ++) {
				if (layerGrid[l][k-1] > 0) {
					gridD[l][k-1] = gridD[l][k]+1;
				} else {
					gridD[l][k-1] = gridD[l][k]+BIG_INT;
				}

			}
		}
		
		
		for (l = 0 ; l < numLayers; l ++) {
			for (i = 0 ; i < numLayers; i++) {
				if (l != i) {
					if (gridD[i][0] > gridD[l][0] + ADIFF(i,l)*1) {
						gridD[i][0] = gridD[l][0] + ADIFF(i,l)*1;
						viaLink[i][0] = l;
					}
				}
			}
		}

		if (treenodes[n1a].assigned) {
			min_result = BIG_INT;
			for (i = treenodes[n1a].topL; i >= treenodes[n1a].botL; i--) {
				if (gridD[i][k] < min_result) {
					min_result = gridD[i][0] ;
					endLayer = i;
				}
			}
			
		} else {
			min_result = gridD[0][k];
			endLayer = 0;
			for (i = 0; i < numLayers; i++) {
				if (gridD[i][k] < min_result) {
					min_result = gridD[i][k] ;
					endLayer = i;
				}
			}
		}

		last_layer = endLayer;

		for (k = 0 ; k <= routelen ; k ++) {
			if (viaLink[last_layer][k] == BIG_INT) {
				last_layer = last_layer;
			} else {
				last_layer = viaLink[last_layer][k];
			}
			gridsL[k] = last_layer;
		}

		gridsL[routelen] = gridsL[routelen -1];

		if (gridsL[routelen] < treenodes[n2a].botL ) {
			treenodes[n2a].botL = gridsL[routelen];
			treenodes[n2a].lID = edgeID;

		}
		if (gridsL[routelen] > treenodes[n2a].topL) {
			treenodes[n2a].topL = gridsL[routelen];
			treenodes[n2a].hID = edgeID;
		}

		if (treenodes[n1a].assigned) {

			if (gridsL[0] < treenodes[n1a].botL ) {
				treenodes[n1a].botL = gridsL[0];
				treenodes[n1a].lID = edgeID;
			}
			if (gridsL[0] > treenodes[n1a].topL) {
				treenodes[n1a].topL = gridsL[0];
				treenodes[n1a].hID = edgeID;
			}

		} else {
			//treenodes[n1a].assigned = TRUE;
			treenodes[n1a].topL = gridsL[0];//max(endLayer, gridsL[0]);
			treenodes[n1a].botL = gridsL[0];//min(endLayer, gridsL[0]);
			treenodes[n1a].lID = treenodes[n1a].hID =edgeID;
		}
	}
	treeedge->assigned = TRUE;

	for (k = 0; k < routelen; k ++) {
		if (gridsX[k] == gridsX[k+1]) {
			min_y = min(gridsY[k], gridsY[k+1]);
			grid = gridsL[k]* gridV + min_y*xGrid+gridsX[k];
			

			if (v_edges3D[grid].usage < v_edges3D[grid].cap) {
				v_edges3D[grid].usage++;
				
			} else {
				v_edges3D[grid].usage++;
			}
				
		} else {
			min_x =  min(gridsX[k], gridsX[k+1]);
			grid = gridsL[k]* gridH + gridsY[k]*(xGrid -1)+min_x;
			
			if (h_edges3D[grid].usage < h_edges3D[grid].cap) {
				h_edges3D[grid].usage ++;
			} else {
				h_edges3D[grid].usage ++;
			}
		}
	}
	
}








void newLayerAssignmentV4() 
{
	int i, j,k, l, netID, edgeID, nodeID, *gridsX, *gridsY, *gridsL, routeLen, min_y, min_x;
	int grid,numError, preFH, preFV, n1, n2, connectionCNT, deg, tmpLayer;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;
	Bool assigned, inOne, BIdir;
	int zigP[MAXLEN], dirZig[MAXLEN],numZig, curX, nextX, preD, curD, zigHead, zigTail, preBH, preBV, n1a, n2a;
	int quehead, quetail, numNodes;
	int edgeQueue[5000];
	int nodeIndex[5000];
	int trash;
	int sumcheck = 0;

	int pinINDs[5000];
	
	numError = 0;

	for (netID = 0; netID < numValidNets; netID++) {
		treeedges = sttrees[netID].edges;
		treenodes = sttrees[netID].nodes;
		deg = sttrees[netID].deg;
		for (edgeID = 0; edgeID < 2*deg-3; edgeID++) {

			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {

				routeLen = treeedge->route.routelen;
				treeedge->route.gridsL = (int*)calloc((routeLen+1), sizeof(int));
				treeedge->assigned = FALSE;
				

			}
		}
	}
	netpinOrderInc();

	for(i=0; i<numValidNets; i++)
    {

		netID = treeOrderPV[i].treeIndex;
		treeedges = sttrees[netID].edges;
		treenodes = sttrees[netID].nodes;
		deg = sttrees[netID].deg;
		quehead = quetail = 0;
		numNodes = 2*deg-2;

		for (nodeID = 0; nodeID < deg; nodeID ++) {
			for (k = 0; k < treenodes[nodeID].conCNT; k++) {
				edgeID = treenodes[nodeID].eID[k];
				if (!treeedges[edgeID].assigned) {
					edgeQueue[quetail] = edgeID;
					treeedges[edgeID].assigned = TRUE;
					quetail++;
				}
			}
		}

		while (quehead != quetail) {
			edgeID = edgeQueue[quehead];
			treeedge = &(treeedges[edgeID]);
			sumcheck += treeedge->route.routelen;
			if (treenodes[treeedge->n1a].assigned) {
				assignEdge(netID, edgeID, 1);
				treeedge->assigned = TRUE;
				if (!treenodes[treeedge->n2a].assigned) {
					for (k = 0; k < treenodes[treeedge->n2a].conCNT; k++) {
						edgeID = treenodes[treeedge->n2a].eID[k];
						if (!treeedges[edgeID].assigned) {
							edgeQueue[quetail] = edgeID;
							treeedges[edgeID].assigned = TRUE;
							quetail++;
						}
					}
					treenodes[treeedge->n2a].assigned = TRUE;
				}
			} else {
				assignEdge(netID, edgeID, 0);
				treeedge->assigned = TRUE;
				if (!treenodes[treeedge->n1a].assigned) {
					for (k = 0; k < treenodes[treeedge->n1a].conCNT; k++) {
						edgeID = treenodes[treeedge->n1a].eID[k];
						if (!treeedges[edgeID].assigned) {
							edgeQueue[quetail] = edgeID;
							treeedges[edgeID].assigned = TRUE;
							quetail++;
						}
					}
					treenodes[treeedge->n1a].assigned = TRUE;
				}
			}
			quehead++;
		}

		deg = sttrees[netID].deg;


		for(nodeID=0;nodeID<2*deg-2;nodeID++)
		{
			treenodes[nodeID].topL = -1;
			treenodes[nodeID].botL = numLayers;
			treenodes[nodeID].conCNT = 0;
			treenodes[nodeID].hID = BIG_INT;
			treenodes[nodeID].lID = BIG_INT;
			treenodes[nodeID].status = 0;
			treenodes[nodeID].assigned = FALSE;


			if(nodeID<deg)
			{
				treenodes[nodeID].botL = 0;
				treenodes[nodeID].assigned = TRUE;
				treenodes[nodeID].status = 1;
			}

		}

		for (edgeID = 0; edgeID < 2*deg-3; edgeID++) {

			treeedge = &(treeedges[edgeID]);

			if (treeedge->len > 0) {

				routeLen = treeedge->route.routelen;

				n1 = treeedge->n1;
				n2 = treeedge->n2;
				gridsL = treeedge->route.gridsL;

				n1a = treenodes[n1].stackAlias;
				n2a = treenodes[n2].stackAlias;
				connectionCNT = treenodes[n1a].conCNT;
				treenodes[n1a].heights[connectionCNT] = gridsL[0];
				treenodes[n1a].eID[connectionCNT] = edgeID;
				treenodes[n1a].conCNT++;
				

				if (gridsL[0]>treenodes[n1a].topL) {
					treenodes[n1a].hID = edgeID;
					treenodes[n1a].topL = gridsL[0];
				}
				if (gridsL[0]<treenodes[n1a].botL) {
					treenodes[n1a].lID = edgeID;
					treenodes[n1a].botL = gridsL[0];
				}
				
				treenodes[n1a].assigned = TRUE;

				connectionCNT = treenodes[n2a].conCNT;
				treenodes[n2a].heights[connectionCNT] = gridsL[routeLen];
				treenodes[n2a].eID[connectionCNT] = edgeID;
				treenodes[n2a].conCNT++;
				if (gridsL[routeLen]>treenodes[n2a].topL) {
					treenodes[n2a].hID = edgeID;
					treenodes[n2a].topL = gridsL[routeLen];
				} 
				if (gridsL[routeLen]<treenodes[n2a].botL) {
					treenodes[n2a].lID = edgeID;
					treenodes[n2a].botL = gridsL[routeLen];
				}

				treenodes[n2a].assigned = TRUE;
			
			}//edge len > 0
		} // eunmerating edges 
	}

	//printf("sum check number 2 %d\n",sumcheck);
}


void newLA ()
{
	int netID, i,d, k, edgeID,nodeID,deg, numpoints, n1, n2, corN,  tmpX[MAXLEN], tmpY[MAXLEN],*gridsX,*gridsY,*gridsL, tmpL[MAXLEN], routeLen, n1a, n2a;;
	int n1x, n1y, n1l, n2x, n2y, n2l,  grid, numError, preH, preV, min_y, min_x,connectionCNT;
	Route *route;
	Bool redundant, newCNT, numVIA, j, l, distance;;
	TreeEdge *treeedges, *treeedge;
	TreeNode *treenodes;
	Bool assigned, inOne;
	int zigP[MAXLEN], dirZig[MAXLEN],numZig, curX, nextX, preD, curD,tmpLayer;


	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		treenodes=sttrees[netID].nodes;
		deg=sttrees[netID].deg;

		numpoints=0;

		for(d=0;d<2*deg-2;d++)
		{
			treenodes[d].topL = -1;
			treenodes[d].botL = numLayers;
			//treenodes[d].l = 0;
			treenodes[d].assigned = FALSE;
			treenodes[d].stackAlias = d;
			treenodes[d].conCNT = 0;
			treenodes[d].hID = BIG_INT;
			treenodes[d].lID = BIG_INT;
			treenodes[d].status = 0;


			if(d<deg)
			{
				treenodes[d].botL = treenodes[d].topL   = 0;
				//treenodes[d].l = 0;
				treenodes[d].assigned = TRUE;
				treenodes[d].status = 1;
			
				xcor[numpoints]=treenodes[d].x;
				ycor[numpoints]=treenodes[d].y;
				dcor[numpoints]=d;
				numpoints++;
			} else {
				redundant=FALSE;
				for(k=0;k<numpoints;k++)
				{
					if((treenodes[d].x==xcor[k])&&(treenodes[d].y==ycor[k]))
					{
						treenodes[d].stackAlias = dcor[k];
						
						redundant=TRUE;
						break;
					}
				}
				if(!redundant)
				{
					xcor[numpoints]=treenodes[d].x;
					ycor[numpoints]=treenodes[d].y;
					dcor[numpoints]=d;
					numpoints++;
				}
			}
		}
	}

	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		treenodes=sttrees[netID].nodes;
		deg = sttrees[netID].deg;

		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {

				n1 = treeedge->n1;
				n2 = treeedge->n2;

				treeedge->n1a = treenodes[n1].stackAlias;
				treenodes[treeedge->n1a ].eID[treenodes[treeedge->n1a ].conCNT] = edgeID;
				treenodes[treeedge->n1a ].conCNT++;
				treeedge->n2a =  treenodes[n2].stackAlias;
				treenodes[treeedge->n2a].eID[treenodes[treeedge->n2a].conCNT] = edgeID;
				treenodes[treeedge->n2a].conCNT++;
			}
		}
	}

	printf("node processing\n");
	newLayerAssignmentV4();
	printf("layer assignment\n");
	numVIA = 0;
	ConvertToFull3DType2() ;


}




void printEdge3D(int netID, int edgeID)
{
    int i;
    TreeEdge edge;
    TreeNode *nodes;
    
    edge = sttrees[netID].edges[edgeID];
    nodes = sttrees[netID].nodes;
    
    printf("edge %d: n1 %d (%d, %d)-> n2 %d(%d, %d)\n", edgeID, edge.n1, nodes[edge.n1].x, nodes[edge.n1].y,edge.n2, nodes[edge.n2].x, nodes[edge.n2].y);
	if (edge.len > 0) {
		for(i=0; i<=edge.route.routelen; i++)
		{
			printf("(%d, %d,%d) ", edge.route.gridsX[i], edge.route.gridsY[i],edge.route.gridsL[i]);
		}
		printf("\n");
	}
}

void printTree3D(int netID)
{
	int edgeID, nodeID;
	for(nodeID=0; nodeID<2*sttrees[netID].deg-2;nodeID++)
    {
		printf("nodeID %d,  [%d, %d]\n", nodeID,sttrees[netID].nodes[nodeID].y,sttrees[netID].nodes[nodeID].x);
	}

	for(edgeID=0; edgeID<2*sttrees[netID].deg-3; edgeID++)
    {
		printEdge3D(netID, edgeID);
	}
}


void checkRoute3D()
{
    int i, netID, edgeID,nodeID, edgelength, *gridsX, *gridsY, *gridsL;
    int n1, n2, x1, y1, x2, y2, l1, l2, deg;
    int cnt, Zpoint, distance;
	Bool gridFlag;
    TreeEdge *treeedge;
    TreeNode *treenodes;
    
    for(netID=0; netID<numValidNets; netID++)
    {

        treenodes = sttrees[netID].nodes;
		deg = sttrees[netID].deg;

		for(nodeID=0;nodeID<2*deg-2;nodeID++)
		{
			if(nodeID<deg)
			{
				if (treenodes[nodeID].botL != 0) {
					printf("causing pin node floating\n");
				}

				if (treenodes[nodeID].botL  > treenodes[nodeID].topL) {
					//printf("pin node l %d h %d wrong lid %d hid %d\n", treenodes[nodeID].botL, treenodes[nodeID].topL, treenodes[nodeID].lID, treenodes[nodeID].hID);
				}
			}
		}
        for(edgeID=0; edgeID<2*sttrees[netID].deg-3; edgeID++)
        {
			if (sttrees[netID].edges[edgeID].len == 0) {
				continue;
			}
            treeedge = &(sttrees[netID].edges[edgeID]);
            edgelength = treeedge->route.routelen;
            n1 = treeedge->n1;
            n2 = treeedge->n2;
            x1 = treenodes[n1].x;
            y1 = treenodes[n1].y;
            x2 = treenodes[n2].x;
            y2 = treenodes[n2].y;
            gridsX = treeedge->route.gridsX;
            gridsY = treeedge->route.gridsY;
			gridsL = treeedge->route.gridsL;

			gridFlag = FALSE;
            
			if(gridsX[0]!=x1 || gridsY[0]!=y1 )
            {
                printf("net[%d] edge[%d] start node wrong, net deg %d, n1 %d\n", netID, edgeID, deg, n1);
                printEdge3D(netID, edgeID);
            }
			if(gridsX[edgelength]!=x2 || gridsY[edgelength]!=y2)
            {
                printf("net[%d] edge[%d] end node wrong, net deg %d, n2 %d\n", netID, edgeID,deg, n2);
                printEdge3D(netID, edgeID);
            }
            for(i=0; i<treeedge->route.routelen; i++)
            {
				distance = ADIFF(gridsX[i+1],gridsX[i]) + ADIFF(gridsY[i+1],gridsY[i]) + ADIFF(gridsL[i+1],gridsL[i]);
                if(distance >1 || distance < 0)
                {
					gridFlag = TRUE;
                    printf("net[%d] edge[%d] maze route wrong, distance %d, i %d\n", netID, edgeID,distance,i);
					printf("current [%d, %d, %d], next [%d, %d, %d]",gridsL[i],gridsY[i],gridsX[i],gridsL[i+1],gridsY[i+1],gridsX[i+1]);
                }
				
            }

			for(i=0; i<=treeedge->route.routelen; i++) {
				if (gridsL[i] < 0) {
					printf("gridsL less than 0, %d\n",gridsL[i]);
				}
			}
			if (gridFlag) {
				printEdge3D(netID, edgeID);
			}
        }
    }
}


void write3D()
{
	int netID, d, i,k, edgeID,nodeID,deg, lastX, lastY,lastL, xreal, yreal,l, routeLen,*gridsX, *gridsY, *gridsL;
	TreeEdge *treeedges, *treeedge;
	FILE *fp;
	TreeNode *nodes;
	TreeEdge edge;
	
	fp=fopen("output.out", "w");
    if (fp == NULL) {
        printf("Error in opening %s\n", "output.out");
        exit(1);
    }

	for(netID=0;netID<numValidNets;netID++)
	{
		fprintf(fp, "%s %d\n", nets[netID]->name, netID);
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		

		nodes = sttrees[netID].nodes;
		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			edge = sttrees[netID].edges[edgeID];
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {
				
				routeLen = treeedge->route.routelen;
				gridsX = treeedge->route.gridsX;
				gridsY = treeedge->route.gridsY;
				gridsL = treeedge->route.gridsL;
				lastX = wTile*(gridsX[0]+0.5)+xcorner;
				lastY = hTile*(gridsY[0]+0.5)+ycorner;
				lastL = gridsL[0];
				for (i = 1; i <= routeLen; i ++) {
					xreal = wTile*(gridsX[i]+0.5)+xcorner;
					yreal = hTile*(gridsY[i]+0.5)+ycorner;

					fprintf(fp, "(%d,%d,%d)-(%d,%d,%d)\n", lastX,lastY,lastL+1,xreal,yreal,gridsL[i]+1);
					lastX = xreal;
					lastY = yreal;
					lastL = gridsL[i];
				}
			}
		}
		fprintf(fp, "!\n");
	}
	fclose(fp);
}





static int compareTEL (const void *a, const void *b)
{
	if (((OrderTree*)a)->xmin < ((OrderTree*)b)->xmin) return 1;
	if (((OrderTree*)a)->xmin == ((OrderTree*)b)->xmin) return 0;
	if (((OrderTree*)a)->xmin > ((OrderTree*)b)->xmin) return -1;
}



void StNetOrder()
{
	int i, j, d, n1, n2, x1, y1, x2, y2, ind, l, grid, min_x, min_y,*gridsX, *gridsY;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;
	StTree* stree;

	numTreeedges = 0;

	if (treeOrderCong != NULL) {
		free(treeOrderCong);
	}

	treeOrderCong = (OrderTree*) malloc(numValidNets*sizeof(OrderTree));

	i = 0;
	for(j=0; j<numValidNets; j++)
	{
		stree = &(sttrees[j]);
		d = stree->deg;
		treeOrderCong[j].xmin = 0;
		treeOrderCong[j].treeIndex = j;
		for(ind=0; ind<2*d-3; ind++)
		{
			treeedges = stree->edges;
            treeedge = &(treeedges[ind]);

			gridsX = treeedge->route.gridsX;
			gridsY = treeedge->route.gridsY;
			for(i=0; i<=treeedge->route.routelen; i++)
			{
				if(gridsX[i]==gridsX[i+1]) // a vertical edge
				{
					min_y = min(gridsY[i], gridsY[i+1]);
					grid = min_y*xGrid+gridsX[i];
					treeOrderCong[j].xmin += max(0, v_edges[grid].usage - v_edges[grid].cap);
				}
				else ///if(gridsY[i]==gridsY[i+1])// a horizontal edge
				{
					min_x = min(gridsX[i], gridsX[i+1]);
					grid = gridsY[i]*(xGrid-1)+min_x;
					treeOrderCong[j].xmin += max(0, h_edges[grid].usage - h_edges[grid].cap);
				}
			}

		}
	}

	qsort (treeOrderCong, numValidNets, sizeof(OrderTree), compareTEL);
}



void recoverEdge(int netID, int edgeID)
{
    int i, k, grid, Zpoint, ymin, ymax, xmin, lv, lh, n1a, n2a, hl, bl, hid, bid, deg;
    int *gridsX, *gridsY, *gridsL, connectionCNT, routeLen;
    RouteType ripuptype;
	TreeEdge *treeedges, *treeedge;
    TreeNode *treenodes;


	treeedges = sttrees[netID].edges;
	treeedge = &(treeedges[edgeID]);

	routeLen = treeedge->route.routelen;


	if(treeedge->len==0) {
		printf("trying to recover an 0 length edge\n");
		exit(0);
	}

	treenodes = sttrees[netID].nodes;

	gridsX = treeedge->route.gridsX;
	gridsY = treeedge->route.gridsY;
	gridsL = treeedge->route.gridsL;

	deg =  sttrees[netID].deg;

	n1a = treeedge->n1a;
	n2a = treeedge->n2a;

	connectionCNT = treenodes[n1a].conCNT;
	treenodes[n1a].heights[connectionCNT] = gridsL[0];
	treenodes[n1a].eID[connectionCNT] = edgeID;
	treenodes[n1a].conCNT++;
	

	if (gridsL[0]>treenodes[n1a].topL) {
		treenodes[n1a].hID = edgeID;
		treenodes[n1a].topL = gridsL[0];
	}
	if (gridsL[0]<treenodes[n1a].botL) {
		treenodes[n1a].lID = edgeID;
		treenodes[n1a].botL = gridsL[0];
	}
	
	treenodes[n1a].assigned = TRUE;

	connectionCNT = treenodes[n2a].conCNT;
	treenodes[n2a].heights[connectionCNT] = gridsL[routeLen];
	treenodes[n2a].eID[connectionCNT] = edgeID;
	treenodes[n2a].conCNT++;
	if (gridsL[routeLen]>treenodes[n2a].topL) {
		treenodes[n2a].hID = edgeID;
		treenodes[n2a].topL = gridsL[routeLen];
	} 
	if (gridsL[routeLen]<treenodes[n2a].botL) {
		treenodes[n2a].lID = edgeID;
		treenodes[n2a].botL = gridsL[routeLen];
	}

	
	treenodes[n2a].assigned = TRUE;

	for(i=0; i<treeedge->route.routelen; i++)
	{
		if (gridsL[i] ==  gridsL[i+1]) {
			if(gridsX[i]==gridsX[i+1]) // a vertical edge
			{
				ymin = min(gridsY[i], gridsY[i+1]);
				grid = gridsL[i]*gridV+ymin*xGrid+gridsX[i];
				v_edges3D[grid].usage += 1;
			}
			else if(gridsY[i]==gridsY[i+1])// a horizontal edge
			{
				xmin = min(gridsX[i], gridsX[i+1]);
				grid = gridsL[i]*gridH+gridsY[i]*(xGrid-1)+xmin;
				h_edges3D[grid].usage += 1;
			} 
		}
	}

}


 
void checkUsage()
{
	int netID, d, i,k, edgeID,nodeID,deg, lastX, lastY,lastL, xreal, yreal,l, routeLen,*gridsX, *gridsY, *gridsL;
	int j, grid ,xmin,ymin, tmp_gridsX[XRANGE], tmp_gridsY[XRANGE], cnt;
	Bool redsus;
	TreeEdge *treeedges, *treeedge;
	FILE *fp;
	TreeNode *nodes;
	TreeEdge edge;
	

	for(netID=0;netID<numValidNets;netID++)
	{
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		
		nodes = sttrees[netID].nodes;
		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			edge = sttrees[netID].edges[edgeID];
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {
				
				gridsX = treeedge->route.gridsX;
				gridsY = treeedge->route.gridsY;

				redsus = TRUE;

				while (redsus) {
					redsus = FALSE;

					for(i=0; i<=treeedge->route.routelen; i++)
					{
						for (j = 0; j < i; j++) {
							if(gridsX[i]==gridsX[j] && gridsY[i]==gridsY[j]) // a vertical edge
							{
								cnt = 1;
								for (k = i+1; k <= treeedge->route.routelen; k++) {
									gridsX[j+cnt] = gridsX[k];
									gridsY[j+cnt] = gridsY[k];
									cnt++;
								}
								treeedge->route.routelen -= i-j;
								redsus = TRUE;
								i = 0;
								j=0;
								printf("redundant edge component discovered\n");
							}
						}

					}
				}
			}
		}
	}


	printf("usage checked\n");
}



static int compareEdgeLen (const void *a, const void *b)
{
	if (((OrderNetEdge*)a)->length < ((OrderNetEdge*)b)->length) return 1;
	if (((OrderNetEdge*)a)->length == ((OrderNetEdge*)b)->length) return 0;
	if (((OrderNetEdge*)a)->length > ((OrderNetEdge*)b)->length) return -1;
}



void netedgeOrderDec(int netID)
{
	int j, d, numTreeedges;
 
	d = sttrees[netID].deg;
	numTreeedges = 2*d -3;

	for(j=0; j<numTreeedges; j++)
	{
		netEO[j].length = sttrees[netID].edges[j].route.routelen;
		netEO[j].edgeID = j;
	}

	qsort (netEO, numTreeedges, sizeof(OrderNetEdge), compareEdgeLen);
}

void printEdge2D(int netID, int edgeID)
{
    int i;
    TreeEdge edge;
    TreeNode *nodes;
    
    edge = sttrees[netID].edges[edgeID];
    nodes = sttrees[netID].nodes;
    
    printf("edge %d: n1 %d (%d, %d)-> n2 %d(%d, %d), routeType %d\n", edgeID, edge.n1, nodes[edge.n1].x, nodes[edge.n1].y,edge.n2, nodes[edge.n2].x, nodes[edge.n2].y, edge.route.type);
	if (edge.len > 0) {
		for(i=0; i<=edge.route.routelen; i++)
		{
			printf("(%d, %d) ", edge.route.gridsX[i], edge.route.gridsY[i]);
		}
		printf("\n");
	}
}

void printTree2D(int netID)
{
	int edgeID, nodeID;
	for(nodeID=0; nodeID<2*sttrees[netID].deg-2;nodeID++)
    {
		printf("nodeID %d,  [%d, %d]\n", nodeID,sttrees[netID].nodes[nodeID].y,sttrees[netID].nodes[nodeID].x);
	}

	for(edgeID=0; edgeID<2*sttrees[netID].deg-3; edgeID++)
    {
		printEdge2D(netID, edgeID);
	}
}



Bool checkRoute2DTree(int netID)
{
    int i, edgeID, edgelength, *gridsX, *gridsY;
    int n1, n2, x1, y1, x2, y2;
    int cnt, Zpoint, distance;
	Bool STHwrong, gridFlag, outrangeFlag;
    TreeEdge *treeedge;
    TreeNode *treenodes;
    
	STHwrong = FALSE;
    
    treenodes = sttrees[netID].nodes;
    for(edgeID=0; edgeID<2*sttrees[netID].deg-3; edgeID++)
    {
		treeedge = &(sttrees[netID].edges[edgeID]);
        edgelength = treeedge->route.routelen;
        n1 = treeedge->n1;
        n2 = treeedge->n2;
        x1 = treenodes[n1].x;
        y1 = treenodes[n1].y;
        x2 = treenodes[n2].x;
        y2 = treenodes[n2].y;
        gridsX = treeedge->route.gridsX;
        gridsY = treeedge->route.gridsY;

		gridFlag = FALSE;
		outrangeFlag = FALSE;

		if (treeedge->len < 0) { 
			printf("rip upped edge without edge len re assignment\n");
			STHwrong = TRUE;
		}



		if (treeedge->len > 0) {

			if ( treeedge->route.routelen < 1) { 
				printf(".routelen %d len  %d\n",treeedge->route.routelen, treeedge->len);
				STHwrong = TRUE;
				printf("checking failed %d\n", netID);
				return (TRUE);
			}
            
			if(gridsX[0]!=x1 || gridsY[0]!=y1 )
			{
				printf("initial grid wrong y1 x1 [%d %d] , net start [%d %d] routelen %d\n ",y1,x1,gridsY[0],gridsX[0], treeedge->route.routelen );
				STHwrong = TRUE;
			}
			if(gridsX[edgelength]!=x2 || gridsY[edgelength]!=y2 )
			{
				printf("end grid wrong y2 x2 [%d %d] , net start [%d %d] routelen %d\n ",y1,x1,gridsY[edgelength],gridsX[edgelength],treeedge->route.routelen );
				STHwrong = TRUE;
			}
			for(i=0; i<treeedge->route.routelen; i++)
			{

				distance = ADIFF(gridsX[i+1],gridsX[i]) + ADIFF(gridsY[i+1],gridsY[i]) ;
				if(distance > 1 || distance < 0)
				{
					printf("net[%d] edge[%d] maze route wrong, distance %d, i %d\n", netID, edgeID,distance,i);
					gridFlag = TRUE;
					STHwrong = TRUE;
				}
			}

			if (gridFlag) {
				printEdge2D(netID, edgeID);
			}
			if (STHwrong) {
				printf("checking failed %d\n", netID);
				return (TRUE);
			} 
		}
    }

	return (STHwrong);
}



void writeRoute3D(char routingfile3D[])
{
	int netID, d, i,k, edgeID,nodeID,deg, lastX, lastY,lastL, xreal, yreal,l, routeLen,*gridsX, *gridsY, *gridsL;
	TreeEdge *treeedges, *treeedge;
	FILE *fp;
	TreeNode *nodes;
	TreeEdge edge;
	
	fp=fopen(routingfile3D, "w");
    if (fp == NULL) {
        printf("Error in opening %s\n", routingfile3D);
        exit(1);
    }

	for(netID=0;netID<numValidNets;netID++)
	{
		fprintf(fp, "%s %d\n", nets[netID]->name, netID);
		treeedges=sttrees[netID].edges;
		deg=sttrees[netID].deg;
		

		nodes = sttrees[netID].nodes;
		for(edgeID = 0 ; edgeID < 2*deg-3; edgeID++)
		{
			edge = sttrees[netID].edges[edgeID];
			treeedge = &(treeedges[edgeID]);
			if (treeedge->len > 0) {
				
				routeLen = treeedge->route.routelen;
				gridsX = treeedge->route.gridsX;
				gridsY = treeedge->route.gridsY;
				gridsL = treeedge->route.gridsL;
				lastX = wTile*(gridsX[0]+0.5)+xcorner;
				lastY = hTile*(gridsY[0]+0.5)+ycorner;
				lastL = gridsL[0];
				for (i = 1; i <= routeLen; i ++) {
					xreal = wTile*(gridsX[i]+0.5)+xcorner;
					yreal = hTile*(gridsY[i]+0.5)+ycorner;

					fprintf(fp, "(%d,%d,%d)-(%d,%d,%d)\n", lastX,lastY,lastL+1,xreal,yreal,gridsL[i]+1);
					lastX = xreal;
					lastY = yreal;
					lastL = gridsL[i];
				}
			}
		}
		fprintf(fp, "!\n");
	}
	fclose(fp);
}





float *pH;
float *pV;
struct BBox *netBox;
struct BBox **pnetBox;



struct TD{
	int id;
	float cost;
};

struct BBox{
	int xmin;
	int ymin;
	int xmax;
	int ymax;
	int hSpan;
	int vSpan;
};//lower_left corner and upper_right corner



struct wire
{
    int x1,y1,x2,y2;
    int netID;
};


static int ordercost(const void *a,  const void *b)
{
    struct TD *pa, *pb;
    
    pa = *(struct TD**)a;
    pb = *(struct TD**)b;
    
    if (pa->cost < pb->cost) return 1;
    if (pa->cost > pb->cost) return -1;
    return 0;
   // return ((struct Segment*)a->x1-(struct Segment*)b->x1);
}//decreasing order

static int ordervSpan(const void *a,  const void *b)
{
    struct BBox *pa, *pb;
    
    pa = *(struct BBox**)a;
    pb = *(struct BBox**)b;
    
    if (pa->vSpan < pb->vSpan) return -1;
    if (pa->vSpan > pb->vSpan) return 1;
    return 0;
   // return ((struct Segment*)a->x1-(struct Segment*)b->x1);
}

static int orderhSpan(const void *a,  const void *b)
{
    struct BBox *pa, *pb;
    
    pa = *(struct BBox**)a;
    pb = *(struct BBox**)b;
    
    if (pa->hSpan < pb->hSpan) return -1;
    if (pa->hSpan > pb->hSpan) return 1;
    return 0;
   // return ((struct Segment*)a->x1-(struct Segment*)b->x1);
}

// binary search to map the new coordinates to original coordinates


void ACE_assignment_vertical(int);
void ACE_assignment_horizontal(int);
void vcapinitial();

void ACE() {

	int i, j, netID, edgeID, bboxID, vID, hID, deg;
	int n1, n2, n1x, n1y, n2x, n2y, xmin, xmax, ymin, ymax;
	int numTwoPinNets;
	int grid;
	TreeEdge *treeedges, *treeedge;
	TreeNode *treenodes;
	
	numTwoPinNets = 0;
	for(netID = 0; netID < numValidNets; netID ++) {
		deg = sttrees[netID].deg;
		numTwoPinNets += 2*deg-3;
	}
	pV = (float*)malloc(sizeof(float)*(yGrid-1)*xGrid);
	pH = (float*)malloc(sizeof(float)*yGrid*(xGrid-1));
	netBox = (struct BBox*)malloc(sizeof(struct BBox)*numTwoPinNets);
	pnetBox = (struct BBox**)malloc(sizeof(struct BBox*)*numTwoPinNets);
	bboxID=0;
	for(netID = 0; netID < numValidNets; netID ++) {
		deg = sttrees[netID].deg;
		treeedges = sttrees[netID].edges;
		treenodes = sttrees[netID].nodes;
		for(edgeID = 0; edgeID < 2*deg-3; edgeID++) {
			treeedge=&treeedges[edgeID];
			n1 = treeedge->n1;
			n2 = treeedge->n2;
			n1x = treenodes[n1].x;
			n1y = treenodes[n1].y;
			n2x = treenodes[n2].x;
			n2y = treenodes[n2].y;
			if(n1y<=n2y) {
				ymin = n1y;
				ymax = n2y;
			}
			else {
				ymin = n2y;
				ymax = n1y;
			}
			if(n1x<=n2x) {
				xmin = n1x;
				xmax = n2x;
			}
			else {
				xmin = n2x;
				xmax = n1x;
			}
			netBox[bboxID].xmin = xmin;
			netBox[bboxID].xmax = xmax;
			netBox[bboxID].ymin = ymin;
			netBox[bboxID].ymax = ymax;
			netBox[bboxID].vSpan = xmax-xmin+1;
			netBox[bboxID].hSpan = ymax-ymin+1;
			pnetBox[bboxID] = &netBox[bboxID];
			bboxID++;
		}
	}//get 2-pin net bounding box
	if(bboxID != numTwoPinNets) {
		printf("Bounding box number is not equivalent!!!");
	}

	for (i=0; i<yGrid-1; i++) {
		for (j=0; j<xGrid; j++) {
			grid = i*xGrid+j;		
			pV[grid] = 0;
		}
	}
	for (i=0; i<yGrid; i++) {
		for (j=0; j<xGrid-1; j++) {
			grid = i*(xGrid-1)+j;		
			pH[grid] = 0;
		}
	}//initialize predicted usage to 0
	printf("initialization done\n\n");
	qsort(pnetBox, numTwoPinNets, sizeof(struct BBox *), ordervSpan);
	for(bboxID=0; bboxID < numTwoPinNets; bboxID++) {
		ACE_assignment_vertical(bboxID);
	}
	qsort(pnetBox, numTwoPinNets, sizeof(struct BBox *), orderhSpan);
	for(bboxID=0; bboxID < numTwoPinNets; bboxID++) {
		ACE_assignment_horizontal(bboxID);
	}
	vcapinitial();
	free(pV);
	free(pH);
	free(netBox);
	free(pnetBox);
}

void ACE_assignment_vertical(int bboxID) {

	int i, j, k, t, grid;
	int id;
	int xmin, xmax, ymin, ymax, vSpan;
	float totalCost,curCost;
	struct TD *esort;
	struct TD **pesort;
	
	xmin = pnetBox[bboxID]->xmin;
	xmax = pnetBox[bboxID]->xmax;
	ymin = pnetBox[bboxID]->ymin;
	ymax = pnetBox[bboxID]->ymax;
	vSpan = pnetBox[bboxID]->vSpan;
	esort = (struct TD*)malloc(sizeof(struct TD)*vSpan);
	pesort = (struct TD**)malloc(sizeof(struct TD*)*vSpan);
	for(i=ymin; i<ymax; i++) {
		totalCost=0;
		for(j=0; j<vSpan; j++) {
			grid=i*xGrid+j+xmin;
			esort[j].cost=pV[grid]-v_edges[grid].cap+vCapacity;
			totalCost+=esort[j].cost;
			esort[j].id=j;
			pesort[j]=&esort[j];
		}
		qsort(pesort, vSpan, sizeof(struct TD *), ordercost);
		//sort edges in decreasing order
		k=0;
		while(k<vSpan) {
			curCost=(float)((totalCost+1)/(vSpan-k));
			if(curCost>pesort[k]->cost) {//check if an even assignment is possible
				for(t=k; t<vSpan; t++) {
					id=pesort[t]->id;
					grid=i*xGrid+id+xmin;
					pV[grid]=curCost+v_edges[grid].cap-vCapacity;
				}
				break;
			}
			else {
				totalCost-=pesort[k]->cost;
				k++;
				while(pesort[k-1]->cost==pesort[k]->cost) {
					k++;
					totalCost-=pesort[k]->cost;
				}
			}
		}
	}
	free(esort);
	free(pesort);
}

void ACE_assignment_horizontal(int bboxID) {

	int i, j, k, t, grid;
	int id;
	int xmin, xmax, ymin, ymax, hSpan;
	float totalCost,curCost;
	struct TD *esort;
	struct TD **pesort;

    xmin = pnetBox[bboxID]->xmin;
	xmax = pnetBox[bboxID]->xmax;
	ymin = pnetBox[bboxID]->ymin;
	ymax = pnetBox[bboxID]->ymax;
	hSpan = pnetBox[bboxID]->hSpan;
	esort = (struct TD*)malloc(sizeof(struct TD)*hSpan);
	pesort = (struct TD**)malloc(sizeof(struct TD*)*hSpan);

	for(j=xmin; j<xmax; j++) {
		totalCost=0;
		for(i=0; i<hSpan; i++) {
			grid=(i+ymin)*(xGrid-1)+j;
			esort[i].cost=pH[grid]-h_edges[grid].cap+hCapacity;
			totalCost+=esort[i].cost;
			esort[i].id=i;
			pesort[i]=&esort[i];
		}
		qsort(pesort, hSpan, sizeof(struct TD *), ordercost);
		//sort edges in decreasing order
		k=0;
		while(k<hSpan) {
			curCost=(float)((totalCost+1)/(hSpan-k));
			if(curCost>pesort[k]->cost) {//check if an even assignment is possible
				for(t=k; t<hSpan; t++) {
					id=pesort[t]->id;
					grid=(id+ymin)*(xGrid-1)+j;
					pH[grid]=curCost+h_edges[grid].cap-hCapacity;
				}
				break;
			}
			else {
				totalCost-=pesort[k]->cost;
				k++;
				while(pesort[k-1]->cost==pesort[k]->cost) {
					k++;
					totalCost-=pesort[k]->cost;
				}
			}
		}		
	}
	free(esort);
	free(pesort);
}
		
void vcapinitial() {
	int i, j, grid;
	float pof;

	for(i=0; i<yGrid; i++) {
		for(j=0; j<xGrid-1; j++) {
			grid=i*(xGrid-1)+j;
			pof=pH[grid]-h_edges[grid].cap;
			if(pof>0) {
				h_edges[grid].last_usage+=(long)pof;
			}
		}
	}//horizontal
	for(i=0; i<yGrid-1; i++) {
		for(j=0; j<xGrid; j++) {
			grid=i*xGrid+j;
			pof=pV[grid]-v_edges[grid].cap;
			if(pof>0) {
				v_edges[grid].last_usage+=(long)pof;
			}
		}
	}//vertical
}
	
