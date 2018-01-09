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

#ifndef __INET_MACRELAYUNIT_H
#define __INET_MACRELAYUNIT_H

#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/queue/DropTailQueue.h"
#include <string>

#include "inet/linklayer/ethernet/switch/IMACAddressTable.h"

namespace inet {

class EtherFrame;

class INET_API MACRelayUnit : public cSimpleModule, public ILifecycle
{
  protected:

    IMACAddressTable *addressTable = nullptr;


    const char * switchName="";
    double utilization = 0;
    int PassBackFreq =0;    //the total frequency of passback in this switch;

    int numPorts = 0;

    // Parameters for statistics collection
    double maxUtilOfOutport = 0;


    long numProcessedFrames = 0;
    long numDiscardedFrames = 0;
    long Count=0;
    bool isOperational = false;    // for lifecycle

    //Vector Parameters for statistics collection
  //  cLongHistogram maxPBHopDistr;
  //  cLongHistogram totalHopNumDistr;
   // static simsignal_t maxPBHopSignal;
   // static simsignal_t totalHopNumSignal;


  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    /**
     * Updates address table with source address, determines output port
     * and sends out (or broadcasts) frame on ports. Includes calls to
     * updateTableWithAddress() and getPortForAddress().
     *
     * The message pointer should not be referenced any more after this call.
     */
    virtual void handleAndDispatchFrame(EtherFrame *frame);

    /**
     * Utility function: sends the frame on all ports except inputport.
     * The message pointer should not be referenced any more after this call.
     */
    virtual void broadcastFrame(EtherFrame *frame, int inputport);
   // virtual void collectStatistics(EtherFrame *frame);

    /**
     * Calls handleIncomingFrame() for frames arrived from outside,
     * and processFrame() for self messages.
     */
    virtual void handleMessage(cMessage *msg) override;

    /**
     * Writes statistics.
     */
    virtual void finish() override;

    // for lifecycle:

  public:
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;

  protected:
    virtual void start();
    virtual void stop();

    virtual int max(int a=0,int b=0,int c=0,int d=0,int e=0,int f=0);
    /**
     * get occupancy rate of the outputport data queue of this switch
     */
    virtual double getPortUtilization(int port, int passBackNum);



};

} // namespace inet

#endif // ifndef __INET_MACRELAYUNIT_H

