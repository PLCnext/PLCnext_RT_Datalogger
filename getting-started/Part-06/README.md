This is part of a [series of articles](https://github.com/PLCnext/plcnext-real-time-datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 6 - Transmission of logged Data via PROFINET


The example below prepare the logged data for PROFINET transmission to another PN-Device and provide the data via Port-Variable to the real-time program executed by ESM. In the folder "src" of this article you will find the completely implemented source code. If you don't have much time to implement the code yourself, you can replace the files in your Eclipse project with files in that src folder.

If you don't have much time to create the PLCnext Engineer project or Datalogger cofiguration file yourself, you can find it in "PLCnEngDataLoggerProject" or "DataLoggerConfigFile" of this repository. Please note, you have to copy the "Service" folder in "DataLoggerConfigFile" in "/opt/plcnext/projects" folder on the plcnext target.

The goal of this Implementation:
 - 128 Bool variables (DI-Signals) should be logged by event-based logging configuration and saved into queue storage on plcnext target.
 - The variables values should be transmitted via PROFINET in following record format: uint64_Timestamp + 128_Variables x (b_Var_ID_Number + b_Var_Value + uint64_EventCounterValue) + b_ConsistentDataSeries
   The max. needed byte memory for one record is: 8Byte + 128 x (1Byte + 1Byte + 8Byte) + 1Byte = 1289Byte
   The max. datawigth of one PN-Telegram is: 512 Byte. One record should be devided in 3 PN-Telegram as follows:
   1. PN-Telegram = uint64_Timestamp + 50_Variables x (b_Var_ID_Number + b_Var_Value + uint64_EventCounterValue) + b_ConsistentDataSeries = 8Byte + 500Byte + 1Byte = 509Byte
   2. PN-Telegram = uint64_Timestamp + 50_Variables x (b_Var_ID_Number + b_Var_Value + uint64_EventCounterValue) + b_ConsistentDataSeries = 8Byte + 500Byte + 1Byte = 509Byte
   3. PN-Telegram = uint64_Timestamp + 28_Variables x (b_Var_ID_Number + b_Var_Value + uint64_EventCounterValue) + b_ConsistentDataSeries = 8Byte + 280Byte + 1Byte = 289Byte
 

For data transmission via PROFINET, we have to implement the following additional functionality:
 - The logged data should be stored in a queue with a data width no longer as a profinet telegram (in this case <=512Byte).
 - The logged data in the queue should be provided to the real-time program for profinet data transmission.
 - If the max. queue size is reached, the oldest queue elements should be deleted (memory protection).


1. Include into Project `CppDataLoggerComponent` the method " GetRecord" and execute this metod in real-time program "CppDataLoggerProgram" for data providing to Profinet send buffer:
 - Open and include following declaration of methods and fields into header file of component "CppDataLoggerComponent.hpp"

   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
	#include <mutex>
	
	private: // fields
	//struct definition
    struct SaveToQueue {
    uint8 byteRecord[512] = {0}; // (8Byte TimeStamp + 8Byte Data)
    };

    uint8 MaxLogVar = 50; //max. Number of LogVariables inside one PN Telegram: (PN-TelegramSize-TimeStamp)/(LogVarID + LogVarValue + LogVarEvetnCnt)
       					  //max. Number of LogVariables inside one PN Telegram: (512Byte - 8Byte)/(1Byte + 1Byte + 8Byte) = 50

    //newRecord declaration
    SaveToQueue newRecord;

    //mutex declaration
    mutex myLock;
    deque<SaveToQueue> toQueue;

    bool m_QueueOverflowWarning = 0;
    bool m_QueueOverflowError = 0;


	public: // IProgramComponent operations
       uint32 GetRecord(uint8* byteRecord,  bool &b_PN_DataValidBit); //will be called in program execution
	```
   </details>
     
 - Include the method "GetRecord" and adapt the "ReadVariablesDataToByte" in "CppDataLoggerComponent.cpp" file as follows:
 
   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
	
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
	
   ```
   </details>   
	  
- Open and include following declaration of methods and fields into header file of component "CppDataLoggerProgram.hpp"
   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
	    public: /* Ports */

		//#port
		//#attributes(Output|Retain)
		//#name(OutPortPN)
		uint8 OutPortPN [512] = {0}; //The Port-Variable for the connection in PLCnext Engineer

		//#port
		//#attributes(Output|Retain)
		//#name(OutQueueSize)
		uint32 QueueSize = 0;   //The Port-Variable for the connection in PLCnext Engineer

		//#port
		//#attributes(Input|Retain)
		//#name(Cpp_Pn_Valid_Data_Cycle_In)
		bool Cpp_Pn_Valid_Data_Cycle_In = false;   //The Port-Variable for PN-Connection Status, provided by Program in PLCnext Engineer
	```
   </details>
     
 - Call the component method "GetRecord" in "CppDataLoggerProgram.cpp" file as follows:
 
   <details>
   <summary>(click to see/hide code)</summary>

    ```cpp
	   void CppDataLoggerProgram::Execute()
	   {
        //implement program 
	    //Call the reference to the method in the component.
	    //The Execute Program method will be called in real time context
	    QueueSize = cppDataLoggerComponent.GetRecord(OutPortPN, Cpp_Pn_Valid_Data_Cycle_In);
	   }
   	```
   </details>
   

2. Compile the C++ Project.

3. After compilation, the C++ Library should be updated automatically, if the path to library "CppDataLogger.pcwlx" in Eclipse workspace folder is valid. If not, please replace the library "CppDataLogger.pcwlx" in PLCnext Engineer project with the new generated library.

4. Add in PLCnext Engineer project a new program "PN_Send_Receive" with needed system and port variables as showed in steps 1-3 in following picture:

![IEC_Program](/Picture/11_PN_Send_Receive_Var.png)

5. Add the following Code to "PN_Send_Receive" program:

![IEC_Program](/Picture/12_PN_Send_Receive_Code.png)

6. Instantiate the "PN_Send_Receive" program:

![IEC_Program](/Picture/13_PN_Send_Receive_Instance.png)

7. Connect the Port variables as showed in steps 1-4:

![IEC_Program](/Picture/14_PN_Send_Receive_ConnectPortVariables.png)

8. Download and execute the PLCnEng project on the PLCnext target.

9. Connect the AXC F 2152 as PN Device to PN Master PLC.

10. If the PN-Connection is established, you can see the profinet traffic with transmitted logged data.
---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
