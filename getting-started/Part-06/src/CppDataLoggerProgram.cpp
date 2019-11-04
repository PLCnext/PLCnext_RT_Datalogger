 /******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CppDataLoggerProgram.cpp
 *
 *  Created on: Jun, 2019
 *      Author: Eduard Muenz
 *
 ******************************************************************************/

#include "CppDataLoggerProgram.hpp"
#include "Arp/System/Commons/Logging.h"
#include "Arp/System/Core/ByteConverter.hpp"


namespace CppDataLogger
{
 
void CppDataLoggerProgram::Execute()
{
    //implement program 
	//Call the reference to the method in the component.
	//The Execute Program method will be called in real time context
	QueueSize = cppDataLoggerComponent.GetRecord(OutPortPN, Cpp_Pn_Valid_Data_Cycle_In);

}
} // end of namespace CppDataLogger
