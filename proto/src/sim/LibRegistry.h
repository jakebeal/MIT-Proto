/*
 * LibRegistry.h
 *
 *  Created on: Jan 27, 2010
 *      Author: gbays
 */
#include <string>
#include <vector>

#ifndef LIBREGISTRY_H_
#define LIBREGISTRY_H_

using namespace std;

typedef struct RegLine
{
	string type;
	string name;
	string libpath;
};

class LibRegistry
{
public:

	LibRegistry();
	virtual ~LibRegistry();
	LibRegistry(const LibRegistry& libreg);
	LibRegistry& operator=(const LibRegistry& libreg);

	void getLayerList(vector<RegLine>& returnVec);
	void getTimeModelList(vector<RegLine>& returnVec);
	void getDistributionList(vector<RegLine>& returnVec);

	void addToLayerList(const RegLine& regline);
	void addToTimeModelList(const RegLine& regline);
	void addToDistributionList(const RegLine& regline);

    string toString();

	static const string LAYER;
	static const string TIME_MODEL;
	static const string DISTRIBUTION;

private:
	vector<RegLine> mLayerList;
	vector<RegLine> mTimeModelList;
	vector<RegLine> mDistributionList;

};

#endif /* LIBREGISTRY_H_ */
