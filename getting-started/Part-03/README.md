This is part of a [series of articles](https://github.com/PLCnext/plcnext-real-time-datalogger) that demonstrate how to implement a DataLogger application based on the DataLogger service provided by the PLCnext Control firmware.  Each article builds on tasks that were completed in earlier articles, so it is recommended to follow the series in sequence.

## Part 3 - OPC UA Historical Access

The PLCnext embedded OPC UA Server can provide historical data for every configured GDS port assigned to the datalogger.  
Please configure the demanded GDS port as described in the youtube video to the PLCnext datalogger and mark the `OPC option`in PLCnext Engineer. 
After the changes are downloaded to the PLCnext Control it is possible to read the historical values with any given OPC UA HA client.

Please find also a [Tutorial video on Youtube](https://youtu.be/Npu_obHw1rY?) from Phoenix Contact that gives a brief overview and commisionning of OPC UA Historical Access.

---

Copyright © 2019 Phoenix Contact Electronics GmbH

All rights reserved. This program and the accompanying materials are made available under the terms of the [MIT License](http://opensource.org/licenses/MIT) which accompanies this distribution.
