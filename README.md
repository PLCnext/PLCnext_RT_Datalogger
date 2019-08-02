# PLCnext Technology - Real Time Datalogger

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Web](https://img.shields.io/badge/PLCnext-Website-blue.svg)](https://www.phoenixcontact.com/plcnext)
[![Community](https://img.shields.io/badge/PLCnext-Community-blue.svg)](https://www.plcnext-community.net)

The dataLogger is a service component of the PLCnext Technology firmware that provides real time data logging for Global Data Space (GDS) ports.  
Each data set is first stored in a Ramdisk and then stored in a database on the file-system of the PLCnext Control at a user configurable interval.  
The datalogger service starts and stops synchronously with the firmware.

First implementation in PLCnext Technology **Firmware 2019.6**.  

## Getting Started

This Readme describes the basic configuration and application of the PLCnext Real-Time Datalogger.
Please find more detailes in the [PLCnext Community](www.plcnext-community.net) and on our [Phoenix Contact Technical Support YouTube Channel](https://www.youtube.com/c/PhoenixContactTechnicalSupport).

**Important User Note:**  
In the first implementation, the database can be created on the file system using the following methods.

1. "rollover = false"
By using setting creates the datalogger one database (SQLlite format) file on the file system (located in the same directory like the configuration file).  
When the configured maxFileSize is reached, will the datalogger delete the database file and directly start with a new one.  
**This may lead to an unforeseen loss of historical data.**

2. "rollover = true"  
In case the rollover option should be used will the datalogger creates a new database file (same directory like in the first option) after the maxFileSize size is reached.
The datalogger will in the first implementation will create as many files as possible on the file system (until now available memory is available).  
**This may lead to an out of memory condition on the PLCnext Control**  
To prevent this situation must all files be copied and deleted by using e.g. SFTP manually.

Phoenix Contact recommend the usage of mode 1 ("rollover" = false) in combination with an uninterruptible power source (UPS).

In the next versions of the datalogger will the database acts as FIFO ("rollover = false") or the oldest files will be deleted ("rollover = true").

### Configuration

1. Clone the folder `Services` onto your host machine
2. Copy the folder to your PLCnext Control with e.g. WinSCP into the directory `/opt/plcnext/projects/`
3. Now open  the `/opt/plcnext/projects/Services/DataLogger/data-logger.config` with an editor and modify the settings according to your use case.
4. Save the file and restart the PLCnext Control (via PLCnext Engineer or shell `sudo /etc/init.d/plcnext restart`)
5. The log files will now be available in the directory `/opt/plcnext/projects/Services/DataLogger/` and can be copied via SFTP (e.g. with WinSCP).  
All files are stored in a SQLlite format and can be used with every SQLlite compatible viewer.

**Application Note:**

* The datalogger will subscribe to every defined GDS port defined in the `<Variables>` section, please make sure that all defined ports are available in your application
* Please ensure that the adjusted `samplingInterval` is not faster than the fastest possible user task of your PLcnext Control (please refer the datasheet first).  
* A very fast `samplingInterval` will result in a high CPU load and may have a site effect to your application (especialy non-real time).

### OPC UA Historical Access

The PLCnext embedded OPC UA Server can provide historical data for every configured GDS port assigned to the datalogger.  
Please configure the demanded GDS port as described above to the PLCnext datalogger and mark the `OPC option`in PLCnext Engineer.  
After the changes are downloaded to the PLCnext Control it is possible to read the historical values with any given OPC UA HA client.

## License

Copyright (c) Phoenix Contact Gmbh & Co KG. All rights reserved.

Licensed under the [MIT](LICENSE) License.
