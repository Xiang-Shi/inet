//
// Copyright (C) 2015 Andras Varga
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
// Author: Andras Varga
//

#ifndef __INET_IDUPLICATEDETECTOR_H
#define __INET_IDUPLICATEDETECTOR_H

#include "inet/common/INETDefs.h"

namespace inet {
namespace ieee80211 {

class Ieee80211DataOrMgmtFrame;

class INET_API IDuplicateDetector
{
    public:
        virtual void assignSequenceNumber(Ieee80211DataOrMgmtFrame *frame) = 0;
        virtual bool isDuplicate(Ieee80211DataOrMgmtFrame *frame) = 0;
        //TODO some purge mechanism
        virtual ~IDuplicateDetector() {}
};

} // namespace ieee80211
} // namespace inet

#endif
