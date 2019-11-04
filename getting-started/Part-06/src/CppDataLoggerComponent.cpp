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

//////////////////////////////////////////////////////////////////////////////////////////
//This is the "GetRecord" method and will be executed by program in real-time context.  //
//The method copied the data from the queue to byteRecord. The byteRecord is a array    //
//defined as OutPort-Variable and is connected with InPort-Variable in PLCnext Engineer.//
//The IEC61131 Program in PLCnext Engineer copied the data from to Profinet send buffer.//
//The variable "b_PN_DataValidBit" is a Input-Port and provides the state of Profinet-  //
//Communication from PLCnext Engineer system variable "PND_S1_VALID_DATA_CYCLE". We use //
//this input as trigger for transmission of data from the queue to Profinet send buffer.//
//                                                                                      //
//The Return Value currQueueSize is a OutPort-Variable and can be used as memory status //
//in PLCnext Engineer project.                                                          //
//////////////////////////////////////////////////////////////////////////////////////////

uint32 CppDataLoggerComponent::GetRecord(uint8* byteRecord, bool &b_PN_DataValidBit) {

	uint32 currQueueSize = 0; //the value returns the current dequeue size

	if(m_bInitialized == true)
	{
		currQueueSize = toQueue.size(); //get the dequeue size

		if(currQueueSize > 0 && b_PN_DataValidBit == true) //if the size is not zero and the Profinet communication is established
		{
			shared_ptr<SaveToQueue> toPN; //shared pointer

			myLock.lock(); //get mutex so we can read the record from the queue;
			toPN = make_shared<SaveToQueue>((toQueue.front())); //get the first element to the shared pointer

			if(toPN)
			{
				memset(byteRecord, 0x00, sizeof(toPN->byteRecord));
				memcpy(byteRecord, toPN->byteRecord, sizeof(toPN->byteRecord)); //Copy 512 Bytes to the byteRecord
			}

			toQueue.pop_front(); //delete the first element
			myLock.unlock(); //unlock mutex
		}

		if(currQueueSize > 10000 && currQueueSize < 100000 && m_QueueOverflowWarning == false) //set warning message, if the queue size is greater as 10000
		{
			Log::Info("[CppDataLoggerComponent]-------------------------------Record Overflow in the Queue is expected!");
			m_QueueOverflowWarning = true;
		}
		else if(currQueueSize <= 1000)
			m_QueueOverflowWarning = false;

		if(currQueueSize > 100000 && m_QueueOverflowError == false) //set alarm message, if the queue size is greater as 100000
		{
			toQueue.erase(toQueue.begin(), toQueue.begin() + 10000); //erase the first 10000 elements
			Log::Error("[CppDataLoggerComponent]-------------------------------Record Overflow in the Queue, the first 10000 records are erased! ");
			m_QueueOverflowError = true;
		}
		else
			m_QueueOverflowError = false;
	}
	return(currQueueSize);
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
//This is the ReadVariablesDataToByte method with ReadVariablesData service call of     //
//DataLogger service. The Service Call reads the data from the given variable from      //
//the session. This service function returns the data values from the passed variable   //
//names including timestamps and data series consistent flags, which is called a record.//
//                                                                                      //
//In a record the values are in a static order and doesn't contain any type information.//
//Each record starts with the timestamp followed by the values from the given variable  //
//by names and ends with the consistent flag.                                           //
//////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CppDataLoggerComponent::ReadVariablesDataToByte(const Arp::String& sessionName,
    const Arp::DateTime& startTime, const Arp::DateTime& endTime,
    const std::vector<Arp::String>& variableNames, uint8* byteMemory)
{
    IDataLoggerService::ReadVariablesDataValuesDelegate readValuesDelegate =
        IDataLoggerService::ReadVariablesDataValuesDelegate::create([&](
            IRscReadEnumerator<RscVariant<512>>& readEnumerator)
  {
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
            size_t r_offset = 0;                        //reinitialize the r_offset

            uint8 ID_Number = 0;                        //reinitialize the ID_Number
            uint8 LogVarCounter = 0;                    //reinitialize the LogVarCounter
            RscVariant<512> valueTmp = {0};             //reinitialize the valueTmp
            uint8 valueLogVarTmp[8] = {0};              //reinitialize the valueLogVarTmp
            uint8 dateTimeBuffer[8] = {0};              //reinitialize the dateTimeBuffer

            bool b_FoundNullValue = false;              //reinitialize the b_FoundNullValue

            std::vector<size_t> dataValidOffsetTmp;     //vector for r_offset of DataValidBit
            std::vector<SaveToQueue> byteRecordTmp;     //temporary vector for variable Array will be used as PN-Telegram buffer if the variable numbers are above 50 (509 Bytes)

            memset(newRecord.byteRecord, 0x00, sizeof(newRecord.byteRecord));  //reinitialize the newRecord.byteRecord

            for (size_t i = 0; i < (arraySize-1); i++)  //for each element-1 in the array, the last element is a DataValidBit and will be copy separately after this loop
            {
             // The Value will be copied into variant
            arrayReader.ReadNext(valueTmp);

            // Each RscType should be check separately
            // The following data types are expected: DateTime, Bool, Uint64 and Void(NULL)
             switch (valueTmp.GetType())
             {
             case RscType::DateTime:  //if the DataType is DateTime
             {
                uint8 dateTimeBuffer[8] = {0};                   //reinitialize the dateTimeBuffer
             	valueTmp.CopyTo(*((DateTime*)(dateTimeBuffer))); //copy the time stamp value to dateTimeBuffer

             	for(int i = 0; i < sizeof(dateTimeBuffer); i++)  //write the dateTimeBuffer into newRecord.byteRecord Array
             	{
             		memcpy((newRecord.byteRecord + r_offset), &dateTimeBuffer[i], 1);
             		r_offset += 1;
             	 }

            	ID_Number = 0; // set the Variable ID-Number to zero, the ID-Number will be incremented during iteration of elements
             }
             break;

             case RscType::Void:
             {
             	if (b_FoundNullValue == false)
             	{
             		b_FoundNullValue = true;  //Null-Value is founded
             	}
             	else
             	{
             		//Log::Info("ID_Number = {0}   Value = Void", (int)ID_Number);
             		ID_Number += 1;            //increment logging variable ID_Number if the Null-Value and Null-EventCounter is founded
             		b_FoundNullValue = false;
             	}
             }
             break;

             case RscType::Bool:  //if the DataType is Bool
             {
            	newRecord.byteRecord[r_offset] = ID_Number;  // copy the variable ID-Number to newRecord.byteRecord
            	ID_Number += 1;                              // increment logging variable ID_Number
            	r_offset += 1;                               // increment the offset

            	valueTmp.CopyTo(*((bool*)(valueLogVarTmp))); // copy the logging variable value to valueLogVarTmp
            	newRecord.byteRecord[r_offset] = valueLogVarTmp[0]; // copy the record-element to newRecord.byteRecord
                r_offset += 1; //increment the offset

                //Log::Info("ID_Number = {0}   Value = {1}", (int) newRecord.byteRecord[r_offset-2], (int) newRecord.byteRecord[r_offset-1]);
             }
             break;

             case RscType::Uint64:
             {
                uint8 eventCountBuffer[8] = {0}; //reset eventCountBuffer

                //copy the eventVariable Counter Value to the newRecord
                valueTmp.CopyTo(*((uint64*)(eventCountBuffer)));  //copy the event counter value to eventCountBuffer

                for(int i = 0; i < sizeof(eventCountBuffer); i++) //write the event counter into newRecord.byteRecord Array
                {
                	memcpy((newRecord.byteRecord + r_offset), &eventCountBuffer[i], 1);
                	r_offset += 1;
                }

                LogVarCounter += 1; //increment LogVarCounter
             }
              break;

             default:
                 break;
             }

             if (LogVarCounter >= MaxLogVar) // The limit for one PN Telegram is 50 variables: Time stamp + 50 x (VarID + VarValue + eventCount) + DataValidBit
                                             // 50 Variables = 8 Byte + 50 x 10Bytes + 1Byte = 509 Bytes (Offset 0..508)
             {
             	dataValidOffsetTmp.push_back(r_offset); //save the the offset of "r_offset" to dataValidOffsetTmp-vector (is needed for complete the PN-Telegram with data-valid bit)
             	byteRecordTmp.push_back(newRecord);     //save the record to byteRecordTmp-vector

             	LogVarCounter = 0;  //reset the LogVarCounter
                r_offset = 0;       //reset the r_offset

                memset(newRecord.byteRecord, 0x00, sizeof(newRecord.byteRecord)); //reinitialize the newRecord.byteRecord

                for(int i = 0; i < sizeof(dateTimeBuffer); i++) //write the time stamp in the first 8 Bytes of newRecord.byteRecord Array
                 {
                 	memcpy((newRecord.byteRecord + r_offset), &dateTimeBuffer[i], 1); //copy one of 8 time stamp bytes into newRecord.byteRecord Array
                 	r_offset += 1; //increment the offset after copy of 1Byte
                 }
             }
         }

     arrayReader.ReadNext(valueTmp); //read in the last element "ConsistentDataSeries"

	 if(valueTmp.GetType() == RscType::Bool) //The last Element is "ConsistentDataSeries"
	 {
		 valueTmp.CopyTo(*((bool*)(newRecord.byteRecord + r_offset))); //copy the ConsistentDataSeries value into newRecord.byteRecord

		 int iOffset = 0;

		 for(size_t j=0; j < byteRecordTmp.size(); j++) //If more that 50 variables are logged, divide the Record in tree PN-Telegrams
		 {
		 	iOffset = (int)dataValidOffsetTmp[j]; //Get iOffset as index for "ConsistentDataSeries"
		 	byteRecordTmp[j].byteRecord[iOffset]= newRecord.byteRecord[r_offset]; //copy the ConsistentDataSeries value into PN-Telegram (index position 508, 508, 288)

		 	myLock.lock(); //get mutex so we can write our new record to the queue;
		 	toQueue.push_back(byteRecordTmp[j]); //save the byteRecordTmp into PN-Telegram Queue
		 	myLock.unlock(); //unlock mutex
		 }

		 //std::lock_guard<std::mutex> lock(this->myLock);
		 myLock.lock(); //get mutex so we can write our new record to the queue;
		 toQueue.push_back(newRecord); //save thenewRecord into PN-Telegram Queue
		 myLock.unlock(); //unlock mutex
	  }

      else
     {
    	  Log::Error("ReadVariablesDataToByte()----------The ConsistentDataSeries Value is NOT found");
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
		startTime = Arp::DateTime::Now(); //initialize startTime
		endTime = Arp::DateTime::Now();   //initialize endTime

		Init(); //Call Init() function
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
