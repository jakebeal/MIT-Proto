/*
 * LibRegistry.cpp
 *
 *  Created on: Jan 27, 2010
 *      Author: gbays
 */

#include <iostream>
#include "LibRegistry.h"

const string LibRegistry::LAYER = "Layer";
const string LibRegistry::TIME_MODEL = "TimeModel";
const string LibRegistry::DISTRIBUTION = "Distribution";

LibRegistry::LibRegistry() {}

LibRegistry::~LibRegistry() {}

LibRegistry::LibRegistry(const LibRegistry& other)
{
	mLayerList = other.mLayerList;
	mTimeModelList = other.mTimeModelList;
	mDistributionList = other.mDistributionList;
}

LibRegistry& LibRegistry::operator=(const LibRegistry& other)
{

	if (&other == this)
	{
		return *this;
	}
	mLayerList = other.mLayerList;
	mTimeModelList = other.mTimeModelList;
	mDistributionList = other.mDistributionList;

	return *this;
}

void LibRegistry::getLayerList(vector<RegLine>& returnVec)
{
	returnVec = mLayerList;
}

void LibRegistry::getTimeModelList(vector<RegLine>& returnVec)
{
	returnVec = mTimeModelList;
}

void LibRegistry::getDistributionList(vector<RegLine>& returnVec)
{
	returnVec = mDistributionList;
}

void LibRegistry::addToLayerList(const RegLine& regline)
{
	mLayerList.push_back(regline);
}

void LibRegistry::addToTimeModelList(const RegLine& regline)
{
	mTimeModelList.push_back(regline);
}

void LibRegistry::addToDistributionList(const RegLine& regline)
{
	mDistributionList.push_back(regline);
}

string LibRegistry::toString()
{
   string returnStr = "";
   vector<RegLine>::iterator it;

   for (it = mLayerList.begin();it != mLayerList.end();it++)
   {
      returnStr += it->type + " " + it->name + " " + it->libpath + "\n";
   }

   for (it = mTimeModelList.begin();it != mTimeModelList.end();it++)
   {
      returnStr += it->type + " " + it->name + " " + it->libpath + "\n";
   }

   for (it = mDistributionList.begin();it != mDistributionList.end();it++)
   {
      returnStr += it->type + " " + it->name + " " + it->libpath + "\n";
   }

   return returnStr;
}
