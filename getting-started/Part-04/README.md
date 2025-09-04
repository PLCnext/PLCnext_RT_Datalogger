This is part of a [series of articles](https://github.com/PLCnext/PLCnext_RT_Datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 4 - Using RSC Services for access to the logged data via Rsc service


The example below uses the Call "GetLoggedVariables" of "IDataLoggerService" RSC service to access to the logged data, and store the values into byte array. In the folder "src" of this article you will find the completely implemented source code. If you don't have much time to implement the code yourself, you can replace the files in your Eclipse project with files in that src folder.

Please note, the following code has following critical implementation that will be fixed in the next part of this article: 
By storage of data in static byte array, you do not know how many records are read, only how big a record is! Therefore, the code is very critical, because the index of variables can go beyond the index limits of the  array. In outher words, if the memory is not enough the storage will be written beyond the limits! This sample code should only be used for demonstration of service call method implementation and is an intermediate step to the final solution.

1. Include into Project `CppDataLogger` the method "ReadVariablesDataToByte":

 - Open and include following declaration of methods and fields into header file of component "CppDataLoggerComponent.hpp"
   	<details>
   	<summary>(click to see/hide code)</summary>

    ```cpp
    private: // methods
	ErrorCode ReadVariablesDataToByte(const Arp::String& sessionName,
	    const Arp::DateTime& startTime, const Arp::DateTime& endTime,
		const std::vector<Arp::String>& variableNames, uint8* byteMemory);
	
    private: // fields
	//Session Name
    Arp::String sessionname = {};

    //Vector for Variable Names, sorted by name.
    std::vector<Arp::String> CountingVariableNames = {};

    //Start and End time as time window parameter
    Arp::DateTime startTime;
    Arp::DateTime endTime;

    //Define the buffer for the records. Please note, this code is very critical because,
	//if the memory is not enough the storage will be written beyond the array limits!
    uint8 m_records[2578000];
	```
   </details>
     
 - Include the method "ReadVariablesDataToByte" and adapt the "workerThreadBody" in "CppDataLoggerComponent.cpp" file as follows:
 
   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
		bool CppDataLoggerComponent::Init()
	{
		if (m_bInitialized)  // If already initialized, don't execute initialization again
		{
			return(true);
		}

		bool bRet = false;

		m_pDataLoggerService = ServiceManager::GetService<IDataLoggerService2>();     //get IDataLoggerService2

		if (m_pDataLoggerService != NULL) //if IDataLoggerService is valid
		{

			////////////////////////////////////////////////////////////////////
			//This is the ListSessionNames service call of DataLogger service.//
			////////////////////////////////////////////////////////////////////

			//Result vector of sessions names started by DataLogger Service
			std::vector<Arp::String> sessions;

			this->m_pDataLoggerService->ListSessionNames(IDataLoggerService2::ListSessionNamesResultDelegate::create([&](IRscReadEnumerator<RscString<512>>& enumerator)
				{
					size_t nVariables = enumerator.BeginRead();
					sessions.reserve(nVariables);
					RscString<512> current;
					while (enumerator.ReadNext(current))
					{
						sessions.push_back(current.CStr());
						log.Info("[CppDataLoggerComponent] Session-Name inside DataLoggerServices is: {0}", current.CStr());
					}
					enumerator.EndRead();
				}));


			//////////////////////////////////////////////////////////////////////////
			//This is the GetLoggedVariables service call of DataLogger service.    //
			//The service call Queries all info about logged variables of a session.//
			//////////////////////////////////////////////////////////////////////////

			//Name of session to query logged variables
			this->sessionname = sessions[0]; // The array element "sessions[0]" contains the current session name, the content is set by Service Call "ListSessionNames"

			//Vector for Variable Names, sorted by name. This vector will be necessary in the next part of this article
			this->CountingVariableNames = {};

			//Result vector of Logged variables
			std::vector<Arp::Plc::Gds::Services::VariableInfo> VariableInfos;



			ErrorCode error = this->m_pDataLoggerService->GetLoggedVariables(sessionname, IDataLoggerService2::GetLoggedVariablesInfosDelegate::create([&](IRscReadEnumerator<Arp::Plc::Gds::Services::VariableInfo>& enumerator)
				{

					size_t nVariables = enumerator.BeginRead();
					VariableInfos.reserve(nVariables);
					VariableInfo current;

					std::vector<std::string> stringarray; //this is the temp-vector for sorting of variables

					while (enumerator.ReadNext(current))
					{
						stringarray.push_back(Arp::String(current.Name)); //copy the Log-Vaiable-Name and Event-Variable-Name to this vector

						VariableInfos.push_back(current); //save all information about logg-variables in this vector (only for information in output.log data)
						log.Info("[CppDataLoggerComponent] Returned list of variables contain {0}, {1}", current.Name, current.Type);
					}
					enumerator.EndRead();

					std::sort(stringarray.begin(), stringarray.end()); //sort the names in the string array by name

					int iCnt = 0;
					for (const auto& km : stringarray) //if the names are sorted, store it in the CountingVariableNames-Vector (as parameter for the method "ReadVariablesDataToByte which will be implemented in the next part)"
					{
						CountingVariableNames.push_back(Arp::String(stringarray[iCnt]));
						log.Info("[CppDataLoggerComponent] Returned list of sorted variables contain {0}", CountingVariableNames[iCnt].CStr());
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


			this->m_pDataLoggerService->GetSessionNames(currentVariableName, IDataLoggerService2::GetSessionNamesResultDelegate::create([&](IRscReadEnumerator<Arp::Plc::Gds::Services::RscString<512>>& enumerator)
				{
					size_t nSessions = enumerator.BeginRead();
					SessionInfos.reserve(nSessions);
					RscString<512> currentSession;

					while (enumerator.ReadNext(currentSession))
					{
						SessionInfos.push_back(currentSession); //save all session names in this vector
						log.Info("[CppDataLoggerComponent] Session Name, contained the logged variable ''{0}'' is: {1}", currentVariableName.CStr(), currentSession.CStr());
					}
					enumerator.EndRead();

				}));

			m_bInitialized = true;	//set the m_bInitialized flag to "true"
			bRet = true;
		}
		else
		{
			log.Error("[CppDataLoggerComponent] ServiceManager::GetService<IDataLoggerService>() returned error");
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
		IDataLoggerService2::ReadVariablesDataValuesDelegate readValuesDelegate =
			IDataLoggerService2::ReadVariablesDataValuesDelegate::create([&](
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
									log.Info("DateTime: {0}", recordTime.ToBinary());
									/*End of dummy Code*/

									uint8 dateTimeBuffer[8] = { 0 }; 					 //reinitialize the dateTimeBuffer
									valueTmp.CopyTo(*((DateTime*)(dateTimeBuffer))); //copy the time stamp value to dateTimeBuffer

									for (int i = 0; i < sizeof(dateTimeBuffer); i++)  //write the dateTimeBuffer into byteMemory Array in Byte steps
									{
										memcpy((byteMemory + r_offset), &dateTimeBuffer[i], 1);
										r_offset += 1;
									}
								}
								break;

								case RscType::Void:
								{
									//log.Info("NULL Value = Void RSC-Datatype is found");
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
									log.Info("EvetCounter: {0}", recordEventCounter);
									/*End of dummy Code*/

									uint8 eventCountBuffer[8] = { 0 }; //reset eventCountBuffer
									valueTmp.CopyTo(*((uint64*)(eventCountBuffer)));  //copy the event counter value to eventCountBuffer

									for (int i = 0; i < sizeof(eventCountBuffer); i++)  // write the event counter into byteMemory Array in Byte steps
									{
										memcpy((byteMemory + r_offset), &eventCountBuffer[i], 1);
										r_offset += 1;
									}
								}
								break;

								case RscType::Uint8:
								{
									valueTmp.CopyTo(*((uint8*)(byteMemory + r_offset))); //Is only relevant for trigger-based data acquisition.
									//The field indicates to which recording cycle the respective data record belongs.
									r_offset += 1; //increment the offset
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
			IDataLoggerService2::ReadVariablesDataVariableNamesDelegate::create([&](
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

		if (!m_bInitialized) // If not initialized
		{
			//Set the startTime 1 second earlier as DateTime::GetUtcNow().
			Arp::Microseconds ticksNow(DateTime::GetUtcNow().ToUnixTimeMicroseconds());
			startTime = Arp::DateTime::FromUnixTimeMicroseconds((ticksNow - Arp::Seconds(1)).count());
			//log.Info("startTime: {0}", startTime.ToBinary());

			Init();  //Call Init() function
		}

		else {
			endTime = Arp::DateTime::GetUtcNow(); //The time window includes records between two worker thread cycles

			//log.Info("startTime: {0}    endTime: {1}    sessionname {2}", startTime.ToBinary(), endTime.ToBinary(), sessionname.CStr());

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
   ```
   
   </details>   

2. Compile the C++ Project.

3. After compilation, the C++ Library should be updated automatically, if the path to library "CppDataLogger.pcwlx" folder is valid. If not, please replace the library "CppDataLogger.pcwlx" in PLCnext Engineer project with the new generated library.

4. Download and execute the PLCnEng project on the PLCnext target

5. Find the DateTime TimeStamps in file "opt/plcnext/logs/Custom.log" on the PLCnext target

   <details>
   <summary>(click to see/hide code)</summary>

  ```cpp
  
	29.08.25 15:13:11.701 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent]-------------------------------workerThreadInstance start
	29.08.25 15:13:11.702 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent]-------------------------------DataLoggerService started
	29.08.25 15:13:11.710 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Session-Name inside DataLoggerServices is: test-session
	29.08.25 15:13:11.719 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001, Boolean
	29.08.25 15:13:11.720 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002, Boolean
	29.08.25 15:13:11.720 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003, Boolean
	29.08.25 15:13:11.721 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004, Boolean
	29.08.25 15:13:11.722 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005, Boolean
	29.08.25 15:13:11.723 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006, Boolean
	29.08.25 15:13:11.724 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007, Boolean
	29.08.25 15:13:11.724 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008, Boolean
	29.08.25 15:13:11.725 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001_change_count, UInt64
	29.08.25 15:13:11.726 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002_change_count, UInt64
	29.08.25 15:13:11.726 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003_change_count, UInt64
	29.08.25 15:13:11.726 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004_change_count, UInt64
	29.08.25 15:13:11.727 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005_change_count, UInt64
	29.08.25 15:13:11.727 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006_change_count, UInt64
	29.08.25 15:13:11.728 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007_change_count, UInt64
	29.08.25 15:13:11.728 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008_change_count, UInt64
	29.08.25 15:13:11.730 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001
	29.08.25 15:13:11.731 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001_change_count
	29.08.25 15:13:11.732 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002
	29.08.25 15:13:11.732 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002_change_count
	29.08.25 15:13:11.733 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003
	29.08.25 15:13:11.733 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003_change_count
	29.08.25 15:13:11.734 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004
	29.08.25 15:13:11.734 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004_change_count
	29.08.25 15:13:11.735 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005
	29.08.25 15:13:11.735 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005_change_count
	29.08.25 15:13:11.735 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006
	29.08.25 15:13:11.736 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006_change_count
	29.08.25 15:13:11.736 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007
	29.08.25 15:13:11.736 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007_change_count
	29.08.25 15:13:11.737 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008
	29.08.25 15:13:11.737 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008_change_count
	29.08.25 15:13:11.739 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Session Name, contained the logged variable ''Arp.Plc.Eclr/DIO_Producer1.IN_DI_001'' is: test-session
	29.08.25 15:13:13.029 CppDataLogger.CppDataLoggerComponent                         INFO  - DateTime: 5250606790355546144
	29.08.25 15:13:13.029 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.029 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.030 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 0
	29.08.25 15:13:13.195 CppDataLogger.CppDataLoggerComponent                         INFO  - DateTime: 5250606790357046244
	29.08.25 15:13:13.195 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 2
	29.08.25 15:13:13.196 CppDataLogger.CppDataLoggerComponent                         INFO  - DateTime: 5250606790357546044
	29.08.25 15:13:13.196 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 3
	29.08.25 15:13:13.197 CppDataLogger.CppDataLoggerComponent                         INFO  - DateTime: 5250606790358045844
	29.08.25 15:13:13.198 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 4
	29.08.25 15:13:13.199 CppDataLogger.CppDataLoggerComponent                         INFO  - DateTime: 5250606790358545964
	29.08.25 15:13:13.199 CppDataLogger.CppDataLoggerComponent                         INFO  - EvetCounter: 5

  ```
   
   </details> 
---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.