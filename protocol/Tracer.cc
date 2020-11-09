#include <click/config.h>
#include <click/args.hh>
#include <click/timer.hh>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "Tracer.hh"
#include "trace_proto.hh"

CLICK_DECLS

//Constructor with timer
Tracer::Tracer() : _timerS((Element *) this), _timerR((Element *) this) {};

//Destructor
Tracer::~Tracer() { };

//initializes file name and schedules timer
int Tracer::initialize(ErrorHandler *){
	//intialize timers
	//begin sending beacons immidiatly
	//wait a bit for receiving beacons
	_timerS.initialize((Element *) this);
	_timerS.schedule_now();
	_timerR.initialize((Element *) this);
	if(_mode == MODE_SUMMARY){
		_timerR.schedule_after_ms(_rtick);		//for some reason this gives a deprecated warning, in the official git I cant find anything about deprecation
	}

	//file name is static
	_fn = "lastLog_";
	_fn += std::to_string(Timestamp::now().sec());
	_fn += "_";
	_fileIndex = 0;

	RP.clear();		//clearing list in the beginning
	_index = 0;		//start index at 0
	_startIndex = 0;//reset start index

	return 0;
}

//reads the only configuration parameter
int Tracer::configure(Vector<String> &conf, ErrorHandler *errh){

	//set headroom to required size
	headroom = sizeof(click_ip) + sizeof(click_udp) + sizeof(click_ether);

	//read configuration string
	if(Args(conf, this, errh)
		.read_mp("STICK", _stick)	//ms between beacons
		.read_mp("RTICK", _rtick)	//ms between evaluations
		.read_mp("ADD",_address)	//identificator added to beacon
		.complete()<0){
			return -1;
		}

	//set mode in accordance to RTICK
	_mode = (_rtick <= 0) ? MODE_SINGLE : MODE_SUMMARY;

	return 0;

}

//default timer call for both timers
void Tracer::run_timer(Timer *t){

	//test for timer
	if(&_timerS == t){
		//if send timer was called, reset it and send a beacon
		_timerS.reschedule_after_msec(_stick);
		Send();
	}else if(&_timerR == t && _mode == MODE_SUMMARY){
		//if receive timer was called, reset it and process beacons
		//this is not neccessary in single mode
		_timerR.reschedule_after_msec(_rtick);
		Process();
	}else{
		//default action, should never be called
		click_chatter("Tracer got unknown Timer");
	}

	return;
}

//Function for sending beacons
void Tracer::Send(){

	//construct and fill beacon
	struct TraceMsg T;
	T.sender = _address;
	T.index = _index;

	//construct packet from beacon
	WritablePacket *Pack = WritablePacket::make(headroom, &T, sizeof(TraceMsg), 0);
	
	//send beacon
	output(0).push(Pack);

	//set the start index to the current index, if it was just reset
	if(_startIndex < 0){
		_startIndex = _index;
	}

	//increment index
	_index++;

	return;
}

//Function for processing the received beacons
//logs the received beacons to screen and file
void Tracer::Process(){

	//Target format of String / file
	//Sender1(Index1,Index2,...),Sender2(Index1),...

	//create string
	String S;
	S = "";													//reset string to empty

	//add the time frame of this processing iteration
	S += "[";
	S += String(_startIndex);
	S += "-";
	S += String(_index);
	S += "],";
	_startIndex = -1;										//reset start index

	//construct log string based on received beacons
	if(!RP.empty()){										//if beacons were received

		for(int i = 0; i < RP.size(); i++){						//iterate through all received senders
			S += String(RP.at(i).sender);							//add the senders address to the string

			//Add indeces of received beacons to file/string
			S += "(";												//start format with (
			for(int j = 0; j < RP.at(i).indeces.size(); j++){		//iterate through all indeces of received beacons
				S += String(RP.at(i).indeces.at(j));					//add index to string
				if(j < RP.at(i).indeces.size() - 1){					//if it is not the last index
					S += ",";												//add , to the string for formatting
				}
			}
			S += ")";												//end index list with )

			if(i < RP.size() - 1){									//if it was not the last element
				S += ",";												//add a , for csv-formatting
			}
		}

		RP.clear();							//clear out the list after we saved the content to the string
	}

	//log results
	LogToFileAndScreen(S);

	return;
};

//unified function for logging of Strings
//auto appends the linefeed
void Tracer::LogToFileAndScreen(String S){
	click_chatter("%s\n",S.c_str());	//log to screen

	//construct file name from session name + file index
	std::string filename = _fn;
	filename += std::to_string(_fileIndex);
	filename += ".csv";

	//log to file
	myfile.open(filename.c_str(),std::ios_base::app);	//open the file
	myfile << S.c_str() << std::endl;					//append string and line feed to it
	myfile.close();

	//if file has 30000 entries, start a new file (this is at least 10min of data)
	_entriesInFile++;
	if(_entriesInFile > 30000){
		_fileIndex++;
		_entriesInFile = 0;
	}
}

//returns the index of a sender in the Buffer if it is present, else returns a -1
int Tracer::getSaveBufferIndex(unsigned int _sender){
	//iterate through buffer
	for(int i = 0; i < RP.size(); i++){
		//if the searched sender is found
		if(RP.at(i).sender == _sender){
			//return its index
			return i;
		}
	}

	//if no sender was found return -1
	return -1;
}

//simple action used for logging the beacons
Packet *Tracer::simple_action(Packet *p) {
	
	//cast to beacon type
	struct TraceMsg *T = (struct TraceMsg*) p->data();

	if(_mode == MODE_SUMMARY){								//if we run in summary mode
		//save received beacon to list
		//check if saveBeaconBuffer allready contains this sender
		int knownIndex = getSaveBufferIndex(T->sender);
		if(knownIndex < 0){									//sender was previously unknown
			SaveBeacon SB;										//generate new saveEntry
			SB.sender = T->sender;								//set the address of the sender
			SB.indeces.push_back(T->index);						//push the received index
			RP.push_back(SB);									//push new entry to list
		}else{												//sender was known
			RP.at(knownIndex).indeces.push_back(T->index);		//push additional index to known entry
		}
	}else{													//if we run in single mode
		String S;

		//Plausibility check
		if((T->sender < 200 || T->sender > 220) && T->index > 1000000){
			return NULL;									//abort logging if there is an error in 
		}

		//Generate a string of the format:
		//receiver,receiver index,sender,sender index

		S += String(_address);								//first entry is the own address
		S += ",";
		S += String(_index);								//log own index
		S += ",";											//we use comma seperated formatting
		S += String(T->sender);								//log address of the sender of this beacon
		S += ",";
		S += String(T->index);								//log index of the beacon
		LogToFileAndScreen(S);								//save the results
	}

	//resend message, may be used for signaling
	return NULL;

};CLICK_ENDDECLS

EXPORT_ELEMENT(Tracer)
