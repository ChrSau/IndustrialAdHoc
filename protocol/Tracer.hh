#ifndef CLICK_TRACER__HH
#define CLICK_TRACER__HH

#include <click/element.hh>
#include <click/timer.hh>
#include <clicknet/ether.h>
#include <clicknet/udp.h>
#include <iostream>
#include <fstream>
#include <string>

//

#define MODE_SINGLE 1
#define MODE_SUMMARY 2

CLICK_DECLS

/*
=c

Tracer(STICK,RTICK,ADD)

=s

Recieves connection tracing beacons and saves connections to file

=d

Logging beacons

*/

struct SaveBeacon{
	unsigned int sender;
	Vector<unsigned long long> indeces;
};

class Tracer : public Element {
	int _mode;

	Timer _timerS;					//Evaluation of beacons is in time independent of received beacons, timer is used
	Timer _timerR;
	unsigned int _stick;			//ms berween evaluations
	unsigned int _rtick;

	int headroom;					//size of header space, whigh must be reserved, when beacon is generated
	unsigned int _address;			//identificator of this client, usually last three digits of decimal IP address

	void run_timer(Timer *t);		//attach timer

	unsigned long long _index;		//index of the current beacon, constantly increments
	long long _startIndex;			//index at the start of the current processing time frame
	Vector<SaveBeacon> RP;			//saves senders and indeces of the received beacons
	std::string _fn;				//the file name to save to
	std::ofstream myfile;			//file stream of the save file
	unsigned long _entriesInFile;	//number of entries logged to the current log file
	unsigned int _fileIndex;		//Index of the currently active log file

	public:
		Tracer();			//Constructor
		~Tracer();		//Destructor

		const char *class_name() const { return "Tracer"; }	//name of element
		const char *port_count() const { return "1/1"; }			//type of element, one input and one output (output can be used as signal)
		const char *processing() const { return PUSH; }				//beacons are pushed into this element

		Packet *simple_action(Packet *p);							//Handling of incoming beacons
		int initialize(ErrorHandler *);								//setting up variables
		int configure(Vector<String> &conf, ErrorHandler *errh);	//reading configuration from string
		void Process();												//Processing the received beacons
		void Send();												//Sending a beacon
		int getSaveBufferIndex(unsigned int _sender);				//returns the index of a sender in the buffer, -1 if sender is not yet in buffer
		void LogToFileAndScreen(String S);							//simplyfied logging of data
};

CLICK_ENDDECLS

#endif
