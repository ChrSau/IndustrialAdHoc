// Tracer.click

// Simple configuration, that tracks the connections of clients in a IEEE802.11 mesh network.
// Uses broadcasted beacons to detecct the presence of other clients.
// Presence is logged to lastLog.csv in the directory of this configuration.

// Parameters:
// 216		Last three decimal digits of the IP address, used as identificator
// b8:27:eb:e6:33:6b		MAC address of the wlan0 interface
// TICK			Frequence for sending and evaluating beacons
//				Should be identical, otherwise over- or undersampling occurs

define($WIFACENAME wlan0);											//wlan0 is the used interface, must be configured according to following lines and in ad-hoc mode. Attention to channels, must also be the same.
AddressInfo($WIFACENAME 10.20.2.216 b8:27:eb:e6:33:6b);					//info of wlan0
define($RECVPORT 4322);												//two static ports, are not realy relevant
define($SENDPORT 1235);
define($BCIP 255.255.255.255);										//all beacons must be broadcast

T :: Tracer(STICK 200,RTICK 0,ADD 216)						//Sending and processing element
	-> UDPIPEncap($WIFACENAME, $SENDPORT, $BCIP, $RECVPORT)			//wrapping beacons, addressing them to broadcast
	-> Warpq :: ARPQuerier($WIFACENAME)								//ARPQuerier for wlan0
	-> WQ :: Queue													//wlan0 output queue
	-> ToDevice($WIFACENAME);										//device interface

FromDevice($WIFACENAME)												//incoming messages from wlan0
	-> wcl :: Classifier(12/0800,12/0806 20/0002,12/0806 20/0001)	//classified as ip, arpResp and arpQue
	-> CheckIPHeader(14)											//Checking header length of ip messages
	-> Strip(42)													//stripping headers from message
	-> T;															//logging incoming beacons

wcl[1] -> [1]Warpq;													//arpResp gehen an den Querier
wcl[2] -> ARPResponder($WIFACENAME)									//auf arpQue werden arpResp generiert
       -> WQ;														//und in die wlan0-queue gelegt
Warpq[1] -> WQ;														//durch den Querier generierte Queries werden in die Queue geschoben

