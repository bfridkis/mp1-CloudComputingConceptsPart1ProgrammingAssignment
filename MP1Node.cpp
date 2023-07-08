/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
	this->guardianCount = 0;
	this->childCount = 0;
	this->pendingChildCount = 0;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_this node failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	//Message *msg;
	
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
		
		//Initialize memberList with self-entry. This entry will sit between guardians (at the front of the list up, to a count of MAX_GUARDIANS) and children (at the back of the list, up to a count of MAX_CHILDREN). Set timestamp = heartbeat on initialization for self-entry in memberList.
		int myID = stoi(memberNode->addr.getAddress().substr(0, memberNode->addr.getAddress().find_first_of(":")));
		short myPort = (short)stoi(memberNode->addr.getAddress().substr(memberNode->addr.getAddress().find_last_of(":")+1));
		memberNode->memberList.push_back(MemberListEntry(myID, myPort, memberNode->heartbeat, memberNode->heartbeat));
		//Set myPos iterator = to self-entry
		//memberNode->myPos = memberNode->memberList.begin();
		
		//multicast join message to all nodes in domain
		multicast(JOININF);
    }
    else {

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
		sendMessage(JOINREQ, joinaddr);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	 
	 //Initialize buffer for converting raw address data from message to string object to Address member. (Is either sender's address or, in the case of FailMessages, failed node's address.)
	 char buf[13];
	 
	 //Initialize pointer for dynamic Address object, used to convert raw Message address data to an Address object.
	 Address* fromAddr; 
	 //Initialize pointer for dynamic Address object, used to convert raw Message address data to an Address object.
	 Address* addrOfRemovedNode;
	 
	 //Initialize other variables for converting raw message data
	 int id;
	 short port;
	 long heartbeat;
	 short gc;
	 short cc;
	 vector<MemberListEntry>* sendersML;
	 
	 
	if (data[0] != FAILINF) {
		//Parse Message Data
		////Get from address from Message
		snprintf(buf, sizeof(buf), "%d.%d.%d.%d.%d:%d", *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6));
		string fromAddress;
		fromAddress.assign(buf);
		////Determine sender's id
		id = stoi(fromAddress.substr(0, fromAddress.find_first_of(".")));
		//Determine sender's port
		port = (short)stoi(fromAddress.substr(fromAddress.find_last_of(":")+1));
		//Determine sender's heartbeat
		memcpy(&heartbeat, data + 2 + sizeof(memberNode->addr.addr), sizeof(long));
		//Determine sender's Guardian Count
		gc = *(data + 3 + sizeof(memberNode->addr.addr) + sizeof(long));
		//Determine sender's Child Count
		cc = *(data + 4 + sizeof(memberNode->addr.addr) + sizeof(long) + sizeof(short));
		//Determine (pointer to) sender's memberList (Note normally senders list in its entirety would have to be send, but just using pointer for simulation.)
		sendersML = *((vector<MemberListEntry>**)(data + 5 + sizeof(memberNode->addr.addr) + sizeof(long) + sizeof(short)*2));

		//Create Address object from string fromAddress
		fromAddr = new Address(fromAddress);
	}
	 
	 //If message header is join request:
	 if (data[0] == JOINREQ) {

		//Send response back to node...
		sendMessage(JOINREP, fromAddr);
	 }
	 
	 //If message header is join reply:
	 if (data[0] == JOINREP) {
		//Join acknowledged by introducer. Node is now member of the group!
		memberNode->inGroup = true;
		//Initialize memberList with self-entry. This entry will sit between guardians (at the front of the list up, to a count of MAX_GUARDIANS) and children (at the back of the list, up to a count of MAX_CHILDREN). Set timestamp = heartbeat on initialization for self-entry in memberList.
		int myID = stoi(memberNode->addr.getAddress().substr(0, memberNode->addr.getAddress().find_first_of(":")));
		short myPort = (short)stoi(memberNode->addr.getAddress().substr(memberNode->addr.getAddress().find_last_of(":")+1));
		memberNode->memberList.push_back(MemberListEntry(myID, myPort, memberNode->heartbeat, memberNode->heartbeat));
		//Set myPos iterator = to self-entry
		//memberNode->myPos = memberNode->memberList.begin();
		//Now, multicast guardian requests to the network.
		multicast(GRDREQ);
		//Multicast join inform message to the network.
		multicast(JOININF);
	 }
	 
	 if (data[0] == JOININF) {
		 //Log acknowledgment of a successful join
		 log->logNodeAdd(&(memberNode->addr), fromAddr);
	 }
	 
	 if (data[0] == GRDREQ) {
		 if (childCount + pendingChildCount < MAX_CHILDREN) { 
			//Make sure child requesting is not already in memberList as a child node.
			bool alreadyChildNode = false;
			for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin() + guardianCount; it != memberNode->memberList.end(); it++) {
				if (id == it->id) { 
					alreadyChildNode = true;
					break;
				}
			}
			if (!alreadyChildNode) {
				//Hold a spot for the child until confirm or deny... (Note: Need code to periodically reset childCount equal to actual number of children incase neither GRDCNF nor GRDABT is received back in response to GRDACK in a reasonable timeframe...)
				pendingChildCount++;
				//Send GRDACK back to requesting node
				sendMessage(GRDACK, fromAddr);
			}
		}
	 }
	 
	 if (data[0] == GRDACK) {
		if (guardianCount < MAX_GUARDIANS) {
			//Make sure guardian requested is not already present as guardian in memberList
			bool alreadyGuardianNode = false;
			for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.begin() + guardianCount; it++) {
				if (id == it->id) { 
					alreadyGuardianNode = true;
					break;
				}
			}
			if (!alreadyGuardianNode) {
			
				//Add acknowledging guardian to recipients memberList, specifically to the front, as the guardians are stored in the front of the list. Set timestamp initially equal to heartbeat.
				memberNode->memberList.insert(memberNode->memberList.begin(), MemberListEntry(id, port, heartbeat, heartbeat));
				guardianCount++;
				//Push back myPos iterator (which points to self entry).
				//memberNode->myPos++;
				
				//Send GRDCNF back to sender
				sendMessage(GRDCNF, fromAddr);
				 
				//Log node added to memberList
				//log->logNodeAdd(&(memberNode->addr), fromAddr);
			}
			//Abort the guardian request if node already has this node registered as a guardian
			else { sendMessage(GRDABT, fromAddr); } 
		}
		//Abort the guardian request if node already has MAX_GUARDIANS established
		else { sendMessage(GRDABT, fromAddr); }
	 }
	 
	 if (data[0] == GRDCNF) {
		//If Guardianship confirmed, initially set timestamp equal to node's heartbeat.
		memberNode->memberList.push_back(MemberListEntry(id, port, heartbeat, heartbeat));
		//Increment childCount and decrement pendingChildCount as the pending child is registered as child.
		childCount++;
		pendingChildCount--;
		
		//Log node added to memberList
		//log->logNodeAdd(&(memberNode->addr), fromAddr);
	 }
	 
	 //If a node confirms 3 gaurdians before this one is accepted as a guardian, it will send back a guardian abort message. Rollback pendingChildCount if GRDABT is received.
	 if (data[0] == GRDABT && pendingChildCount > 0) {
		pendingChildCount--; 
	}
	 
	 if (data[0] == HBMSG) {
		//Update all member's of MemberListEntry table with newer heartbeats where applicable. Skip self-entry.
		for (vector<MemberListEntry>::iterator sender_ml_it = sendersML->begin(); sender_ml_it != sendersML->end(); sender_ml_it++) {
			for (vector<MemberListEntry>::iterator recipient_ml_it = memberNode->memberList.begin(); recipient_ml_it != memberNode->memberList.end(); recipient_ml_it++) {
				if (sender_ml_it->id == recipient_ml_it->id) {
					if (recipient_ml_it->heartbeat < sender_ml_it->heartbeat) {
						recipient_ml_it->heartbeat = sender_ml_it->heartbeat;
						//if(sender_ml_it->heartbeat < 0 || recipient_ml_it->heartbeat < 0) { cout << "HB Update | Recipient address: " << memberNode->addr.getAddress() << " ID (recipient): " << recipient_ml_it->id << " ID (sender): " << sender_ml_it->id << " heartbeat (recipient): " << recipient_ml_it->heartbeat << " heartbeat (sender): " << sender_ml_it->heartbeat << " GT: " << par->globaltime << endl; }
					}
					//break;
				}
			}
		}
		//Send HBACK message back to child/sender (HBMSG messages only come from children, not guardians)
		////Send guardian acknowledge message back
		sendMessage(HBACK, fromAddr);
	 }
	 
	 if (data[0] == HBACK) {
		 //Update Guardian entry in MemberListEntry table after receiving heartbeat acknowledge
		 for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++) {
			if (it->id == id) {
				it->heartbeat = heartbeat;
				break;
			}
		 }
		 /*for (vector<MemberListEntry>::iterator sender_ml_it = sendersML->begin(); sender_ml_it != sendersML->end(); sender_ml_it++) {
			for (vector<MemberListEntry>::iterator recipient_ml_it = memberNode->memberList.begin(); recipient_ml_it != memberNode->memberList.end(); recipient_ml_it++) {
				if (sender_ml_it->id == recipient_ml_it->id && sender_ml_it->id == id) {
					if (recipient_ml_it->heartbeat < sender_ml_it->heartbeat) {
						recipient_ml_it->heartbeat = sender_ml_it->heartbeat;
						//if(sender_ml_it->heartbeat < 0 || recipient_ml_it->heartbeat < 0) { cout << "HB Update | Recipient address: " << memberNode->addr.getAddress() << " ID (recipient): " << recipient_ml_it->id << " ID (sender): " << sender_ml_it->id << " heartbeat (recipient): " << recipient_ml_it->heartbeat << " heartbeat (sender): " << sender_ml_it->heartbeat << " GT: " << par->globaltime << endl; }
					}
					//break;
				}
			}
		}*/
	 }
	 if (data[0] == FAILINF) {
		//Parse FailMessage Data
		////Get from address from FailMessage
		snprintf(buf, sizeof(buf), "%d.%d.%d.%d.%d:%d", *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6));
		string failedAddress;
		failedAddress.assign(buf);
		////Determine failed node's id
		int id = stoi(failedAddress.substr(0, failedAddress.find_first_of(".")));
		//Determine failed node's port
		short port = (short)stoi(failedAddress.substr(failedAddress.find_last_of(":")+1));
		
		//Search for failedNode in memberList
		bool failedNodeInMemberList = false;
		for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++) {
			if (it->id == id) { 
				failedNodeInMemberList = true;
				break;
			}
		}
		//If the reported [failed] node is in memberList, skip logging failure (as node will be removed from memberList and logged as failed during nodeLoopOps(), once local TREMOVE threshold is reached). Otherwise, log failure.
		if (!failedNodeInMemberList && std::find(failedNodes.begin(), failedNodes.end(), id) == failedNodes.end()) { 
			//Create Address object from string failedAdress
			addrOfRemovedNode = new Address(failedAddress);
			//Log node removal
			log->logNodeRemove(&(memberNode->addr), addrOfRemovedNode);
			//Add to failedNodes list to prevent repeat logging
			failedNodes.push_back(id);
			delete addrOfRemovedNode;
		}
	 }
		
	 //Cleanup
	 if (data[0] != FAILINF) { delete fromAddr; }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
	
	//Update this member nodes' heartbeat and self-entry in memberList
	memberNode->heartbeat++;
	if (!memberNode->memberList.empty()) {
		//memberNode->myPos = memberNode->memberList.begin();
		//memberNode->myPos->heartbeat++;
		((memberNode->memberList.begin()+guardianCount)->heartbeat) = memberNode->heartbeat;
	}
	//((memberNode->memberList.begin()+guardianCount)->heartbeat)++;
	//memberNode->myPos->heartbeat++;
	
	//If guardianCount is < MAX_GUARDIANS, multicast GRDREQ messages
	if (guardianCount < MAX_GUARDIANS) {
		multicast(GRDREQ); 
	}
	
	//Make an Address object for each guardian and send heartbeat message
	int i; vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
	for (int i = 0; i < guardianCount; i++, it++) {
		char buf[13];
		snprintf(buf, sizeof(buf), "%d.0.0.0.0:0", it->id);
		string toAddress;
		toAddress.assign(buf);
		//Create Address object from string toAddr
		Address* toAddr = new Address(toAddress);
		sendMessage(HBMSG, toAddr);
		//Cleanup
		delete toAddr;
	}
	
	//Loop through memberList and update timestamp for each, 
	//then determine if any members are suspected failed or ready for removal.
	for (it = memberNode->memberList.begin(); it != memberNode->memberList.end();) {
		
		//Testing Only
		//if (par->globaltime < 200 && it->heartbeat < 0) {
		//cout << "This node address: " << memberNode->addr.getAddress() << " MemberListEntry id: " << it->id << " heartbeat: " << it->heartbeat << " timestamp: " << it->timestamp << " GC: " << guardianCount << " CC: " << childCount << " Size of memberList: " << memberNode->memberList.size() << endl;
		//}
		
		//If node's new timestamp is greater than FAIL_THRESHOLD counts above node's heartbeat, failure is suspected
		if (++(it->timestamp) - it->heartbeat > TREMOVE) {
			//If failed member is a guardian, need to decrement guardian count, so new guardian can be established next cycle
			if (it - memberNode->memberList.begin() < guardianCount) { 
				guardianCount--;
				//Move up myPos iterator when a guardian is removed.
				//memberNode->myPos--;
			}
			//Else, decrement childCount, as removed node is a child node
			else { childCount--; }
			
			//Make an Address object for node being removed (Address object required for logNodeRemove function).
			char buf[13];
			snprintf(buf, sizeof(buf), "%d.0.0.0.0:0", it->id);
			string addressOfRemovedNode;
			addressOfRemovedNode.assign(buf);
			
			//Create Address object from string addressOfRemovedNode
			Address* addrOfRemovedNode = new Address(addressOfRemovedNode);
			
			//cout <<"Node detecting failure: " << memberNode->addr.getAddress() << " Failed node id: " << it->id << " heartbeat: " << it->heartbeat << " timestamp: " << it->timestamp << endl;
			
			//save id of node to be removed, so it can be added to failedNodes container, to avoid re-logging failure
			int failedID = it->id;
			
			//Remove failed node from memberList
			memberNode->memberList.erase(it);
			
			//cout << "It points to " << it->id << " after removal." << endl;
			
			//If node has not already been reported failed, add failed node id to failedNodes list (to avoid re-logging same failure) and log failure.
			if (std::find(failedNodes.begin(), failedNodes.end(), failedID) == failedNodes.end()) {
				failedNodes.push_back(failedID);
				log->logNodeRemove(&(memberNode->addr), addrOfRemovedNode);
			}
			
			//multicast failure inform message so all other nodes can remove failed node
			multicast(FAILINF, addrOfRemovedNode);
			
			//Cleanup
			delete addrOfRemovedNode;
		}
		else { it++; }		
	}
	
	
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

/**
 * FUNCTION NAME: sendMessage
 *
 * DESCRIPTION: Wrapper for emulNet->ENsend to send message with all pertinent data fields
 * 					
 */
 void MP1Node::sendMessage(MsgTypes mt, Address* toAddr, Address* failedAddr) {
	 if (mt != FAILINF) {
		 Message *msg;
		 msg = (Message *) malloc(sizeof(Message));
		 // load message type
		 msg->msgType = mt;
		 // load sender's address
		 memcpy((char *)(msg) + 1, &memberNode->addr.addr, sizeof(memberNode->addr.addr));
		 // load sender's heartbeat
		 memcpy((char *)(msg) + sizeof(memberNode->addr.addr) + 2, &memberNode->heartbeat, sizeof(long));
		 // load sender's guardian count
		 memcpy((char *)(msg) + sizeof(memberNode->addr.addr) + sizeof(long) + 3, &guardianCount, sizeof(short));
		 // load sender's child count
		 memcpy((char *)(msg) + sizeof(memberNode->addr.addr) + sizeof(long) + sizeof(short) + 4, &childCount, sizeof(short));
		 // load sender's memberList
		 vector<MemberListEntry>* memberListAddress = &memberNode->memberList;
		 memcpy((char *)(msg) + sizeof(memberNode->addr.addr) + sizeof(long) + sizeof(short)*2 + 5, &memberListAddress, sizeof(vector<MemberListEntry>*)); 
		 // send message
		 emulNet->ENsend(&(memberNode->addr), toAddr, (char *)msg, sizeof(Message));
		 
		 free(msg);
	 }
	 else {
		 FailMessage *failMsg;
		 failMsg = (FailMessage *) malloc(sizeof(FailMessage));
		 // load message type
		 failMsg->msgType = mt;
		 // load failed node's address
		 memcpy((char *)(failMsg) + 1, failedAddr->addr, sizeof(failedAddr->addr));
		 // send message
		 emulNet->ENsend(&(memberNode->addr), toAddr, (char *)failMsg, sizeof(FailMessage));
		 
		 free(failMsg);
	 }
 }
 
 /**
 * FUNCTION NAME: multicast
 *
 * DESCRIPTION: Function to send a message to all nodes in subnet in randomized order
 * 
 * Reference:  https://cplusplus.com/reference/algorithm/shuffle/
 */
 void MP1Node::multicast(MsgTypes mt, Address* failedAddr) {
//void MP1Node::multicast() {	 
	
	//Load a vector with the number of addresses
	std::vector<int> order;
	for (int i=0; i<par->EN_GPSZ; i++) { order.push_back(i+1); }
	
	// obtain a time-based seed:
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	
	//Shuffle Order
	shuffle (order.begin(), order.end(), std::default_random_engine(seed));
	
	//Print shuffled elements (testing only)
	//std::cout << "shuffled elements:";
	//for (int x: order) std::cout << ' ' << x;
	//std::cout << '\n';
	
	//Make Addresses (Specific for x.0.0.0:0 address space for this assignment.) and send multicast message
	for (int x: order) {
		char buf[13];
		snprintf(buf, sizeof(buf), "%d.0.0.0.0:0", x);
		string toAddress;
		toAddress.assign(buf);
		//cout << "toAdress inside multicast: " << toAddress << endl;
		//Create Address object from string toAddr
		Address* toAddr = new Address(toAddress);
		if (memberNode->addr == *toAddr) { }
		else { 
			//if (mt == GRDREQ && par->globaltime < 5) { cout << "Requesting Guardian @: " << toAddress << endl; }
			if (mt != FAILINF) { sendMessage(mt, toAddr); }
			else { sendMessage(mt, toAddr, failedAddr) ; }
		}
		//emulNet->ENsend(&(memberNode->addr), toAddr, (char *)msgToMultiCast, sizeof(Message));	
		//Cleanup
		delete toAddr;
	}
 }
 
 // References:
 // https://stackoverflow.com/questions/47232384/non-repeating-random-numbers-in-vector-c
 // https://stackoverflow.com/questions/571394/how-to-find-out-if-an-item-is-present-in-a-stdvector