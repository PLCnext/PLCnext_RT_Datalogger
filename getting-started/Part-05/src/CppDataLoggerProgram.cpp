#include "CppDataLoggerProgram.hpp"
#include "Arp/System/Commons/Logging.h"
#if ARP_ABI_VERSION_MAJOR < 2
#include "Arp/System/Core/ByteConverter.hpp"
#else
#include "Arp/Base/Core/ByteConverter.hpp"
#endif

namespace CppDataLogger
{
 
void CppDataLoggerProgram::Execute()
{
    //implement program 
    //implement program 
	//Call the reference to the method in the component.
	//The Execute Program method will be called in real time context
	QueueSize = cppDataLoggerComponent.GetRecord(OutPortPN, Cpp_Pn_Valid_Data_Cycle_In);
}

} // end of namespace CppDataLogger
