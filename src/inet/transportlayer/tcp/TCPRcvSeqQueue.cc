/*
 * TCPRcvSeqQueue.cc
 *
 *  Created on: Oct 19, 2017
 *      Author: shixiang
 */
#include "inet/transportlayer/tcp/TCPRcvSeqQueue.h"
#include "inet/transportlayer/tcp_common/TCPSegment_m.h"
#include "inet/transportlayer/tcp/TCPConnection.h"


namespace inet {

namespace tcp {

TCPRcvSeqQueue::TCPRcvSeqQueue()
{
    conn = nullptr;
    begin = end =  0;
    rcv_nxt_seq = 1;
}

TCPRcvSeqQueue::~TCPRcvSeqQueue()
{
    while (!rcvSeqQueue.empty())
        rcvSeqQueue.pop_front();
}

void TCPRcvSeqQueue::init(uint32 seqNum)
{
    begin = seqNum;
    end = seqNum;
    rcv_nxt_seq = 1;
}

std::string TCPRcvSeqQueue::str() const
{
    std::stringstream out;

    out << "[" << begin << ".." << end << "): rcv_nxt_seq = "<<rcv_nxt_seq<<" , size = "<<rcvSeqQueue.size()<<"\n";
    return out.str();
}

void TCPRcvSeqQueue::info() const
{
    EV_DETAIL << str() << endl;

    uint j = 1;

    for (RcvSeqQueue::const_iterator i = rcvSeqQueue.begin(); i != rcvSeqQueue.end(); i++) {
        EV_DETAIL << j << ". region: [" << i->beginSeqNum << ".." << i->endSeqNum << "): sentSeq =" << i->sentSeq
                  << endl;
        j++;
    }
}

void TCPRcvSeqQueue::extractBytesUpTo(uint32 seqNum)
{
    ASSERT(seqLE(begin, seqNum) && seqLE(seqNum, end));

    EV_INFO << "rcvSeqQueue: " << str() << " discardRcvSeqData [" << begin << ".." << seqNum << ")\n";

    if (!rcvSeqQueue.empty()) {
        auto i = rcvSeqQueue.begin();

        while ((i != rcvSeqQueue.end()) && seqLE(i->endSeqNum, seqNum))
            i = rcvSeqQueue.erase(i);

        if (i != rcvSeqQueue.end()) {
            if((!seqLE(i->beginSeqNum, seqNum) && seqLess(seqNum, i->endSeqNum)))
                begin = i->beginSeqNum;
            else
            {
                i->beginSeqNum = seqNum;
                begin = seqNum;
            }
        }
    }

}

uint32 TCPRcvSeqQueue::enqueueRcvSegment(TCPSegment* tcpseg)
{
    bool found = false;

    uint32 fromSeqNum, toSeqNum, sentSeq;

    fromSeqNum = tcpseg->getSequenceNo();
    toSeqNum = tcpseg->getSequenceNo() + tcpseg->getPayloadLength();
    sentSeq = tcpseg->getSendSeqNo();


    conn->printConnBrief();

    EV_DETAIL << "rcvSeqQueue: " << str() << " enqueueRcvSeqData [" << fromSeqNum << ".." << toSeqNum << "..sentSeq = " << sentSeq << ")\n";

    auto i = rcvSeqQueue.begin();
    while (i != rcvSeqQueue.end() && seqLess(i->beginSeqNum, fromSeqNum))
        ++i;

    // insert, avoiding duplicates
    if (i != rcvSeqQueue.end() && i->beginSeqNum == fromSeqNum)
        delete tcpseg;
    else
    {
        i = rcvSeqQueue.insert(i,Region(fromSeqNum,toSeqNum,sentSeq));
        found = true;
        ASSERT(seqLE(rcvSeqQueue.front().beginSeqNum, rcvSeqQueue.back().beginSeqNum));
    }

    if (!found) {
        EV_DETAIL << "Fail recording receiving sequence of segment(" << fromSeqNum << ", " << toSeqNum << "):"<< sentSeq <<"\nThe Queue is:\n";
        info();
    }

    ASSERT(found);

    begin = rcvSeqQueue.front().beginSeqNum;
    end = rcvSeqQueue.back().endSeqNum;


    info(); //check insert segments

    if(rcv_nxt_seq == sentSeq)
    {
        //rcv_nxt_seq++;  //current expected segment has arrived
        findRcvNxtSeq(); //find if there are other continuous segments
    }

    return rcv_nxt_seq;

}

uint32 TCPRcvSeqQueue::findRcvNxtSeq()
{

    RcvSeqQueue::iterator i = rcvSeqQueue.begin();
    while(i != rcvSeqQueue.end())
    {
        if(i->sentSeq == rcv_nxt_seq)
            rcv_nxt_seq++;
        i++;
    }
  //  if(i == rcvSeqQueue.end() && i->sentSeq == rcv_nxt_seq)
    //        rcv_nxt_seq++;

    EV_DETAIL<< "rcvSeqQueue: " << str() << " rcv_nxt_seq =" << rcv_nxt_seq <<"\n";
}



} // namespace tcp

} // namespace inet





