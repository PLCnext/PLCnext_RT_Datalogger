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
	 	sessionname = sessions[0] ; // The array element "sessions[0]" contains the current session name, the content is set by Service Call "ListSessionNames"

		//Result vector of Logged variables
		std::vector<Arp::Plc::Gds::Services::VariableInfo> VariableInfos;

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


//////////////////////////////////////////////////////////////////////////////////////////
//This is the ReadVariablesDataToByte method with ReadVariablesData service call of   	//
//DataLogger service. The Service Call reads the data from the given variable from 		//
//the session. This service function returns the data values from the passed variable 	//
//names including timestamps and data series consistent flags, which is called a record.//
//																						//
//In a record the values are in a static order and doesn't contain any type information.//
//Each record starts with the timestamp followed by the values from the given variable 	//
//by names and ends with the consistent flag.											//
//////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CppDataLoggerComponent::ReadVariablesDataToByte(const Arp::String& sessionName,
    const Arp::DateTime& startTime, const Arp::DateTime& endTime,
    const std::vector<Arp::String>& variableNames, uint8* byteMemory)
{
    IDataLoggerService::ReadVariablesDataValuesDelegate readValuesDelegate =
        IDataLoggerService::ReadVariablesDataValuesDelegate::create([&](
            IRscReadEnumerator<RscVariant<512>>& readEnumerator)
    {
    	size_t r_offset = 0; 						//reinitialize the r_offset
    	memset(byteMemory, 0x00, sizeof(byteMemory));  //reinitialize the byteMemory array

        // The readEnumerator gets the N-records,
        // the number of record is not available, the records come as N (undefined) Records!
        readEnumerator.BeginRead();
        RscVariant<512> currentVariant;

        while (readEnumerator.ReadNext(currentVariant))
        {
            RscType rscType = currentVariant.GetType();

            // Check if the rscType is a Array,
            // if yes -> the next record is founded
            if (rscType == RscType::Array)
            {
                RscArrayReader arrayReader(currentVariant); //read currentVariant into arrayReader
                size_t arraySize = arrayReader.GetSize();   //Get the size of Array

                for (size_t i = 0; i < arraySize; i++)  // for each element in the array
                {
                	// The Value will be copied into variant
                    RscVariant<512> valueTmp;
                	arrayReader.ReadNext(valueTmp);

                	// Each RscType should be check separately
                	// The following data types are expected: DateTime, Bool, Uint64 and Void(NULL)
                	switch (valueTmp.GetType())
                	{
						case RscType::DateTime:  //if the DataType is DateTime
						{
							/*Start of dummy Code: Only for Output of TimeStamp*/
	                     	Arp::DateTime recordTime;
	                     	valueTmp.CopyTo(recordTime);
	                     	Log::Info("DateTime: {0}", recordTime.ToBinary());
	                     	/*End of dummy Code*/

						uint8 dateTimeBuffer[8] = {0}; 					 //reinitialize the dateTimeBuffer
						valueTmp.CopyTo(*((DateTime*)(dateTimeBuffer))); //copy the time stamp value to dateTimeBuffer

						for(int i = 0; i < sizeof(dateTimeBuffer); i++)  //write the dateTimeBuffer into byteMemory Array in Byte steps
						{
							memcpy((byteMemory + r_offset), &dateTimeBuffer[i], 1);
							r_offset += 1;
						}
					 }
					 break;

					 case RscType::Void:
					 {
						//Log::Info("NULL Value = Void RSC-Datatype is found");
					 }
					 break;

					 case RscType::Bool:  //if the DataType is Bool
					 {
						valueTmp.CopyTo(*((bool*)(byteMemory + r_offset))); //copy the logging variable value into byteMemory Array
						r_offset += 1; //increment the offset
					 }
					 break;

					 case RscType::Uint64:
					 {
							/*Start of dummy Code: Only for Output of EventCount*/
	                     	uint64 recordEventCounter;
	                     	valueTmp.CopyTo(recordEventCounter);
	                     	Log::Info("EvetCounter: {0}", recordEventCounter);
	                     	/*End of dummy Code*/

						uint8 eventCountBuffer[8] = {0}; //reset eventCountBuffer
						valueTmp.CopyTo(*((uint64*)(eventCountBuffer)));  //copy the event counter value to eventCountBuffer

						for(int i = 0; i < sizeof(eventCountBuffer); i++)  // write the event counter into byteMemory Array in Byte steps
						{
							memcpy((byteMemory + r_offset), &eventCountBuffer[i], 1);
							r_offset += 1;
						}
					 }
					 break;

                 default:
                     break;
                 }
               }
            }
        }
        readEnumerator.EndRead();
    });

    ErrorCode result;

   //Call the ReadVariablesData Method from DataLogger Service
   result = this->m_pDataLoggerService->ReadVariablesData(
            sessionName,
            startTime,
            endTime,

   // This is the Delegate for the transmission of VariableNames
   IDataLoggerService::ReadVariablesDataVariableNamesDelegate::create([&](
         IRscWriteEnumerator<RscString<512>>& writeEnumerator)
		 {
            writeEnumerator.BeginWrite(variableNames.size());
            for (const auto& varName : variableNames)
            {
                writeEnumerator.WriteNext(varName);
            }
            writeEnumerator.EndWrite();
		}),
		readValuesDelegate);
    return result;
};

/// Thread Body
void CppDataLoggerComponent::workerThreadBody(void) {

	// you can check the log messages in the local log-file of this application, usually in a sub folder named "Logs"
	if(!m_bInitialized) // If not initialized
	{
		//Set the startTime 1 second earlier as DateTime::Now().
		Arp::Microseconds ticksNow(DateTime::Now().ToUnixMicrosecondTicks());
		startTime = Arp::DateTime::FromUnixMicrosecondTicks((ticksNow - Arp::Seconds(1)).count());
		//Log::Info("startTime: {0}", startTime.ToBinary());

		Init();  //Call Init() function
	}

	else{
		   endTime = Arp::DateTime::Now(); //The time window includes records between two worker thread cycles

		   ErrorCode result = this->ReadVariablesDataToByte(
				    sessionname,			//sessionname is defined in datalogger.config file.
			        startTime,				//start time is initialized in the Init() method and will be updated after this method call
			        endTime,				//end time will be updated in this method
			        CountingVariableNames,	//this is the vector with logged variable names
					m_records				//this is the pointer to the ByteArray, but will be not used in this application, because the values will be copied directly to the dequeue during iteration of elements in record
			        );
			startTime = endTime;  		  	//The time window includes records between two worker thread cycles
		}
	}
} // end of namespace CppDataLogger
