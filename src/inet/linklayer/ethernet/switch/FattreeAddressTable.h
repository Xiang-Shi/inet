/*
 * FattreeAddressTable.h
 *
 *  Created on: 2017年12月25日
 *      Author: shixiang
 */

#ifndef FATTREEADDRESSTABLE_H_
#define FATTREEADDRESSTABLE_H_

#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ethernet/switch/IMACAddressTable.h"

namespace inet {
/*
 *  SHI:
 *  All table is assigned staticly, do not update table,
 *  override read file readAddressTable()
 *  override find port getPortForAddress()
 */
class FattreeAddressTable: public cSimpleModule, public IMACAddressTable
{
  protected:

    struct AddressEntry
    {
        int netMask=0;
        int portno =-1;

        AddressEntry() {}
        AddressEntry(int netMask, int portno) :
            netMask(netMask), portno(portno) {}
    };
    friend std::ostream& operator<<(std::ostream& os, const AddressEntry& entry);

    struct MAC_compare
     {
        //disable sort
         bool operator()(const MACAddress& u1, const MACAddress& u2) const { return true; }
     };

    typedef std::map<MACAddress, AddressEntry, MAC_compare> AddressTable;


    AddressTable *addressTable = nullptr;

  protected:

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  public:
    // Table management
    FattreeAddressTable();
    ~FattreeAddressTable();

    bool compareAddressWithMask(MACAddress address, MACAddress addressWithMask, int netMask);

    /**
     * @brief For a known arriving port, V-TAG and destination MAC. It finds out the port where relay component should deliver the message
     * @param address MAC destination
     * @param vid VLAN ID
     * @return Output port for address, or -1 if unknown.
     */
    virtual int getPortForAddress(MACAddress& address, unsigned int vid = 0) override;
    /**
     * Pre-reads in entries for Address Table during initialization.
     */
    virtual void readAddressTable(const char *fileName) override;

    /*
     * All table is assigned staticly at initial, do not update
     */
    virtual bool updateTableWithAddress(int portno, MACAddress& address, unsigned int vid = 0) override;

    /**
     *  @brief Clears portno cache
     */
    // TODO: find a better name
    virtual void flush(int portno) override;

    /**
      *  @brief Prints cached data
      */
     virtual void printState() override;

     /**
      * @brief Copy cache from portA to portB port
      */
     virtual void copyTable(int portA, int portB) override;

     /**
      * @brief Remove aged entries from a specified VLAN
      */
     virtual void removeAgedEntriesFromVlan(unsigned int vid = 0) override;
     /**
      * @brief Remove aged entries from all VLANs
      */
     virtual void removeAgedEntriesFromAllVlans() override;

     /*
      * It calls removeAgedEntriesFromAllVlans() if and only if at least
      * 1 second has passed since the method was last called.
      */
     virtual void removeAgedEntriesIfNeeded() override;

     /**
      * For lifecycle: clears all entries from the vlanAddressTable.
      */
     virtual void clearTable() override;

     /*
      * Some (eg.: STP, RSTP) protocols may need to change agingTime
      */
     virtual void setAgingTime(simtime_t agingTime) override;
     virtual void resetDefaultAging() override;
};

} //namespace inet

#endif /* FATTREEADDRESSTABLE_H_ */
