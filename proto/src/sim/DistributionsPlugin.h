/* 
 * File:   DistributionsPlugin.h
 * Author: prakash
 *
 * Created on February 5, 2010, 6:47 PM
 */

#ifndef _DISTRIBUTIONSPLUGIN_H
#define	_DISTRIBUTIONSPLUGIN_H

#include "ProtoPluginLibrary.h"
#include <vector>
using namespace std;

class DistributionsPlugin : public ProtoPluginLibrary
{
private:
    static vector<string> knownDistributions;
public:
    DistributionsPlugin();
    Distribution* get_distribution(char* name,
                                                    Args* args,
                                                    SpatialComputer* cpu,
                                                    int n);
};

#endif	/* _DISTRIBUTIONSPLUGIN_H */

