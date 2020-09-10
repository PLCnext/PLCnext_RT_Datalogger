 /******************************************************************************
 *
 *  Copyright (c) Phoenix Contact GmbH & Co. KG. All rights reserved.
 *	Licensed under the MIT. See LICENSE file in the project root for full license information.
 *
 *  CppDataLoggerComponent.hpp
 *
 *  Created on: Jun, 2019
 *      Author: Eduard Muenz
 *
 ******************************************************************************/

#pragma once
#include "Arp/System/Core/Arp.h"
#include "Arp/System/Acf/ComponentBase.hpp"
#include "Arp/System/Acf/IApplication.hpp"
#include "Arp/Plc/Commons/Esm/ProgramComponentBase.hpp"
#include "CppDataLoggerComponentProgramProvider.hpp"
#include "CppDataLoggerLibrary.hpp"
#include "Arp/Plc/Commons/Meta/MetaLibraryBase.hpp"
#include "Arp/System/Commons/Logging.h"


#include "Arp/System/Acf/IControllerComponent.hpp"
#include "Arp/System/Commons/Threading/WorkerThread.hpp"
#include "Arp/System/Commons/Threading/Thread.hpp"
#include "Arp/System/Commons/Threading/ThreadSettings.hpp"

#include "Arp/System/Rsc/ServiceManager.hpp"
#include "Arp/Services/DataLogger/Services/IDataLoggerService2.hpp"
#include "Arp/System/Rsc/Services/RscVariant.hxx"
#include "Arp/System/Rsc/Services/RscType.hpp"
#include "Arp/System/Rsc/Services/RscArrayReader.hpp"
#include "Arp/Services/DataLogger/Services/ErrorCode.hpp"
#include "Arp/Plc/Gds/Services/VariableInfo.hpp"

#include <mutex>

namespace CppDataLogger
{

using namespace Arp;
using namespace Arp::System::Acf;
using namespace Arp::Plc::Commons::Esm;
using namespace Arp::Plc::Commons::Meta;

using namespace Arp::System::Rsc;
using namespace Arp::System::Rsc::Services;
using namespace Arp::Plc::Gds::Services;
using namespace Arp::Services::DataLogger::Services;

using namespace Arp::System::Acf;
using namespace Arp::Plc::Commons::Esm;
using namespace Arp::Plc::Commons::Meta;

//#component
class CppDataLoggerComponent
		: public ComponentBase
		, public ProgramComponentBase
		, private Loggable<CppDataLoggerComponent>
		, public IControllerComponent
{
public: // typedefs

public: // construction/destruction
    CppDataLoggerComponent(IApplication& application, const String& name);
    virtual ~CppDataLoggerComponent() = default;

public: // IComponent operations
    void Initialize() override;
    void LoadConfig() override;
    void SetupConfig() override;
    void ResetConfig() override;

public: // IControllerComponent operations
    void Start(void)override;
    void Stop(void)override;

public: // ProgramComponentBase operations
    void RegisterComponentPorts() override;

private: // methods
    CppDataLoggerComponent(const CppDataLoggerComponent& arg) = delete;
    CppDataLoggerComponent& operator= (const CppDataLoggerComponent& arg) = delete;

    void workerThreadBody(void);
    bool Init();

    ErrorCode ReadVariablesDataToByte(const Arp::String& sessionName,
   	        const Arp::DateTime& startTime, const Arp::DateTime& endTime,
   			const std::vector<Arp::String>& variableNames, uint8* byteMemory);

public: // static factory operations
    static IComponent::Ptr Create(Arp::System::Acf::IApplication& application, const String& name);

private: // fields
    CppDataLoggerComponentProgramProvider programProvider;

    	  //Worker Thread
          WorkerThread workerThreadInstance;
          bool xStopThread = false;
          bool m_bInitialized = false;	// class already initialized?

          // IDataLoggerService Handle
          IDataLoggerService2::Ptr m_pDataLoggerService;

          //Session Name
          Arp::String sessionname = {};

          //Vector for Variable Names, sorted by name. This vector will be necessary in the next part of this article
          std::vector<Arp::String> CountingVariableNames = {};

          //Start and End time as time window parameter
          Arp::DateTime startTime;
          Arp::DateTime endTime;

          //Define the buffer for the records. Please note, this code is very critical because,
          //if the memory is not enough the storage will be written beyond the array limits!
          uint8 m_records[10] = {0};

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

public:   // IProgramComponent operations
          uint32 GetRecord(uint8* byteRecord,  bool &b_PN_DataValidBit); //will be called in program execution

public: /* Ports
           =====
           Component ports are defined in the following way:

           //#port
           //#attributes(Hidden)
           struct PORTS {
               //#name(NameOfPort)
               //#attributes(Input|Retain|Opc)
               Arp::boolean portField = false;
               // The GDS name is "<componentName>/NameOfPort" if the struct is declared as Hidden
               // otherwise the GDS name is "<componentName>/PORTS.NameOfPort"
           } ports;

           Create one (and only one) instance of this struct.
           Apart from this single struct instance, there must be no other Component variables declared with the #port comment.
           The only attribute that is allowed on the struct instance is "Hidden", and this is optional.
           The struct can contain as many members as necessary.
           The #name comment can be applied to each member of the struct, and is optional.
           The #name comment defines the GDS name of an individual port element. If omitted, the member variable name is used as the GDS name.
           The members of the struct can be declared with any of the attributes allowed for a Program port.
        */
};

///////////////////////////////////////////////////////////////////////////////
// inline methods of class CppDataLoggerComponent
inline CppDataLoggerComponent::CppDataLoggerComponent(IApplication& application, const String& name)
: ComponentBase(application, ::CppDataLogger::CppDataLoggerLibrary::GetInstance(), name, ComponentCategory::Custom)
, programProvider(*this)
, ProgramComponentBase(::CppDataLogger::CppDataLoggerLibrary::GetInstance().GetNamespace(), programProvider)

// ADDED: Worker Thread
, workerThreadInstance(make_delegate(this, &CppDataLoggerComponent::workerThreadBody) , 100, "WorkerThreadName")
{
}

inline IComponent::Ptr CppDataLoggerComponent::Create(Arp::System::Acf::IApplication& application, const String& name)
{
    return IComponent::Ptr(new CppDataLoggerComponent(application, name));
}

} // end of namespace CppDataLogger
