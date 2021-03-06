//
// Copyright (C) 2011 Zoltan Bojthe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/linklayer/ethernet/EtherFrameClassifier.h"

#include "inet/linklayer/ethernet/EtherFrame.h"

namespace inet {

Define_Module(EtherFrameClassifier);

void EtherFrameClassifier::handleMessage(cMessage *msg)
{
    if (dynamic_cast<EtherPauseFrame *>(msg) != nullptr)
        send(msg, "pauseOut");
    else
    {
        EtherFrame *frame = dynamic_cast<EtherFrame *>(msg);
        if(frame != nullptr)
        {
            //SHI: if passed back frame , put into passBackQueue
            if(frame->getPassBackNum()>0)
                send(msg, "passBackOut");
            else
                send(msg, "defaultOut");
        }

    }
}

} // namespace inet

