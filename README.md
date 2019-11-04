# PLCnext Technology - Real Time DataLogger

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Web](https://img.shields.io/badge/PLCnext-Website-blue.svg)](https://www.phoenixcontact.com/plcnext)
[![Community](https://img.shields.io/badge/PLCnext-Community-blue.svg)](https://www.plcnext-community.net)

The DataLogger is a service component of the PLCnext Technology firmware that provides real time data logging for Global Data Space (GDS) ports.  
Each data set is first stored in a Ramdisk and then stored in a database on the file-system of the PLCnext Control at a user configurable interval.  
The datalogger service starts and stops synchronously with the firmware.

Implementation in PLCnext Technology **Firmware 2019.9**.  

## Before getting started ...

Please note that the application developed in this series of articles uses:
- `PLCnext Engineer Project`, for declaration and status change of logged port-variables.
- `WinSCP`, for copying and editing of datalogger configuration file.
- A PLC with IP address 192.168.1.10, but any valid IP address can be used.

## Getting Started

In the following series of technical articles, you will configure and implement your own datalogger application for PLCnext Control. Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

Next to this getting started we have prepared a series of introductory videos in our [Phoenix Contact Technical Support Channel on Youtube](https://www.youtube.com/playlist?list=PLXpIBdAgtoRLmx0pAn-I19x9bt-AxF29W).



|\#| Topic | Objectives |
| --- | ------ | ------ |
|[01](getting-started/Part-01/README.md)| [PLCnext Engineer Project](getting-started/Part-01/README.md)| Create a simple PLCnext Engineer Project, and run it on a PLCnext Control.|
|[02](getting-started/Part-02/README.md)| [DataLogger Configuration](getting-started/Part-02/README.md)| Configure Datalogger, and log the port variables.|
|[03](getting-started/Part-03/README.md)| [OPC UA Historical Access](getting-started/Part-03/README.md)| Make data in the datalogger application available through an OPC UA server.|
|[04](getting-started/Part-04/README.md)| [Using RSC Services for getting DataLogger configuration](getting-started/Part-04/README.md)| Implement "IDataLogger" RSC service and read the datalogger configuration back from the service.|
|[05](getting-started/Part-05/README.md)| [Using RSC Services for access to logged Data](getting-started/Part-05/README.md)| Use the "IDataLogger" RSC service to read in the logged data from a data sink.|
|[06](getting-started/Part-06/README.md)| [Transmission of logged Data via PROFINET](getting-started/Part-06/README.md)| Transmit the logged data via PROFINET to another PN-Device.|
|| [Explore unlimited possibilities ...](getting-started/Part-99/README.md)| Get ideas for other interesting features you can implement in your own PLCnext datalogger application.|

---

## How to get support or provide suggestion and ideas
You can get support or provide suggestion and ideas in the forum of the [PLCnext Community](www.plcnext-community.net).

---

## License

Copyright (c) Phoenix Contact Gmbh & Co KG. All rights reserved.

Licensed under the [MIT](LICENSE) License.
