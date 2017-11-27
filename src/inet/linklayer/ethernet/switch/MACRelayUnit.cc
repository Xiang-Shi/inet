//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/linklayer/ethernet/switch/MACRelayUnit.h"
#include "inet/linklayer/ethernet/EtherFrame.h"
#include "inet/linklayer/ethernet/EtherMACBase.h"
#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/queue/DropTailQueue.h"
#include "inet/linklayer/ethernet/switch/IMACAddressTable.h"
#include <string>
#include "math.h"
#include "time.h"
#include "chistogram.h"

namespace inet {

Define_Module(MACRelayUnit);

#define Threshold 0.000000
#define A 45

#define N 3 //number of upstream switches, varies with topology

simsignal_t MACRelayUnit:: maxPBHopSignal = registerSignal("maxPBHop");
simsignal_t MACRelayUnit:: totalHopNumSignal =registerSignal("totalHopNum");

int MACRelayUnit::max(int a,int b,int c,int d,int e,int f)
{
    int temp1 =a>b?a:b;
    int temp2 =c>d?c:d;
    int temp3 =e>f?e:f;
    int temp4 =temp1>temp2?temp1:temp2;

    return temp4>temp3?temp4:temp3;
}


//get the utilization of this switch-> eth[specified port/frame outputport]
double MACRelayUnit::getPortUtilization( int port, int passBackNum )
{
    int queueLength, queueCapacity, realCapacity;
    double queueUtil;
    DropTailQueue * queue;

    if (passBackNum == 0)
    {
        queue = check_and_cast<DropTailQueue *>(getParentModule()->getSubmodule("eth",port)->getSubmodule("queue")->getSubmodule("dataQueue"));

    }
    else
    {
        queue = check_and_cast<DropTailQueue *>(getParentModule()->getSubmodule("eth",port)->getSubmodule("queue")->getSubmodule("passBackQueue"));
    }

    queueLength = queue->getLength();
    queueCapacity = queue->getframeCapacity();
    realCapacity = queueCapacity - N;

    queueUtil = (double)(1.0*queueLength)/(queueCapacity);

    //round the util to be 1, to avoid packet loss during parallel decision making of packets
    if (queueLength > realCapacity)
    {
        queueUtil = 1;
    }

    return queueUtil;
}



void MACRelayUnit::initialize(int stage)
{

    if (stage == INITSTAGE_LOCAL) {
        // number of ports
        numPorts = gate("ifOut", 0)->size();
        if (gate("ifIn", 0)->size() != numPorts)
            throw cRuntimeError("the sizes of the ifIn[] and ifOut[] gate vectors must be the same");

        numProcessedFrames = numDiscardedFrames = 0;
        addressTable = check_and_cast<IMACAddressTable *>(getParentModule()->getSubmodule("macTable"));

        maxPBHopDistr.setNumCells(8);
        totalHopNumDistr.setCellSize(1);
        totalHopNumDistr.setRange(0,100);

        switchName = getParentModule()->getFullName();

        WATCH(PassBackFreq);
        WATCH(numFramesOfHop0);
        WATCH(numFramesOfHop1);
        WATCH(numFramesOfHop2);
        WATCH(numFramesOfHop3);
        WATCH(maxPBFreqOfFrame);
        WATCH(maxHopOfFrame);
        WATCH(maxPBHopOfFrame);
        WATCH(totalFrameNum);
        WATCH(numProcessedFrames);
        WATCH(numDiscardedFrames);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
    }

}

void MACRelayUnit::handleMessage(cMessage *msg)
{
    if (!isOperational) {
        EV << "Message '" << msg << "' arrived when module status is down, dropped it\n";
        delete msg;
        return;
    }
    EtherFrame *frame = check_and_cast<EtherFrame *>(msg);
    // Frame received from MAC unit
    handleAndDispatchFrame(frame);
}

// method used to collect statistics at the bottom switch(currently switch7)
void MACRelayUnit::collectStatistics(EtherFrame *frame)
{
    //count total frame number
    totalFrameNum++;

    //count pass back hop distribution
    if (frame->getMaxPassBackHop() == 0)
        numFramesOfHop0++;
    else if (frame->getMaxPassBackHop() == 1)
        numFramesOfHop1++;
    else if (frame->getMaxPassBackHop() == 2)
        numFramesOfHop2++;
    else if (frame->getMaxPassBackHop() == 3)
        numFramesOfHop3++;

    //collect max Pass Back Hop distribution
    maxPBHopDistr.collect(frame->getMaxPassBackHop());
    emit(maxPBHopSignal,frame->getMaxPassBackHop());

    //collect total hop num distribution and calculate average hop
    totalHopNumDistr.collect(frame->getTotalHopNum());
    emit(totalHopNumSignal,frame->getTotalHopNum());


    if (maxPBFreqOfFrame < frame->getPassBackNum())
        maxPBFreqOfFrame = frame->getPassBackNum();
    if (maxPBHopOfFrame < frame->getMaxPassBackHop())
        maxPBHopOfFrame = frame->getMaxPassBackHop();
    if (maxHopOfFrame < frame->getTotalHopNum())
        maxHopOfFrame = frame->getTotalHopNum();

}

void MACRelayUnit::handleAndDispatchFrame(EtherFrame *frame)
{

    int inputport = frame->getArrivalGate()->getIndex();
    int passBackHop = 0 , maxPassBackHop = 0,  passBackNum = 0 , totalHopNum = 0;

    passBackHop = frame->getPassBackHop(); //pass back hop of the frame
    maxPassBackHop = frame->getMaxPassBackHop();//max pass back hop of the frame
    passBackNum = frame->getPassBackNum(); //pass back num of the frame
    totalHopNum = frame->getTotalHopNum(); //total hop num of the frame

    numProcessedFrames++;

    // update address table
    if(passBackNum == 0)
    {
        addressTable->updateTableWithAddress(inputport, frame->getSrc());
    }

    // handle broadcast frames first
    if (frame->getDest().isBroadcast()) {
        EV << "Broadcasting broadcast frame " << frame << endl;
        broadcastFrame(frame, inputport);
        return;
    }

    // Finds output port of destination address and sends to output port
    // if not found then broadcasts to all other ports instead
    // should not send out the same frame on the same ethernet port
    // (although wireless ports are ok to receive the same message)

    int outputport = addressTable->getPortForAddress(frame->getDest());


/*
    //no passback
    if (inputport == outputport) {
        EV << "Output port is same as input port, " << frame->getFullName()
               << " dest " << frame->getDest() << ", discarding frame\n";
        numDiscardedFrames++;
        delete frame;
        return;
        }

    if (outputport >= 0) {
        EV << "Sending frame " << frame << " with dest address " << frame->getDest() << " to port " << outputport << endl;
        //send frame
        send(frame, "ifOut", outputport);
    }
    else {
        EV << "Dest address " << frame->getDest() << " unknown, broadcasting frame " << frame << endl;
        broadcastFrame(frame, inputport);
    }
*/
//---------------------------------------------------------------------------------------------
    //with passback
    double probability,dice,temp;


    if (outputport >= 0)
    {

        utilization = getPortUtilization(outputport, passBackNum);


        //check whether the occupy rate of the queue exceeds a number
        if(utilization>Threshold)
        {

            //------------------------probability function: exponential decay----------------------------
            //original
            //   temp = pow(exp(A*(passBackHop+1)),(utilization-Threshold));
            //   probability = (temp - 1)/ (pow(exp(A*(passBackHop+1)),(1-Threshold))-1);
            temp = pow(exp(A*(passBackHop+1)),(Threshold - utilization));
            probability = (temp - 1)/ (pow(exp(A*(passBackHop+1)),(Threshold - 1))-1);

            //------------------------probability function: exponential decay----------------------------
            dice = dblrand();

            EV<<"switch: "<<switchName<<" ,port: "<<outputport<<" passing back frames "<<frame<<"(passBackNum= "<<frame->getPassBackNum()<<" ) ,with probability = "<<probability<<" , dice = "<<dice<<endl;


            //PASSING BACK: construct a event occur at probability P, then pass back frame
            if(dice<=probability) //passbacks
            {
                outputport = addressTable->getPortForAddress(frame->getSrc());
                passBackNum++;
                frame->setPassBackNum(passBackNum);

                //passBackHop: add when being passed back
                passBackHop++;
                frame->setPassBackHop(passBackHop);

                //record the max hop value of the frame
                if( maxPassBackHop < passBackHop )
                {
                    maxPassBackHop = passBackHop;
                    frame->setMaxPassBackHop(maxPassBackHop);
                }

                totalHopNum++;
                frame->setTotalHopNum(totalHopNum);

                //count the number of passed back frames in this switch
                if(passBackNum > 0)
                    PassBackFreq++;

                EV << "Passing back frame " << frame << " with src address " << frame->getSrc()
                                     <<" at the "<<frame->getPassBackHop() <<" hop "<< " to port " << outputport << endl;

                //pass back frame
                send(frame, "ifOut", outputport);

            }
            else
            {
                EV << "Sending frame " << frame << " with dest address " << frame->getDest() << " to port " << outputport << endl;

                //passBackHop: only reduce when >=1
                if(passBackHop >= 1)
                {
                    passBackHop--;
                    frame->setPassBackHop(passBackHop);
                }

                totalHopNum++;
                frame->setTotalHopNum(totalHopNum);

                //collect statistics before sending to receiver
                if(strcmp(switchName,"etherSwitch7")==0 && outputport == 3)
                {
                    collectStatistics(frame);
                }

                //send frame
                send(frame, "ifOut", outputport);
            }
        }

        else
        {
            EV << "Sending frame " << frame << " with dest address " << frame->getDest() << " to port " << outputport << endl;
            //passBackHop: only reduce when >=1
            if(passBackHop >= 1)
            {
                passBackHop--;
                frame->setPassBackHop(passBackHop);
            }

            totalHopNum++;
            frame->setTotalHopNum(totalHopNum);

            //collect statistics before sending to receiver
            if (strcmp(switchName, "etherSwitch7") == 0 && outputport == 3)
            {
                collectStatistics(frame);
            }

            //send frame
            send(frame, "ifOut", outputport);
        }
    }
    else
    {
        EV << "Dest address " << frame->getDest() << " unknown, broadcasting frame " << frame << endl;
        broadcastFrame(frame, inputport);
    }

}



void MACRelayUnit::broadcastFrame(EtherFrame *frame, int inputport)
{
    for (int i = 0; i < numPorts; ++i)
        if (i != inputport)
            send((EtherFrame *)frame->dup(), "ifOut", i);

    delete frame;
}

void MACRelayUnit::start()
{
    addressTable->clearTable();
    isOperational = true;
}

void MACRelayUnit::stop()
{
    addressTable->clearTable();
    isOperational = false;
}

bool MACRelayUnit::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();

    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_LINK_LAYER) {
            start();
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_LINK_LAYER) {
            stop();
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH) {
            stop();
        }
    }
    else {
        throw cRuntimeError("Unsupported operation '%s'", operation->getClassName());
    }

    return true;
}

void MACRelayUnit::finish()
{
    //output scalar values
    recordScalar("passback frame frequency",PassBackFreq);
    recordScalar("processed frames", numProcessedFrames);
    recordScalar("discarded frames", numDiscardedFrames);

    maxPBHopDistr.recordAs("Pass back hop distribution");
    totalHopNumDistr.recordAs("Total hop num distribution");


}

} // namespace inet

