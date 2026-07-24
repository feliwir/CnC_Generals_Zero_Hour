/*
**	Command & Conquer Generals(tm)
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

#include "Common/Thing.h"

#include "GameNetwork/IPEnumeration.h"
#include "GameNetwork/NetworkUtil.h"

IPEnumeration::IPEnumeration( void )
{
	m_isWinsockInitialized = false;
	m_IPlist = NULL;
}

IPEnumeration::~IPEnumeration( void )
{
	EnumeratedIP *ip = m_IPlist;
	while (ip)
	{
		ip = ip->getNext();
		m_IPlist->deleteInstance();
		m_IPlist = ip;
	}
}

EnumeratedIP * IPEnumeration::getAddresses( void )
{
	if (m_IPlist)
		return m_IPlist;

	// construct a list of addresses
	int numAddresses = 0;
	NET_Address** addresses = NET_GetLocalAddresses(&numAddresses);
	for(int i = 0; i < numAddresses; i++)
	{
		NET_Address* addr = addresses[i];

		const char* addrString = NET_GetAddressString(addr);
		if (addrString == NULL)
		{
			continue;
		}

		// Only get IP4 addresses, since that's all we support right now. (CBD)
		// IPv6 addresses contain ':'; NetAddressToIP() also returns 0 for them.
		UnsignedInt addrIp = NetAddressToIP(addr);
		if (addrIp == 0 || strchr(addrString, ':') != NULL)
		{
			DEBUG_LOG(("Skipping non-IP4 address %s\n", addrString));
			continue;
		}

		// Skip addresses that can never be used to reach a LAN/Internet peer:
		// loopback (127.0.0.0/8) and link-local/APIPA (169.254.0.0/16). The old
		// gethostbyname() path never returned these, but SDL's NET_GetLocalAddresses()
		// enumerates every interface, and auto-selecting one of them as the local
		// address (the list head, see OptionsMenu) breaks network play.
		UnsignedInt firstOctet  = (addrIp >> 24) & 0xff;
		UnsignedInt secondOctet = (addrIp >> 16) & 0xff;
		if (firstOctet == 127 || (firstOctet == 169 && secondOctet == 254))
		{
			DEBUG_LOG(("Skipping non-routable address %s\n", addrString));
			continue;
		}

		EnumeratedIP *newIP = newInstance(EnumeratedIP);
		AsciiString str = AsciiString(addrString);
		DEBUG_LOG(("IP: %s\n", str.str()));
		newIP->setIPstring(str);
		newIP->setIP(addrIp);

		// Insert into the list in ascending IP order so the auto-selected local
		// address (the list head) is deterministic regardless of the order in
		// which the OS happens to enumerate interfaces. (The previous code simply
		// prepended, making the default local IP depend on enumeration order.)
		if (!m_IPlist)
		{
			m_IPlist = newIP;
			newIP->setNext(NULL);
		}
		else if (newIP->getIP() < m_IPlist->getIP())
		{
			newIP->setNext(m_IPlist);
			m_IPlist = newIP;
		}
		else
		{
			EnumeratedIP *p = m_IPlist;
			while (p->getNext() && p->getNext()->getIP() < newIP->getIP())
			{
				p = p->getNext();
			}
			newIP->setNext(p->getNext());
			p->setNext(newIP);
		}
	}

	NET_FreeLocalAddresses(addresses);

	return m_IPlist;
}

AsciiString IPEnumeration::getMachineName( void )
{
	if (!m_isWinsockInitialized)
	{
#ifdef _WIN32
		WORD verReq = MAKEWORD(2, 2);
		WSADATA wsadata;

		int err = WSAStartup(verReq, &wsadata);
		if (err != 0) {
			return NULL;
		}

		if ((LOBYTE(wsadata.wVersion) != 2) || (HIBYTE(wsadata.wVersion) !=2)) {
			WSACleanup();
			return NULL;
		}
#endif
		m_isWinsockInitialized = true;
	}

	// get the local machine's host name
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)))
	{
		DEBUG_LOG(("Failed call to gethostname; WSAGetLastError returned %d\n", WSAGetLastError()));
		return NULL;
	}

	return AsciiString(hostname);
}


