/*
 * FattreeAddressTable.cc
 *
 *  Created on: 2017年12月25日
 *      Author: shixiang
 */

#include <map>
#include "inet/linklayer/ethernet/switch/FattreeAddressTable.h"

namespace inet {

#define MAX_LINE    100

Define_Module(FattreeAddressTable);

std::ostream& operator<<(std::ostream& os, const FattreeAddressTable::AddressEntry& entry)
{
    os << "{netMask=" << entry.netMask << ", port=" << entry.portno << "}";
    return os;
}

FattreeAddressTable::FattreeAddressTable()
{
    addressTable = new AddressTable();
}
void FattreeAddressTable::initialize()
{

    // Option to pre-read in Address Table. To turn it off, set addressTableFile to empty string
    const char *addressTableFile = par("addressTableFile");
    if (addressTableFile && *addressTableFile)
        readAddressTable(addressTableFile);

    AddressTable& addressTable = *this->addressTable;    // magic to hide the '*' from the name of the watch below
    WATCH_MAP(addressTable);
}

/**
 * Function reads from a file stream pointed to by 'fp' and stores characters
 * until the '\n' or EOF character is found, the resultant string is returned.
 * Note that neither '\n' nor EOF character is stored to the resultant string,
 * also note that if on a line containing useful data that EOF occurs, then
 * that line will not be read in, hence must terminate file with unused line.
 */
static char *fgetline(FILE *fp)
{
    // alloc buffer and read a line
    char *line = new char[MAX_LINE];
    if (fgets(line, MAX_LINE, fp) == nullptr) {
        delete[] line;
        return nullptr;
    }

    // chop CR/LF
    line[MAX_LINE - 1] = '\0';
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';

    return line;
}
void FattreeAddressTable::handleMessage(cMessage *)
{
    throw cRuntimeError("This module doesn't process messages");
}

bool FattreeAddressTable::compareAddressWithMask(MACAddress address, MACAddress addressWithMask, int netMask)
{
    int i;
    int numToCompare = netMask/8;

    EV<<"SHI: CompareAddressWithMask(): address="<<address<<" ,addressWithMask = "<<addressWithMask
            <<" ,netMask = "<< netMask << " ,numToCompare = "<<numToCompare<<endl;

    if(netMask == 8)//compare from right to left
    {
        i = MAC_ADDRESS_SIZE -1;
        while(address.getAddressByte(i) == addressWithMask.getAddressByte(i) && i>0)
        {
            EV<<"SHI: i="<<i<<" ,address.getAddressByte(i)==addressWithMask.getAddressByte(i)\n";
            i--;
        }
        EV<<"matched Byte="<<MAC_ADDRESS_SIZE - i - 1<<",  requestedByte = "<<numToCompare<<endl;

        if(MAC_ADDRESS_SIZE - i - 1 < numToCompare)
            return false;
        else
            return true;
    }
    else //compare from left to right
    {
        for(i = 0; i < MAC_ADDRESS_SIZE; i++)
        {
            if(address.getAddressByte(i) != addressWithMask.getAddressByte(i))
                break;
            else
                EV<<"SHI: i="<<i<<" ,address.getAddressByte(i)==addressWithMask.getAddressByte(i)\n";

        }
        EV<<"matched Byte="<<i<<",  requestedByte = "<<numToCompare+2<<endl;
        if(i < numToCompare+2)
            return false;
        else
            return true;

    }
}
/*
 * For a known arriving port,  it generates a vector with the ports where relay component
 * should deliver the message.
 * returns false if not found
 */
int FattreeAddressTable::getPortForAddress(MACAddress& address, unsigned int vid)
{
    Enter_Method("FattreeAddressTable::getPortForAddress()");

    EV<<"SHI: Dest MAC Address str:"<<address<<"\n";

    for (int i = 0; i < MAC_ADDRESS_SIZE; i++)
           EV<<"i="<<i<<", addressByte(i)="<<address.getAddressByte(i)<<"\n";


    if (addressTable == nullptr)
           return -1;

    EV<<"SHI: Address Table: \n";

    for (auto iter = addressTable->rbegin(); iter != addressTable->rend(); iter++ )
    {
        EV<<iter->first<<", "<<iter->second.netMask<<","<<iter->second.portno<<"\n";
        if(iter->second.netMask == 0)
            continue;
        if(compareAddressWithMask(address, iter->first, iter->second.netMask))
            return iter->second.portno;
    }

    return -1;
}

void FattreeAddressTable::readAddressTable(const char *fileName)
{
    FILE *fp = fopen(fileName, "r");
    if (fp == nullptr)
        throw cRuntimeError("cannot open address table file `%s'", fileName);

    //  Syntax of the file goes as:
    //  Switch ID, dest ID, portno
    //  10.pod.switch.1 10.pod.x.x/24 or 10.pod.switch.x/32 0or1
    //  ......
    //  10.pod.switch.1 0.0.0.0/0 0
    //  10.pod.switch.1 0.0.0.2/8 2or3
    //  10.pod.switch.1 0.0.0.3/8 2or3
    //
    //  Each iteration of the loop reads in an entire line i.e. up to '\n' or EOF characters
    //  and uses strtok to extract tokens from the resulting string

    char *line;
    for (int lineno = 0; (line = fgetline(fp)) != nullptr; delete [] line)
    {
         lineno++;

         // lines beginning with '#' are treated as comments
         if (line[0] == '#')
             continue;

         // scan in Switch ID
         char *switchID = strtok(line, " \t");
         // scan in dest ID
         char *destIDandNetMask = strtok(nullptr, " \t");
         // scan in port number
         char *portno = strtok(nullptr, " \t");

         char *destID = strtok(destIDandNetMask, "/");
         char *netMask =  strtok(nullptr, "/");

         // empty line?
         if (!switchID)
             continue;
         // broken line?
         if (!portno || !destID) //* || Switch ID != current Switch ID
             throw cRuntimeError("line %d invalid in address table file `%s'", lineno, fileName);

         // Create an entry with address and portno and insert into table
         AddressEntry entry(atoi(netMask), atoi(portno));
         //* destID to MACaddress
         (*addressTable)[MACAddress(destID)] = entry;

    }
    fclose(fp);
}

/*
 * Register a new MAC address at addressTable.
 * True if refreshed. False if it is new.
 */
bool FattreeAddressTable::updateTableWithAddress(int portno, MACAddress& address, unsigned int vid)
{
   return true;
}

void FattreeAddressTable::flush(int portno)
{

}
void FattreeAddressTable::printState()
{

}
void FattreeAddressTable::copyTable(int portA, int portB)
{

}
void FattreeAddressTable::removeAgedEntriesFromVlan(unsigned int vid)
{

}
void FattreeAddressTable::removeAgedEntriesFromAllVlans()
{

}
void FattreeAddressTable::removeAgedEntriesIfNeeded()
{

}
void FattreeAddressTable::clearTable()
{

}

FattreeAddressTable::~FattreeAddressTable()
{
 //   for (auto & elem : addressTable)
 //       delete elem.second;
}
void FattreeAddressTable::setAgingTime(simtime_t agingTime)
{
   // this->agingTime = agingTime;
}
void FattreeAddressTable::resetDefaultAging()
{
   // agingTime = par("agingTime");
}
}// namespace inet
