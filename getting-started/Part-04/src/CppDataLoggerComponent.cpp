 /******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CppDataLoggerComponent.cpp
 *
 *  Created on: Jun, 2019
 *      Author: Eduard Muenz
 *
 ******************************************************************************/

#include "CppDataLoggerComponent.hpp"
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"

namespace CppDataLogger
{

void CppDataLoggerComponent::Initialize()
{
    // never remove next line
    ProgramComponentBase::Initialize();

    // subscribe events from the event system (Nm) here
}

void CppDataLoggerComponent::LoadConfig()
{
    // load project config here
}

void CppDataLoggerComponent::SetupConfig()
{
    // never remove next line
    ProgramComponentBase::SetupConfig();

    // setup project config here
}

void CppDataLoggerComponent::ResetConfig()
{
    // never remove next line
    ProgramComponentBase::ResetConfig();

    // implement this inverse to SetupConfig() and LoadConfig()
}

void CppDataLoggerComponent::Start(void) {
	xStopThread = false;
	Log::Info("[CppDataLoggerComponent]-------------------------------workerThreadInstance start");
	workerThreadInstance.Start();
	Log::Info("[CppDataLoggerComponent]-------------------------------DataLoggerService started");
}

void CppDataLoggerComponent::Stop(void) {
	// if you want to stop some loops of your thread during execution
	// add something like "stoptheThread" before executing workerThreadStop.
	xStopThread = true;

	Log::Info("[CppDataLoggerComponent]-------------------------------workerThreadInstance stop");
	workerThreadInstance.Stop();
	Log::Info("[CppDataLoggerComponent]-------------------------------DataLoggerService stopped");
}


bool CppDataLoggerComponent::Init()
{
	if(m_bInitialized)  // If already initialized, don't execute initialization again
	{
		return(true);
	}

	bool bRet = false;

	m_pDataLoggerService = ServiceManager::GetService<IDataLoggerService>();     //get IDataLoggerService

		if(m_pDataLoggerService != NULL) //if IDataLoggerService is valid
			{

				////////////////////////////////////////////////////////////////////
				//This is the ListSessionNames service call of DataLogger service.//
				////////////////////////////////////////////////////////////////////

				//Result vector of sessions names started by DataLogger Service
				std::vector<Arp::String> sessions;

				this->m_pDataLoggerService->ListSessionNames(IDataLoggerService::ListSessionNamesResultDelegate::create([&](IRscReadEnumerator<RscString<512>> &enumerator)
				{
					size_t nVariables = enumerator.BeginRead();
					sessions.reserve(nVariables);
					RscString<512> current;
					while(enumerator.ReadNext(current))
					{
						sessions.push_back(current.CStr());
						Log::Info("[CppDataLoggerComponent] Session-Name inside DataLoggerServices is: {0}", current.CStr());
					}
					enumerator.EndRead();
				}));


				//////////////////////////////////////////////////////////////////////////
				//This is the GetLoggedVariables service call of DataLogger service.    //
				//The service call Queries all info about logged variables of a session.//
				//////////////////////////////////////////////////////////////////////////

				//Name of session to query logged variables
			 	Arp::String sessionname = sessions[0] ; // The array element "sessions[0]" contains the current session name, the content is set by Service Call "ListSessionNames"

				//Result vector of Logged variables
				std::vector<Arp::Plc::Gds::Services::VariableInfo> VariableInfos;

				//Vector for Variable Names, sorted by name. This vector will be necessary in the next part of this article
				std::vector<Arp::String> CountingVariableNames = {};

				ErrorCode error = this->m_pDataLoggerService->GetLoggedVariables(sessionname, IDataLoggerService::GetLoggedVariablesInfosDelegate::create([&](IRscReadEnumerator<Arp::Plc::Gds::Services::VariableInfo> &enumerator)
				{

			        size_t nVariables = enumerator.BeginRead();
			        VariableInfos.reserve(nVariables);
			        VariableInfo current;

			        std::vector<std::string> stringarray; //this is the temp-vector for sorting of variables

			        while (enumerator.ReadNext(current))
			        {
			        	stringarray.push_back(Arp::String(current.Name)); //copy the Log-Vaiable-Name and Event-Variable-Name to this vector

			            VariableInfos.push_back(current); //save all information about logg-variables in this vector (only for information in output.log data)
			            Log::Info("[CppDataLoggerComponent] Returned list of variables contain {0}, {1}", current.Name, current.Type);
			        }
			        enumerator.EndRead();

			        std::sort(stringarray.begin(), stringarray.end()); //sort the names in the string array by name

			        int iCnt=0;
			        for (const auto& km : stringarray) //if the names are sorted, store it in the CountingVariableNames-Vector (as parameter for the method "ReadVariablesDataToByte which will be implemented in the next part)"
			        {
			        	CountingVariableNames.push_back(Arp::String(stringarray[iCnt]));
			        	Log::Info("[CppDataLoggerComponent] Returned list of sorted variables contain {0}", CountingVariableNames[iCnt].CStr());
			        	iCnt++;
			        }
				}));


				//////////////////////////////////////////////////////////////////////////////
				//This is the GetSessionNames service call of DataLogger service. 		    //
				//The Service Call retrieves names of sessions which log assigned variables.//
				//////////////////////////////////////////////////////////////////////////////

				//Name of variable to which corresponding sessions should be found
			    Arp::String currentVariableName = CountingVariableNames[0]; // The array element "CountingVariableNames[0]" contains the logged variable name, the content is set by Service Call "GetLoggedVariables"

				//Result vector for Session Names, contained the logged variable
			    std::vector<Arp::Plc::Gds::Services::RscString<512>> SessionInfos;


			    this->m_pDataLoggerService->GetSessionNames(currentVariableName, IDataLoggerService::GetSessionNamesResultDelegate::create([&](IRscReadEnumerator<Arp::Plc::Gds::Services::RscString<512>> &enumerator)
			    {
			    	size_t nSessions = enumerator.BeginRead();
			    	SessionInfos.reserve(nSessions);
			    	RscString<512> currentSession;

			    	while (enumerator.ReadNext(currentSession))
			    	{
			    		SessionInfos.push_back(currentSession); //save all session names in this vector
			    		Log::Info("[CppDataLoggerComponent] Session Name, contained the logged variable ''{0}'' is: {1}", currentVariableName.CStr(), currentSession.CStr());
			    	}
			    	enumerator.EndRead();

			    }));

				m_bInitialized = true;	//set the m_bInitialized flag to "true"
				bRet = true;
			}
		else
		{
			Log::Error("[CppDataLoggerComponent] ServiceManager::GetService<IDataLoggerService>() returned error");
		}
	return(bRet);
}



/// Thread Body
void CppDataLoggerComponent::workerThreadBody(void) {

	// you can check the log messages in the local log-file of this application, usually in a sub folder named "Logs"
	if(!m_bInitialized) // If not initialized
	{
		Init();  //Call Init() function
	}
}

} // end of namespace CppDataLogger
