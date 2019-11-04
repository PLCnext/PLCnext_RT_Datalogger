 /******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CppDataLoggerProgram.hpp
 *
 *  Created on: Jun, 2019
 *      Author: Eduard Muenz
 *
 ******************************************************************************/

#pragma once
#include "Arp/System/Core/Arp.h"
#include "Arp/Plc/Commons/Esm/ProgramBase.hpp"
#include "Arp/System/Commons/Logging.h"
#include "CppDataLoggerComponent.hpp"

namespace CppDataLogger
{

using namespace Arp;
using namespace Arp::System::Commons::Diagnostics::Logging;
using namespace Arp::Plc::Commons::Esm;

//#program
//#component(CppDataLogger::CppDataLoggerComponent)
class CppDataLoggerProgram : public ProgramBase, private Loggable<CppDataLoggerProgram>
{
public: // typedefs

public: // construction/destruction
    CppDataLoggerProgram(CppDataLogger::CppDataLoggerComponent& cppDataLoggerComponentArg, const String& name);
    CppDataLoggerProgram(const CppDataLoggerProgram& arg) = delete;
    virtual ~CppDataLoggerProgram() = default;

public: // operators
    CppDataLoggerProgram&  operator=(const CppDataLoggerProgram& arg) = delete;

public: // properties

public: // operations
    void    Execute() override;

public: /* Ports
           =====
           Ports are defined in the following way:
           //#port
           //#attributes(Input|Retain)
           //#name(NameOfPort)
           boolean portField;

           The attributes comment define the port attributes and is optional.
           The name comment defines the name of the port and is optional. Default is the name of the field.
        */

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

private: // fields
    CppDataLogger::CppDataLoggerComponent& cppDataLoggerComponent;
};

///////////////////////////////////////////////////////////////////////////////
// inline methods of class ProgramBase
inline CppDataLoggerProgram::CppDataLoggerProgram(CppDataLogger::CppDataLoggerComponent& cppDataLoggerComponentArg, const String& name)
: ProgramBase(name)
, cppDataLoggerComponent(cppDataLoggerComponentArg)
{
	Log::Info("DL Constructor");
}

} // end of namespace CppDataLogger
