This is part of a [series of articles](https://github.com/PLCnext/plcnext-real-time-datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 2 - DataLogger service configuration for the PLCnext target

In the previous article, we built a simple PLCnext Engineer application and started it on a PLCnext Control. This part of article describes the datalogger configuration. 

For datalogger configuration you have proceed the following steps. If the detailed "step by step" instruction is needed, please find it under point "Procedure":

1.	Clone the folder Services onto your host machine
2.	Copy the folder /Services/DataLogger to your PLCnext Control with e.g. WinSCP into the directory /opt/plcnext/projects/ 
3.	Now open the /opt/plcnext/projects/Services/DataLogger/data-logger.dl.config with an editor and modify the settings according to your use case (please refer the comments in the file).
4.	Save the file and restart the PLCnext Control (via PLCnext Engineer or shell sudo /etc/init.d/plcnext restart)
5.	The log files will now be available in the directory /opt/plcnext/projects/Services/DataLogger/ and can be copied via SFTP (e.g. with WinSCP).
	All files are stored in a SQLlite format and can be used with every SQLlite compatible viewer.


**Application Note:**

* With the sampling rate key it is possible to configure the scan cycle of the datalogger according to the application demands (in this case 50ms). As faster it is configured as more performance is occupied from the service.
* The publish interval (copy Ramdisk to flash memory) is configured to 250ms and the buffer capacity is set to 1000 data records. Please note, that if the publishing interval is shorter, the CPU load will be higher. If the buffer size is greater, the memory usage will be higher.
* The datalogger will subscribe to every defined GDS port defined in the `<Variables>` section, please make sure that all defined ports are available in your application
* Please ensure that the adjusted `samplingInterval` is not faster than the fastest possible user task of your PLcnext Control (please refer the datasheet first).  

## The DataLogger service supports two types of recordings: ##
1. Continuous recording  
During continuous recording, the DataLogger service is synchronized with the program instance in which the port variables to log are declared.

2. Event-based recording  
During event-based recording, the variable values are only recorded if the value status changes, and the according variables event counter will be incremented.

## Options of storing the database on the flash file system (SD-card) ##
1. "rollover = false"
By using setting creates the datalogger one database (SQLlite format) file on the file system (located in the same directory like the configuration file).  
When the configured maxFileSize is reached, will the datalogger delete the first 30% of oldest data.  

Configuration example: 
(Datasink type="db" dst="/opt/plcnext/projects/Services/DataLogger/yourDB.db" rollover="false" maxFileSize="1000000" storeChangesOnly="false") 

2. "rollover = true"  
In case the rollover option should be used will the datalogger creates a new database file (same directory like in the first option) after the maxFileSize size is reached.
If the defined number of files on the file system is reached, the oldest database file will be deleted. The consecutive file number will be retained.  

Configuration example: 
(Datasink type="db" dst="/opt/plcnext/projects/Services/DataLogger/yourDB.db" rollover="true" maxFileSize="1000000"  maxFiles="10" storeChangesOnly="false") 

* Phoenix Contact recommend the usage of mode 1 ("rollover" = false) in combination with an uninterruptible power source (UPS).


---


## Detailed Procedure ##

For configuration of DataLogger
Please follow these instructions for that:



---


![IEC_Program](/Picture/07_DataLoggerConfiguration.png)

•	Placing the DataLogger configuration file on the target
1.	Open the „/DataLogger/configfile/“ directory on the host machine to find the „data-logger.dl.config” file
2.	Establish a connection to the PLCnext target via WinSCP
3.	In „/opt/plcnext/projects/”, create this path: “/Services/DataLogger“
4.	Copy the „data-logger.dl.config” file into the „/opt/plcnext/projects/Services/DataLogger“ path



---


![IEC_Program](/Picture/08_1_LogPortVariableList.png)

![IEC_Program](/Picture/08_2_LogPortVariableList.png)

•	Creating a Log Variable list in the „data-logger.dl.config“ file
1.	In the „/opt/plcnext/projects/PCWE/Plc/Gds/“ folder on the PLCnext target, open the „PCWE.gds.config“ file
2.	In the „/opt/plcnext/projects/Services/DataLogger/“ folder on the PLCnext target, open the „data-logger.dl.config“ file
3.	In the „PCWE.gds.config“ file, copy the variable name from the <Connectors> section
4.	In the „data-logger.dl.config“ file, paste the variable name into the <Variables> section
5.	Save the edited „data-logger.dl.config“ file
6.	Restart the PLCnext target



---


![IEC_Program](/Picture/09_LocationSQLiteDB.png)

•	Where to find the SQLite databases on the PLCnext target
1.	Find the SQLite databases in the folder defined in „data-logger.dl.config“ by a <Datasink> entry. The default value is "/opt/plcnext/logs/DataSink.db".



---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
