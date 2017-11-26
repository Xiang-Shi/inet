/*
 * TCPRcvSeqQueue.h
 *
 *  Created on: Oct 19, 2017
 *      Author: shixiang
 */

#ifndef TCPRCVSEQQUEUE_H_
#define TCPRCVSEQQUEUE_H_


#include "inet/common/INETDefs.h"

#include "inet/transportlayer/tcp/TCPConnection.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"

namespace inet {

namespace tcp {

/**
 * SHI: Record received TCP segment sent sequence,
 *      to provide information for calculating Reorder Buffer-Occupancy Density
 */
class INET_API TCPRcvSeqQueue
{
public:
    struct Region {
        uint32 beginSeqNum;  //the begin of the segment
        uint32 endSeqNum;   //the end of the segment
        uint32 sentSeq;     //indicates the corresponding sent sequence of the segment
        Region(){}
        Region(uint32 _beginSeqNum, uint32 _endSeqNum, uint32 _sentSeq) : beginSeqNum(_beginSeqNum), endSeqNum(_endSeqNum), sentSeq(_sentSeq) {}

    };

    typedef std::list<Region> RcvSeqQueue;
    RcvSeqQueue rcvSeqQueue; // rcvSeqQueue is ordered by seqnum, and doesn't have overlapped Regions

    uint32 begin;    // 1st sequence number stored
    uint32 end;    // last sequence number stored + 1
    uint32 rcv_nxt_seq;

    TCPConnection *conn;    // the connection that owns this queue


public:
   /**
    * Ctor
    */
    TCPRcvSeqQueue();

   /**
    * Virtual dtor.
    */
   virtual ~TCPRcvSeqQueue();

   /**
    * Set the connection that owns this queue.
    */
   virtual void setConnection(TCPConnection *_conn) { conn = _conn; }

   /**
    * Initialize the object.
    *
    * init() may be called more than once; every call flushes the existing contents
    * of the queue.
    */
   virtual void init(uint32 seqNum);

   /**
    * Returns a string for debug purposes.
    */
   virtual std::string str() const;

   /**
    * Prints the current rcvSeqQueue status for debug purposes.
    */
   virtual void info() const;

   virtual uint32 getBufferStartSeq() const { return begin; }

   virtual uint32 getBufferEndSeq() const { return end; }
   virtual uint32 getRcvNxtSeq() const {return rcv_nxt_seq; }

   /**
    *
    */
   virtual void extractBytesUpTo(uint32 seqNum);

   /**
    * Inserts rcv data to the rcvSeq queue. and return next expected snd sequence
    */
   virtual uint32 enqueueRcvSegment(TCPSegment* tcpseg);

   /**
    * Returns the number of blocks currently buffered in queue.
    */
   virtual uint32 getQueueLength() const { return rcvSeqQueue.size(); }

   /*
    * find continuous regions and returns rcv_nxt_seq
    */
   virtual uint32  findRcvNxtSeq();
};


} // namespace tcp

} // namespace inet



#endif /* TCPRCVSEQQUEUE_H_ */
