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
