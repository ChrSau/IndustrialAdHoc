#ifndef TRACEPROTO_HH
#define TRACEPROTO_HH

// Trace-Protocol
// Simple protocol for tracking the connectivity in mesh network


//TraceMsg is only message type and a type of beacon
//contains indtifier because it is simpler, than accessing and deconstructing the sender IP
//also allows for example multiple access points to act like one participant in the network
struct TraceMsg {
#if CLICK_BYTE_ORDER == CLICK_BIG_ENDIAN
	unsigned int sender : 16;					//16-bit sender address
	unsinged long long : 64;					//64-bit index of sender (might be a bit long)
#elif CLICK_BYTE_ORDER == CLICK_LITTLE_ENDIAN
	unsigned int sender : 16;
	unsigned long long index : 64;
#else
#error "Undefined Byte Order!"
#endif
} CLICK_SIZE_PACKED_ATTRIBUTE;

#endif
