#include "Fault.h"

const char* faultName(FaultType type){
	switch(type){
		case FaultType::None:				return "None";
		case FaultType::InvalidOpcode:		return "InvalidOpcode";
		case FaultType::InvalidRegister:	return "InvalidRegister";
		case FaultType::IllegalWrite:		return "IllegalWrite";
		case FaultType::UnmappedAccess:		return "UnmappedAccess";
		case FaultType::StackOverflow:		return "StackOverflow";
		case FaultType::StackUnderflow:		return "StackUnderflow";
		case FaultType::InvalidJumpTarget:	return "InvalidJumpTarget";
	}
	return "Unknown";
}
