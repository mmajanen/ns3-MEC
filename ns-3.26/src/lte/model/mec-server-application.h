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

#ifndef MEC_SERVER_APPLICATION_H
#define MEC_SERVER_APPLICATION_H

#include <ns3/address.h>
#include <ns3/socket.h>
#include <ns3/virtual-net-device.h>
#include <ns3/traced-callback.h>
#include <ns3/callback.h>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <ns3/eps-bearer.h>
#include <ns3/epc-tft.h>
#include <ns3/epc-tft-classifier.h>
#include <ns3/lte-common.h>
#include <ns3/application.h>
#include <ns3/epc-s1ap-sap.h>
#include <ns3/epc-s11-sap.h>
#include <map>

namespace ns3 {

/**
 * \ingroup lte
 *
 * This application implements the MEC server functionality.
 */
class MecServerApplication : public Application
{

public:

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   * Constructor that binds the tap device to the callback methods.
   *
   * \param tunDevice TUN VirtualNetDevice used to tunnel IP packets
   * \param mecSocket socket used to send GTP-U packets to the eNBs
   */
  MecServerApplication (const Ptr<VirtualNetDevice> tunDevice, const Ptr<Socket> mecSocket);

  /** 
   * Destructor
   */
  virtual ~MecServerApplication (void);
  
  /** 
   * Method to be assigned to the callback of the TUN VirtualNetDevice. It
   * is called when the MEC receives a data packet from the
   * internet (including IP headers) that is to be sent to the UE via
   * its associated eNB, tunneling IP over GTP-U/UDP/IP.
   * 
   * \param packet 
   * \param source 
   * \param dest 
   * \param protocolNumber 
   * \return true always 
   */
  bool RecvFromTunDevice (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);


  /** 
   * Method to be assigned to the recv callback of the MEC socket. It
   * is called when the MEC receives a data packet from the eNB
   * that is to be forwarded to the internet. 
   * 
   * \param socket pointer to the MEC socket
   */
  void RecvFromMecSocket (Ptr<Socket> socket);

  /** 
   * Send a packet to the internet 
   * 
   * \param packet 
   */
  void SendToTunDevice (Ptr<Packet> packet, uint32_t teid);


  /** 
   * Send a packet to the eNB
   * 
   * \param packet packet to be sent
   * \param enbS1uAddress the address of the eNB
   * \param teid the Tunnel Endpoint IDentifier
   */
  void SendToMecSocket (Ptr<Packet> packet, Ipv4Address enbS1uAddress, uint32_t teid);

  /**
   * Get the eNB address
   */
  Ipv4Address GetEnbAddr ();

  /**
   * Set the eNB address
   */
  void SetEnbAddr (Ipv4Address addr);

  
private:

 /**
  * UDP socket to send and receive GTP-U packets to and from the MEC interface
  */
  Ptr<Socket> m_mecSocket;
  
  /**
   * TUN VirtualNetDevice used for tunneling/detunneling IP packets
   * from/to the internet over GTP-U/UDP/IP on the MEC interface 
   */
  Ptr<VirtualNetDevice> m_tunDevice;

  /**
   * Map telling for each UE address the corresponding TEID 
   */
  std::map<Ipv4Address, uint32_t> m_teidByAddrMap;

 
  /**
   * UDP port to be used for GTP
   */
  uint16_t m_gtpuUdpPort;

  /**
   * eNB address
   */
  Ipv4Address m_enbAddr;

  
  //uint32_t m_teidCount;

};

} //namespace ns3

#endif /* MEC_SERVER_APPLICATION_H */


