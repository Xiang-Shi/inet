/*
 * TCPSentSeqQueue.h
 *
 *  Created on: Jul 6, 2017
 *      Author: shixiang
 */

#ifndef TCPSENTSEQQUEUE_H_
#define TCPSENTSEQQUEUE_H_

#include "inet/common/INETDefs.h"

#include "inet/transportlayer/tcp/TCPConnection.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"

namespace inet {

namespace tcp {

/**
 * SHI: Record tcp segment sent sequence from state->snd_una to state->snd_max,
 *      to provide sequence reference for retransmitted segment, thus for
 *      correctness of calculating reordering density
 */
class INET_API TCPSentSeqQueue
{
public:
    TCPConnection *conn;    // the connection that owns this queue

    struct Region {
        uint32 beginSeqNum;  //the begin of the segment
        uint32 endSeqNum;   //the end of the segment
        uint32 sentSeq;     //indicates the corresponding sent sequence of the segment
    };

    typedef std::list<Region> SentSeqQueue;
    SentSeqQueue sentSeqQueue; // sentSeqQueue is ordered by seqnum, and doesn't have overlapped Regions

    uint32 begin;    // 1st sequence number stored
    uint32 end;    // last sequence number stored + 1

public:
   /**
    * Ctor
    */
    TCPSentSeqQueue();

   /**
    * Virtual dtor.
    */
   virtual ~TCPSentSeqQueue();

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
    * Prints the current sentSeqQueue status for debug purposes.
    */
   virtual void info() const;

   virtual uint32 getBufferStartSeq() const { return begin; }

   virtual uint32 getBufferEndSeq() const { return end; }

   /**
    * Tells the queue that bytes up to (but NOT including) seqNum have been
    * transmitted and ACKed, so they can be removed from the queue.
    */
   virtual void discardUpTo(uint32 seqNum);

   /**
    * Inserts sent data to the sentSeq queue.
    */
   virtual void enqueueSentDataSeq(uint32 fromSeqNum, uint32 toSeqNum, uint32 sentSeq);
   /**
    * Find the send sequence number of the corresponding data region
    */
   virtual uint32 findSentDataSeq(uint32 fromSeqNum, uint32 toSeqNum);

   /**
    * Retransmit the head segment of the TCPSentSeqQueue
    */
   virtual void getSegment(TCPSegment * tcpseg, uint32 snd_nxt);

   /**
    * Returns the number of blocks currently buffered in queue.
    */
   virtual uint32 getQueueLength() const { return sentSeqQueue.size(); }


 protected:
   /*
    * Returns if TCPSentSeqQueue is valid or not.
    */
   bool checkQueue() const;
};


} // namespace tcp

} // namespace inet
#endif /* TCPSENTSEQQUEUE_H_ */
