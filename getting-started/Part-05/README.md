This is part of a [series of articles](https://github.com/PLCnext/plcnext-real-time-datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 5 - Using RSC Services for access to the logged data via Rsc service


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

			if(!m_bInitialized) // If not initialized
			{
				//Set the startTime 1 second earlier as DateTime::Now().
				Arp::Microseconds ticksNow(DateTime::Now().ToUnixTimeMicroseconds());
				startTime = Arp::DateTime::FromUnixTimeMicroseconds((ticksNow - Arp::Seconds(1)).count());
				Log::Info("startTime: {0}", startTime.ToBinary());

				Init();  //Call Init() function
			}

			else{
				   //The time window includes records between two worker thread cycles
				   endTime = Arp::DateTime::Now(); 

				   ErrorCode result = this->ReadVariablesDataToByte(
							sessionname, //sessionname is defined in datalogger.config file.
							startTime,	 //start time 
							endTime,	 //end time 
							CountingVariableNames, //vector with logged variable names
							m_records	//this is the pointer to the ByteArray
							);
							
					//The time window includes records inside worker thread cycle
					startTime = endTime;  		  	
				}
			}
	
   ```
   
   </details>   

2. Compile the C++ Project.

3. After compilation, the C++ Library should be updated automatically, if the path to library "CppDataLogger.pcwlx" in Eclipse workspace folder is valid. If not, please replace the library "CppDataLogger.pcwlx" in PLCnext Engineer project with the new generated library.

4. Download and execute the PLCnEng project on the PLCnext target

5. Find the DateTime TimeStamps in file "opt/plcnext/logs" on the PLCnext target
---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.