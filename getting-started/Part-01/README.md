This is part of a [series of articles](https://github.com/PLCnext/plcnext-real-time-datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.
In this use case IO signals of the local bus shall be recorded in a bussynchrinious manner.

## Part 1 - PLCnext Engineer Project

In this article, we will create a simple PLCnext Engineer project that we will use for:


- Creating a cyclic task for synchronization between AXIO local bus and DataLogger services

- Creating an ESM program with loggable Port-Variables and code for status change of these Port-Variables

- Instantiating an created ESM program under cyclic task and connecting of Port-Variables

- AXIO local bus configuration 

---

### Procedure

For synchronizing the DataLogger services with the AXIO local bus, a cyclic task with a 50 ms interval has to be defined in a PLCnext Engineer project.
Please follow these instructions for that:


---

![IEC_Program](Picture/01_IEC_Program.png)

•	Defining the variables
1.	Create an IEC Program “DIO_Producer” in the „Components“ area
2.	Select the „Variables“ tab and create the port variables with “bool” datatype 
and “IN Port” usage for the signals



---

![CyclicTask](Picture/02_CyclicTask.png)

•	Defining a cyclic task
1.	Open the “PLCnext” node in the „PLANT“ area 
2.	Select the „Tasks and Events “ tab
3.	Define a cyclic task with 50 ms task interval 
4.	Instantiate the „DIO_Producer“ program under the cyclic task by drag-and-drop below the “Cyclic1” list entry



---

![CyclicTask](Picture/03_Synchronizing.png)

•	Synchronizing the predefined AXIO update task with the user-defined cyclic task
1.	Open the „Axioline F“ node in the „PLANT“ area
2.	Select the „Settings“ tab
3.	Select the „Cyclic1“ task as „Update task“ and „Trigger task“ in the resp. drop-down menu



---

![CyclicTask](Picture/04_Configuring_AXIO_bus.png)

•	Configuring the AXIO local bus
1.	Select the „Device List“ topic
2.	Insert the AXIO module via the „Rule Picker“ to the bus configuration



---

![CyclicTask](Picture/05_ConnectingDIO.png)

•	Connecting the DIO data with the port variables
1.	Open the node „PLCnext“ in the „PLANT“ area
2.	Select the „Port List“ tab
3.	Connect the DIO process data with the respective port variables 



---

![CyclicTask](Picture/06_Download_PLCnEngProj.png)

•	Downloading the PLCnext Engineer Project to the PLCnext target
1.	Open the „axc-f-2152-1“ node in the „PLANT“ area
2.	Select the „Cockpit“ tab
3.	Write the PLCnext Engineer project to the PLCnext target via the „Write and start“ button (or press the [F5] key)
4.	Wire the Digital Input signals of AXL F DI8/3 DO8/3 2H module with the Digital Output signals (you can use the digital output signals of AXL F DI8/3 DO8/3 2H module or a signal generator)
5.	In debug mode, check the status change of the port variables during changing of the Digital Input signals.


---


Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.