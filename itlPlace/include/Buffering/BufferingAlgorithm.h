#ifndef __BufferingAlgorithm_H__
#define __BufferingAlgorithm_H__

#include "NetBufferingAlgorithm.h"

class HVGAlgorithm
{
public:
    HVGAlgorithm(HDesign& hd);
    void NetlistBuffering();
    void RemoveNewBuffering();

private:
    int typeNetlistBuffering;
    HDesign& design;
};

#endif