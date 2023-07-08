/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include <chrono>       // std::chrono::system_clock
#include <algorithm>    // std::shuffle
#include <random>       // std::default_random_engine

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define MAX_GUARDIANS 3
#define MAX_CHILDREN 6

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,	//Join Request
    JOINREP,	//Join Response
	JOININF,	//Join Inform
	GRDREQ,		//Guardian Request
	GRDACK,		//Guardian Acknowledge
	GRDCNF,		//Guardian Confirm
	GRDABT,		//Guardian Abort
	HBMSG,		//Heartbeat Message
	HBACK,		//Heartbeat Acknowledge
	FAILINF,	//Failure Inform
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: Message
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct Message {
	enum MsgTypes msgType;
	char addr[6];
	long heartbeat;
	short guardianCount;
	short childCount;
	vector<MemberListEntry>* memberList;
}Message;

/**
 * STRUCT NAME: FailMessage
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct FailMessage {
	enum MsgTypes msgType;
	char addr[6];
}FailMessage;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];
	short guardianCount;
	short childCount;
	short pendingChildCount;
	vector<int> failedNodes;		//Used to track failed nodes for logging purposes, to prevent duplicate log entries

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	short getGuardianCount() {
		return guardianCount;
	}
	void setGuardianCount(short guardianCount) {
		this->guardianCount = guardianCount;
	}
	void incGuardianCount() {
		this->guardianCount++;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	void sendMessage(MsgTypes mt, Address* toAddr, Address* failedAddr = nullptr);
	void multicast(MsgTypes mt, Address* failedAddr = nullptr);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
