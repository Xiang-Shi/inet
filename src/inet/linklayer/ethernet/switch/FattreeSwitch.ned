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

package inet.linklayer.ethernet.switch;

import inet.common.lifecycle.NodeStatus;
import inet.linklayer.contract.IMACAddressTable;
import inet.linklayer.contract.IMACRelayUnit;
import inet.linklayer.contract.ISpanningTree;
import inet.linklayer.contract.IWiredNic;
import inet.linklayer.configurator.L2NodeConfigurator;
import inet.networklayer.common.InterfaceTable;


//
// TODO auto-generated type
//
module FattreeSwitch
{
   parameters:
        @FattreeSwitch;
        @networkNode();
        @labels(node,ethernet-node);
        @display("i=device/switch");
        bool hasStatus = default(false);
        bool csmacdSupport = default(true);  // by default use CSMA/CD
        string macType = default(csmacdSupport ? "EtherMAC" : "EtherMACFullDuplex"); // ~EtherMAC or ~EtherMACFullDuplex
        string spanningTreeProtocol = default("");

/////////////////////////////////origin start///////////////////////////////////////

//string relayUnitType = default(firstAvailable("Ieee8021dRelay","MACRelayUnit")); // type of the ~IMACRelayUnit;
/////////////////////////////////origin end//////////////////////////////////////

        /////////////////////////////////modified start//////////////////////////////////////        
        string relayUnitType = default("MACRelayUnit");
        /////////////////////////////////modified end//////////////////////////////////////
        
        string macTableType = default("FattreeAddressTable"); // type of the ~IMACAddressTable
        eth[*].encapType = "EtherEncapDummy";
        eth[*].csmacdSupport = csmacdSupport;
        eth[*].macType = macType;
        *.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        inout ethg[] @labels(EtherFrame-conn);
    submodules:
        status: NodeStatus if hasStatus {
            @display("p=73,142;is=s");
        }
        l2NodeConfigurator: L2NodeConfigurator if spanningTreeProtocol != "" {
            @display("p=73,208");
        }
        interfaceTable: InterfaceTable {
            @display("p=73,65;is=s");
        }
        macTable: <macTableType> like IMACAddressTable {
            @display("p=73,19;is=s");
        }
        relayUnit: <relayUnitType> like IMACRelayUnit {
            parameters:
                @display("p=263,50;is=m");
            gates:
                ifIn[sizeof(ethg)];
                ifOut[sizeof(ethg)];
        }
        stp: <spanningTreeProtocol> like ISpanningTree if spanningTreeProtocol != "" {
            @display("p=168,50");
        }
        eth[sizeof(ethg)]: <default("EthernetInterface")> like IWiredNic {
            parameters:
                mac.promiscuous = true;
                @display("p=263,150,row;q=txQueue");
        }
    connections:
        for i=0..sizeof(ethg)-1 {
            eth[i].upperLayerIn <-- relayUnit.ifOut++;
            eth[i].upperLayerOut --> relayUnit.ifIn++;
            eth[i].phys <--> ethg[i];
        }

        if spanningTreeProtocol != "" {
            stp.relayIn <-- relayUnit.stpOut;
            stp.relayOut --> relayUnit.stpIn;
        }    
}
