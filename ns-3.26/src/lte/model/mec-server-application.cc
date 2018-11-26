/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *
 * Modications for MEC purposes by
 * Mikko Majanen <mikko.majanen@vtt.fi>
 * Copyright (c) 2018 VTT Technical Research Centre of Finland Ltd.
 */


#include <ns3/mec-server-application.h>
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/epc-gtpu-header.h"
#include "ns3/abort.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MecServerApplication");

/////////////////////////
// MecServerApplication
/////////////////////////


TypeId
MecServerApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MecServerApplication")
    .SetParent<Object> ()
    .SetGroupName("Lte");
  return tid;
}

void
MecServerApplication::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_mecSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_mecSocket = 0;
}

  

MecServerApplication::MecServerApplication (const Ptr<VirtualNetDevice> tunDevice, const Ptr<Socket> mecSocket)
  : m_mecSocket (mecSocket),
    m_tunDevice (tunDevice),
    m_gtpuUdpPort (2152)// use fixed port number of 2152
{
  NS_LOG_FUNCTION (this << tunDevice << mecSocket);
  m_mecSocket->SetRecvCallback (MakeCallback (&MecServerApplication::RecvFromMecSocket, this));
}

  
MecServerApplication::~MecServerApplication ()
{
  NS_LOG_FUNCTION (this);
}


bool
MecServerApplication::RecvFromTunDevice (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << source << dest << packet << packet->GetSize ());

  // get IP address of UE
  Ptr<Packet> pCopy = packet->Copy ();
  Ipv4Header ipv4Header;
  pCopy->RemoveHeader (ipv4Header);
  Ipv4Address ueAddr =  ipv4Header.GetDestination ();
  NS_LOG_LOGIC ("packet addressed to UE " << ueAddr);
 
  // find corresponding teid to this UE addr
  std::map<Ipv4Address, uint32_t>::iterator it = m_teidByAddrMap.find (ueAddr);
  if (it == m_teidByAddrMap.end ())
    {        
      NS_LOG_WARN ("unknown UE address " << ueAddr);
    }
  else
    {      
      uint32_t teid = it->second;  
      if (teid == 0)
        {
          NS_LOG_WARN ("no matching bearer for this packet");                   
        }
      else
        {
          NS_LOG_LOGIC("found teid=" << teid);
          SendToMecSocket (packet, GetEnbAddr(), teid);
        }
    }
  
  // there is no reason why we should notify the TUN
  // VirtualNetDevice that he failed to send the packet: if we receive
  // any bogus packet, it will just be silently discarded.
  const bool succeeded = true;
  return succeeded;
}

void 
MecServerApplication::RecvFromMecSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);  
  NS_ASSERT (socket == m_mecSocket);
  Ptr<Packet> packet = socket->Recv ();
  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();

  NS_LOG_LOGIC("RecvFromMecSocket, teid=" << teid);

  //Get the UE address and save (addr, teid) to m_teidByAddrInfo map
  Ptr<Packet> pCopy = packet->Copy();
  Ipv4Header ipv4Header;
  pCopy->RemoveHeader (ipv4Header);
  Ipv4Address ueAddr =  ipv4Header.GetSource ();
  NS_LOG_LOGIC ("packet received from UE " << ueAddr);
  m_teidByAddrMap[ueAddr] = teid;
  
  SendToTunDevice (packet, teid);
}

void 
MecServerApplication::SendToTunDevice (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);
  NS_LOG_LOGIC (" packet size: " << packet->GetSize () << " bytes");
  m_tunDevice->Receive (packet, 0x0800, m_tunDevice->GetAddress (), m_tunDevice->GetAddress (), NetDevice::PACKET_HOST); //0x0800 is IPv4
}

void 
MecServerApplication::SendToMecSocket (Ptr<Packet> packet, Ipv4Address enbAddr, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << enbAddr << teid);

  NS_LOG_LOGIC("SendToMecSocket(): eNB addr, teid:" <<enbAddr<<", "<<teid);
  NS_LOG_LOGIC("m_gtpuUdpPort=" << m_gtpuUdpPort);
  
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);  
  packet->AddHeader (gtpu);
  uint32_t flags = 0;
  m_mecSocket->SendTo (packet, flags, InetSocketAddress (enbAddr, m_gtpuUdpPort));
}

Ipv4Address
MecServerApplication::GetEnbAddr () {
  return m_enbAddr;
}
  
void
MecServerApplication::SetEnbAddr(Ipv4Address addr){
  m_enbAddr = addr;
}

}  // namespace ns3
