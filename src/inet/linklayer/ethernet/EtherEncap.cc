/*
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "inet/linklayer/ethernet/EtherEncap.h"

#include "inet/linklayer/ethernet/EtherFrame.h"
#include "inet/linklayer/ethernet/EtherMacBase.h" //SHI: for retrieve sibling mac module
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"

namespace inet {

Define_Module(EtherEncap);

simsignal_t EtherEncap::encapPkSignal = registerSignal("encapPk");
simsignal_t EtherEncap::decapPkSignal = registerSignal("decapPk");
simsignal_t EtherEncap::pauseSentSignal = registerSignal("pauseSent");
simsignal_t EtherEncap:: reversePassbackSignal = registerSignal("reversePassback");
simsignal_t EtherEncap::totalHopSignal = registerSignal("totalHop");

void EtherEncap::initialize()
{
    seqNum = 0;
    WATCH(seqNum);

    totalReversePassedBack = totalFromHigherLayer = totalFromMAC = totalPauseSent = 0;
    useSNAP = par("useSNAP").boolValue();

    totalHopNumDistr.setCellSize(1); //SHI
    totalHopNumDistr.setRange(0,100);

    bouncedHopNumDistr.setCellSize(1);//SHI
    bouncedHopNumDistr.setRange(0,10);

    WATCH(totalReversePassedBack);
    WATCH(totalFromHigherLayer);
    WATCH(totalFromMAC);
    WATCH(totalPauseSent);
    WATCH(numOfBouncedPackets);
}

void EtherEncap::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("lowerLayerIn")) {
        EV_INFO << "Received " << msg << " from lower layer." << endl;
        processFrameFromMAC(check_and_cast<EtherFrame *>(msg));
    }
    else {
        EV_INFO << "Received " << msg << " from upper layer." << endl;
        // from higher layer
        switch (msg->getKind()) {
            case IEEE802CTRL_DATA:
            case 0:    // default message kind (0) is also accepted
                processPacketFromHigherLayer(PK(msg));
                break;

            case IEEE802CTRL_SENDPAUSE:
                // higher layer want MAC to send PAUSE frame
                handleSendPause(msg);
                break;

            default:
                throw cRuntimeError("Received message `%s' with unknown message kind %d", msg->getName(), msg->getKind());
        }
    }

    if (hasGUI())
        updateDisplayString();
}

void EtherEncap::updateDisplayString()
{
    char buf[80];
    sprintf(buf, "passed up: %ld\nsent: %ld", totalFromMAC, totalFromHigherLayer);
    getDisplayString().setTagArg("t", 0, buf);
}

void EtherEncap::processPacketFromHigherLayer(cPacket *msg)
{
    if (msg->getByteLength() > MAX_ETHERNET_DATA_BYTES)
        throw cRuntimeError("packet from higher layer (%d bytes) exceeds maximum Ethernet payload length (%d)", (int)msg->getByteLength(), MAX_ETHERNET_DATA_BYTES);

    totalFromHigherLayer++;
    emit(encapPkSignal, msg);

    // Creates MAC header information and encapsulates received higher layer data
    // with this information and transmits resultant frame to lower layer

    // create Ethernet frame, fill it in from Ieee802Ctrl and encapsulate msg in it
    EV_DETAIL << "Encapsulating higher layer packet `" << msg->getName() << "' for MAC\n";

    IMACProtocolControlInfo *controlInfo = check_and_cast<IMACProtocolControlInfo *>(msg->removeControlInfo());
    Ieee802Ctrl *etherctrl = dynamic_cast<Ieee802Ctrl *>(controlInfo);
    EtherFrame *frame = nullptr;

    if (useSNAP) {
        EtherFrameWithSNAP *snapFrame = new EtherFrameWithSNAP(msg->getName());

        snapFrame->setSrc(controlInfo->getSourceAddress());    // if blank, will be filled in by MAC
        snapFrame->setDest(controlInfo->getDestinationAddress());
        snapFrame->setOrgCode(0);
        if (etherctrl)
            snapFrame->setLocalcode(etherctrl->getEtherType());
        snapFrame->setByteLength(ETHER_MAC_FRAME_BYTES + ETHER_LLC_HEADER_LENGTH + ETHER_SNAP_HEADER_LENGTH);
        frame = snapFrame;
    }
    else {
        EthernetIIFrame *eth2Frame = new EthernetIIFrame(msg->getName());

        eth2Frame->setSrc(controlInfo->getSourceAddress());    // if blank, will be filled in by MAC
        eth2Frame->setDest(controlInfo->getDestinationAddress());
        if (etherctrl)
            eth2Frame->setEtherType(etherctrl->getEtherType());
        eth2Frame->setByteLength(ETHER_MAC_FRAME_BYTES);
        frame = eth2Frame;
    }
    delete controlInfo;

    frame->encapsulate(msg);
    if (frame->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
        frame->setByteLength(MIN_ETHERNET_FRAME_BYTES); // "padding"

    EV_INFO << "Sending " << frame << " to lower layer.\n";
    send(frame, "lowerLayerOut");
}
/**
 * SHI: In Sender,Before decapsulate, check whether passed back, if yes, put into send buffer
 */
void EtherEncap::processFrameFromMAC(EtherFrame *frame)
{
    int passBackHop = frame->getPassBackHop();
    int totalHopNum = frame->getTotalHopNum();

    //SHI: if sender received passed back frames (identify sender according to its name in the network)

    //SHI: to exclude frame destinated for own
    EtherMACBase * mac = (EtherMACBase *) getParentModule()->getSubmodule("mac");


    // exclude frame destinated for own, find frames being passed back all the way back to the sender host, and
    // put these frames into sender buffer , queueing for resend
    if (  (  strcmp(getParentModule()->getParentModule()->getNedTypeName(),"inet.node.inet.StandardHost") == 0
            || strcmp(getParentModule()->getNedTypeName(),"inet.node.ethernet.EtherHost2")==0  )
           && !mac->getMACAddress().equals(frame->getDest())  && frame->getPassBackNum() > 0 )
    {
        numOfBouncedPackets++;

        //Reduce passBackHop: only reduce when >=1
        if (passBackHop >= 1) {
            passBackHop--;
            frame->setPassBackHop(passBackHop);

        }
        totalHopNum++;
        frame->setTotalHopNum(totalHopNum);

        EV_INFO << "Resending pass-backed" << frame << " to lower layer.\n";

        //send the passed-back frame into data queue(see the EthernetInterface structure)
        send(frame, "lowerLayerOut");

    }
    else
    {
        //SHI: check if the sender received passed back frames destined for itself, this is bi-directional passback, we don't want this happen
        if (strcmp(getParentModule()->getParentModule()->getNedTypeName(),"inet.node.inet.StandardHost") == 0
                || strcmp(getParentModule()->getNedTypeName(),"inet.node.ethernet.EtherHost2")==0 )
        { // for EtherEncap module in the hosts, when they are processing frames from the network...

            if(frame->getPassBackNum() > 0)
            {//record the number of received bounced frames in each host
                totalReversePassedBack++;
                emit(reversePassbackSignal,totalReversePassedBack);
            }

            // collect statistics of the frames received in the host
            emit(totalHopSignal, frame->getTotalHopNum());
            totalHopNumDistr.collect(frame->getTotalHopNum());
            bouncedHopNumDistr.collect(frame->getPassBackNum());


        }

        // decapsulate and attach control info
        cPacket *higherlayermsg = frame->decapsulate();

        // add Ieee802Ctrl to packet
        Ieee802Ctrl *etherctrl = new Ieee802Ctrl();
        etherctrl->setSrc(frame->getSrc());
        etherctrl->setDest(frame->getDest());
        if (dynamic_cast<EthernetIIFrame *>(frame) != nullptr)
            etherctrl->setEtherType(((EthernetIIFrame *)frame)->getEtherType());
        else if (dynamic_cast<EtherFrameWithSNAP *>(frame) != nullptr)
            etherctrl->setEtherType(((EtherFrameWithSNAP *)frame)->getLocalcode());
        higherlayermsg->setControlInfo(etherctrl);

        EV_DETAIL << "Decapsulating frame `" << frame->getName() << "', passing up contained packet `"
                  << higherlayermsg->getName() << "' to higher layer\n";

        totalFromMAC++;
        emit(decapPkSignal, higherlayermsg);

        // pass up to higher layers.
        EV_INFO << "Sending " << higherlayermsg << " to upper layer.\n";
        send(higherlayermsg, "upperLayerOut");
        delete frame;
    }

}

void EtherEncap::handleSendPause(cMessage *msg)
{
    Ieee802Ctrl *etherctrl = dynamic_cast<Ieee802Ctrl *>(msg->removeControlInfo());
    if (!etherctrl)
        throw cRuntimeError("PAUSE command `%s' from higher layer received without Ieee802Ctrl", msg->getName());
    int pauseUnits = etherctrl->getPauseUnits();
    MACAddress dest = etherctrl->getDest();
    delete etherctrl;

    EV_DETAIL << "Creating and sending PAUSE frame, with duration = " << pauseUnits << " units\n";

    // create Ethernet frame
    char framename[40];
    sprintf(framename, "pause-%d-%d", getId(), seqNum++);
    EtherPauseFrame *frame = new EtherPauseFrame(framename);
    frame->setPauseTime(pauseUnits);
    if (dest.isUnspecified())
        dest = MACAddress::MULTICAST_PAUSE_ADDRESS;
    frame->setDest(dest);
    frame->setByteLength(ETHER_PAUSE_COMMAND_PADDED_BYTES);

    EV_INFO << "Sending " << frame << " to lower layer.\n";
    send(frame, "lowerLayerOut");
    delete msg;

    emit(pauseSentSignal, pauseUnits);
    totalPauseSent++;
}

void EtherEncap::finish()
{
    totalHopNumDistr.recordAs("Total hop num distribution");
    bouncedHopNumDistr.recordAs("Bounced hop num distribution");
}
} // namespace inet

