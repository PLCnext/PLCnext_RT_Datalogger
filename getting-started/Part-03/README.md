This is part of a [series of articles](https://github.com/PLCnext/PLCnext_RT_Datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 3 - Using of RSC-Services for reading back the DataLogger configuration

A requirement of PLCnext DataLogger application can contain the access to the DataLogger configuration and the logged data in a datasink for providing this data e.g. via PROFINET to another controllers or systems.

This article uses Eclipse IDE to implement a C ++ application to access the DataLogger configuration through the RSC-Service "IDataLoggerService". The picture below shows the general flow diagram with Program and Worker Thread that we have to implement. In this article we will at first implement methods for WorkerThread: 
 - Initialize global Variables and IDataLoggerService 
 - Get the session name, started by IDataLoggerService
 - Get the variable names, logged during the session 

In the folder "src" of this article you will find the completely implemented source code. If you don't have much time to implement the code yourself, you can replace the files in your Eclipse project with files in that src folder. 

1. Open Eclipse IDE and create a C++ Project for PLCnext target:
   - Project name: CppDataLogger
   - Component name: CppDataLoggerComponent 
   - Program name: CppDataLoggerProgram


2. Implement a WorkerThread for non-realtime RSC-Service and get the information of started sessions and configured log-variables from IDataLoggerService:

 - Open and include following declaration of methods into header file of component "CppDataLoggerComponent.hpp"
   	<details>
   	<summary>(click to see/hide code)</summary>

    ```cpp
    public: // IControllerComponent operations
    void Start(void)override;
    void Stop(void)override;
    
    private: // methods
    void workerThreadBody(void);
	bool Init();
    
    private: // fields
    //Worker Thread
	WorkerThread workerThreadInstance;
    bool xStopThread = false;
	bool m_bInitialized = false;
	
	// IDataLoggerService Handle
	IDataLoggerService2::Ptr m_pDataLoggerService;
    
	```

   </details>
     
 - Open and include following methods into .cpp file of component "CppDataLoggerComponent.cpp"
   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
    
	void CppDataLoggerComponent::Start(void) {
		xStopThread = false;
		log.Info("[CppDataLoggerComponent]-------------------------------workerThreadInstance start");
		workerThreadInstance.Start();
		log.Info("[CppDataLoggerComponent]-------------------------------DataLoggerService started");
	}

	void CppDataLoggerComponent::Stop(void) {
		// if you want to stop some loops of your thread during execution
		// add something like "stoptheThread" before executing workerThreadStop.
		xStopThread = true;

		log.Info("[CppDataLoggerComponent]-------------------------------workerThreadInstance stop");
		workerThreadInstance.Stop();
		log.Info("[CppDataLoggerComponent]-------------------------------DataLoggerService stopped");
	}
    
    ///////////////////////////////////////////////////////////////////////////////
    // Implement WorkerThread Body
    ///////////////////////////////////////////////////////////////////////////////
    void CppDataLoggerComponent::workerThreadBody(void) 
	{ 
	    if(!m_bInitialized) // If not initialized
	    {
		    Init(); // you can check the log messages in the local log-file of this application, usually in a sub folder named "Logs"
	    }
    }
	

    ///////////////////////////////////////////////////////////////////////////////
    // Implement method for getting information about started sessions and logged variables
    ///////////////////////////////////////////////////////////////////////////////
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
			Arp::String sessionname = sessions[0]; // The array element "sessions[0]" contains the current session name, the content is set by Service Call "ListSessionNames"

			//Result vector of Logged variables
			std::vector<Arp::Plc::Gds::Services::VariableInfo> VariableInfos;

			//Vector for Variable Names, sorted by name. This vector will be necessary in the next part of this article
			std::vector<Arp::String> CountingVariableNames = {};

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
   ```
   
   </details>   

3. Include the C++ Library into PLCnext Engineer Project as showed in steps 1-4 in following picture:

![IEC_Program](/Picture/10_IncludeDataLoggerLibrary.png)

4. Download and execute the PLCnEng project on the PLCnext target

5. Find the SqLite database "test-session.db" in "/opt/plcnext/logs/datalogger/test-session.db" on the PLCnext target

6. Find the following content in file "opt/plcnext/logs/Custom.log" on the PLCnext target

    ```cpp
	
	29.08.25 10:28:35.910 CppDataLogger.CppDataLoggerProgram                           INFO  - [CppDataLoggerProgram]-------------------------------DL Constructor
	29.08.25 10:28:37.152 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent]-------------------------------workerThreadInstance start
	29.08.25 10:28:37.154 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent]-------------------------------DataLoggerService started
	29.08.25 10:28:37.156 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Session-Name inside DataLoggerServices is: test-session
	29.08.25 10:28:37.165 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001, Boolean
	29.08.25 10:28:37.165 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002, Boolean
	29.08.25 10:28:37.165 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003, Boolean
	29.08.25 10:28:37.166 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004, Boolean
	29.08.25 10:28:37.166 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005, Boolean
	29.08.25 10:28:37.166 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006, Boolean
	29.08.25 10:28:37.167 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007, Boolean
	29.08.25 10:28:37.168 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008, Boolean
	29.08.25 10:28:37.168 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001_change_count, UInt64
	29.08.25 10:28:37.168 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002_change_count, UInt64
	29.08.25 10:28:37.168 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003_change_count, UInt64
	29.08.25 10:28:37.169 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004_change_count, UInt64
	29.08.25 10:28:37.169 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005_change_count, UInt64
	29.08.25 10:28:37.169 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006_change_count, UInt64
	29.08.25 10:28:37.170 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007_change_count, UInt64
	29.08.25 10:28:37.170 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008_change_count, UInt64
	29.08.25 10:28:37.171 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001
	29.08.25 10:28:37.171 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_001_change_count
	29.08.25 10:28:37.172 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002
	29.08.25 10:28:37.172 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_002_change_count
	29.08.25 10:28:37.172 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003
	29.08.25 10:28:37.172 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_003_change_count
	29.08.25 10:28:37.172 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_004_change_count
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_005_change_count
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_006_change_count
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_007_change_count
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008
	29.08.25 10:28:37.173 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Returned list of sorted variables contain Arp.Plc.Eclr/DIO_Producer1.IN_DI_008_change_count
	29.08.25 10:28:37.175 CppDataLogger.CppDataLoggerComponent                         INFO  - [CppDataLoggerComponent] Session Name, contained the logged variable ''Arp.Plc.Eclr/DIO_Producer1.IN_DI_001'' is: test-session
   ```
---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
