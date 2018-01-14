/*
 * TCPSentSeqQueue.cc
 *
 *  Created on: Jul 24, 2017
 *      Author: shixiang
 */

#include "inet/transportlayer/tcp/TCPSentSeqQueue.h"

namespace inet {

namespace tcp {

TCPSentSeqQueue::TCPSentSeqQueue()
{
    conn = nullptr;
    begin = end = 0;
}

TCPSentSeqQueue::~TCPSentSeqQueue()
{
    while (!sentSeqQueue.empty())
        sentSeqQueue.pop_front();
}

void TCPSentSeqQueue::init(uint32 seqNum)
{
    begin = seqNum;
    end = seqNum;
}

std::string TCPSentSeqQueue::str() const
{
    std::stringstream out;

    out << "[" << begin << ".." << end << ")";
    return out.str();
}

void TCPSentSeqQueue::info() const
{
    EV_DETAIL << str() << endl;

    uint j = 1;

    for (SentSeqQueue::const_iterator i = sentSeqQueue.begin(); i != sentSeqQueue.end(); i++) {
        EV_DETAIL << j << ". region: [" << i->beginSeqNum << ".." << i->endSeqNum << ".." << i->sentSeq
                  << ")"
                  << endl;
        j++;
    }
}

void TCPSentSeqQueue::discardUpTo(uint32 seqNum)
{
    ASSERT(seqLE(begin, seqNum) && seqLE(seqNum, end));

    info();


    if (!sentSeqQueue.empty()) {
        auto i = sentSeqQueue.begin();

        while ((i != sentSeqQueue.end()) && seqLE(i->endSeqNum, seqNum)) // discard/delete regions from sentSeqQueue, which have been acked
            i = sentSeqQueue.erase(i);

        if (i != sentSeqQueue.end()) {
            ASSERT(seqLE(i->beginSeqNum, seqNum) && seqLess(seqNum, i->endSeqNum));
            i->beginSeqNum = seqNum;
        }
    }
    EV_INFO << "sentSeqQueue: " << str() << " discardSentSeqData [" << begin << ".." << seqNum << ")\n";

    begin = seqNum;

    info();

    // TESTING queue:
    ASSERT(checkQueue());


}
uint32 TCPSentSeqQueue::findSentDataSeq(uint32 fromSeqNum, uint32 toSeqNum)
{
    uint32 sentSeq = 0;
    bool found = false;
    ASSERT(seqLE(begin, fromSeqNum) && seqLE(fromSeqNum, end));

    EV_INFO << "Duplicate sending:\n The sentSeqQueue is: \n" << str() << " finding Sent Data [" << fromSeqNum << ".." << toSeqNum << ")\n";

    if (!sentSeqQueue.empty())
    {
            auto i = sentSeqQueue.begin();

            //find the region
            while ((i != sentSeqQueue.end()) && seqLE(i->endSeqNum, fromSeqNum)) //seqLE
                i++;

            if (i != sentSeqQueue.end())
            {
                ASSERT(seqLE(i->beginSeqNum, fromSeqNum) && seqLess(fromSeqNum, i->endSeqNum));
               // ASSERT(seqLE(i->beginSeqNum, toSeqNum) && seqLess(toSeqNum, i->endSeqNum));

                sentSeq = i->sentSeq;
                found = true;
            }
    }

    ASSERT(found);

    info();
    EV_INFO << "sentSeqQueue: " << str()<< " findSentDataSeq [" << fromSeqNum << ".." << toSeqNum << "..sentSeq=" << sentSeq << ")\n";
    return sentSeq;

}
void TCPSentSeqQueue :: getSegment(TCPSegment * tcpseg, uint32 snd_nxt)
{
    bool found = false;

    if (!sentSeqQueue.empty())
       {
               auto i = sentSeqQueue.begin();

               //find the region
               while ((i != sentSeqQueue.end()) && seqLE(i->endSeqNum, snd_nxt)) //seqLE
                   i++;

               if (i != sentSeqQueue.end())
               {
                   ASSERT(seqLE(i->beginSeqNum, snd_nxt) && seqLess(snd_nxt, i->endSeqNum));
                   tcpseg->setSequenceNo(i->beginSeqNum);
                   tcpseg->setPayloadLength(i->endSeqNum - i->beginSeqNum);
                   tcpseg->setSendSeqNo(i->sentSeq);
                   found = true;
               }
       }

    ASSERT(found);
}


void TCPSentSeqQueue::enqueueSentDataSeq(uint32 fromSeqNum, uint32 toSeqNum , uint32 sentSeq)
{

    //ensure the continuity of the region
    ASSERT(seqLE(begin, fromSeqNum) && seqLE(fromSeqNum, end));

    bool found = false;
    Region region;



    EV_INFO << "sentSeqQueue: " << str() << " enqueueSentSeqData [" << fromSeqNum << ".." << toSeqNum << "..sentSeq=" << sentSeq << ")\n";

    ASSERT(seqLess(fromSeqNum, toSeqNum));


    if (sentSeqQueue.empty() || (end == fromSeqNum)) {
        region.beginSeqNum = fromSeqNum;
        region.endSeqNum = toSeqNum;
        region.sentSeq = sentSeq;
        sentSeqQueue.push_back(region);
        found = true;
        fromSeqNum = toSeqNum;
    }


    //ensure the new region is inserted
    ASSERT(fromSeqNum == toSeqNum);

    if (!found) {
        EV_DEBUG << "Fail recording send sequence of segment[" << fromSeqNum << ", " << toSeqNum << "): sentSeq = "<< sentSeq <<"\nThe Queue is:\n";
        info();
    }

    ASSERT(found);

    begin = sentSeqQueue.front().beginSeqNum;
    end = sentSeqQueue.back().endSeqNum;

    // TESTING queue:
    ASSERT(checkQueue());

    info();



}

bool TCPSentSeqQueue::checkQueue() const
{
    uint32 b = begin;
    bool f = true;

    for (SentSeqQueue::const_iterator i = sentSeqQueue.begin(); i != sentSeqQueue.end(); i++) {
        f = f && (b == i->beginSeqNum);
        f = f && seqLess(i->beginSeqNum, i->endSeqNum);
        b = i->endSeqNum;
    }

    f = f && (b == end);

    if (!f) {
        EV_DEBUG << "Invalid Queue\nThe Queue is:\n";
        info();
    }

    return f;
}



} // namespace tcp

} // namespace inet

