/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <windows.h>

#include <MprApi.h>
#include <ipsectypes.h>
#include <ras.h>
#include <raserror.h>
#include <stdio.h>
#include <winerror.h>
#include <winsock.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "brave/components/brave_vpn/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/utils_win.h"

// Simple Windows VPN configuration tool (using RAS API)
// By Brian Clifton (brian@clifton.me)
//
// RAS API docs
// https://docs.microsoft.com/en-us/windows/win32/rras/remote-access-service-functions
//
// MPR API docs
// https://docs.microsoft.com/en-us/windows/win32/api/mprapi/
//
// NOTES:
// RAS = Remote Access Service
// EAP = Extensible Authentication Protocol
// MPR = Multiprotocol Routing
//

#define DEFAULT_PHONE_BOOK NULL

constexpr char kConnectionsCommand[] = "connections";
constexpr char kDevicesCommand[] = "devices";
constexpr char kEntriesCommand[] = "entries";
constexpr char kCreateCommand[] = "create";
constexpr char kRemoveCommand[] = "remove";
constexpr char kHostName[] = "host_name";
constexpr char kVPNName[] = "vpn_name";
constexpr char kUserName[] = "user_name";
constexpr char kPassword[] = "password";

using brave_vpn::PrintRasError;

int PrintConnectionDetails(HRASCONN connection) {
  DWORD dwCb = 0;
  DWORD dwRet = ERROR_SUCCESS;
  PRAS_PROJECTION_INFO lpProjectionInfo = NULL;

  // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetprojectioninfoex
  dwRet = RasGetProjectionInfoEx(connection, lpProjectionInfo, &dwCb);
  if (dwRet == ERROR_BUFFER_TOO_SMALL) {
    lpProjectionInfo = (PRAS_PROJECTION_INFO)HeapAlloc(GetProcessHeap(),
                                                       HEAP_ZERO_MEMORY, dwCb);
    lpProjectionInfo->version = RASAPIVERSION_CURRENT;
    dwRet = RasGetProjectionInfoEx(connection, lpProjectionInfo, &dwCb);
    if (dwRet != ERROR_SUCCESS) {
      PrintRasError(dwRet);
      if (lpProjectionInfo) {
        HeapFree(GetProcessHeap(), 0, lpProjectionInfo);
        lpProjectionInfo = NULL;
      }
      return dwRet;
    }

    if (lpProjectionInfo->type == PROJECTION_INFO_TYPE_IKEv2) {
      // See _RASIKEV2_PROJECTION_INFO in Ras.h for full list of fields.
      // Fields commented out are not implemented (ex: IPv6).
      wprintf(L"\ttype=PROJECTION_INFO_TYPE_IKEv2");

      // IPv4 Projection Parameters
      wprintf(L"\n\tdwIPv4NegotiationError=%d",
              lpProjectionInfo->ikev2.dwIPv4NegotiationError);
      wprintf(L"\n\tipv4Address=");
      printf("%s", inet_ntoa(lpProjectionInfo->ikev2.ipv4Address));
      wprintf(L"\n\tipv4ServerAddress=");
      printf("%s", inet_ntoa(lpProjectionInfo->ikev2.ipv4ServerAddress));

      // IPv6 Projection Parameters
      // DWORD         dwIPv6NegotiationError;
      // RASIPV6ADDR   ipv6Address;
      // RASIPV6ADDR   ipv6ServerAddress;
      // DWORD         dwPrefixLength;

      // AUTH
      wprintf(L"\n\tdwAuthenticationProtocol=");
      if (lpProjectionInfo->ikev2.dwAuthenticationProtocol ==
          RASIKEv2_AUTH_MACHINECERTIFICATES)
        wprintf(L"RASIKEv2_AUTH_MACHINECERTIFICATES");
      else if (lpProjectionInfo->ikev2.dwAuthenticationProtocol ==
               RASIKEv2_AUTH_EAP)
        wprintf(L"RASIKEv2_AUTH_EAP");
      wprintf(L"\n\tdwEapTypeId=%d", lpProjectionInfo->ikev2.dwEapTypeId);

      // -
      wprintf(L"\n\tdwFlags=");
      if (lpProjectionInfo->ikev2.dwFlags & RASIKEv2_FLAGS_MOBIKESUPPORTED)
        wprintf(L"RASIKEv2_FLAGS_MOBIKESUPPORTED, ");
      if (lpProjectionInfo->ikev2.dwFlags & RASIKEv2_FLAGS_BEHIND_NAT)
        wprintf(L"RASIKEv2_FLAGS_BEHIND_NAT, ");
      if (lpProjectionInfo->ikev2.dwFlags & RASIKEv2_FLAGS_SERVERBEHIND_NAT)
        wprintf(L"RASIKEv2_FLAGS_SERVERBEHIND_NAT");
      wprintf(L"\n\tdwEncryptionMethod=");
      // https://docs.microsoft.com/en-us/windows/win32/api/ipsectypes/ne-ipsectypes-ipsec_cipher_type
      if (lpProjectionInfo->ikev2.dwEncryptionMethod == IPSEC_CIPHER_TYPE_DES)
        wprintf(L"IPSEC_CIPHER_TYPE_DES");
      else if (lpProjectionInfo->ikev2.dwEncryptionMethod ==
               IPSEC_CIPHER_TYPE_3DES)
        wprintf(L"IPSEC_CIPHER_TYPE_3DES");
      else if (lpProjectionInfo->ikev2.dwEncryptionMethod ==
               IPSEC_CIPHER_TYPE_AES_128)
        wprintf(L"IPSEC_CIPHER_TYPE_AES_128");
      else if (lpProjectionInfo->ikev2.dwEncryptionMethod ==
               IPSEC_CIPHER_TYPE_AES_192)
        wprintf(L"IPSEC_CIPHER_TYPE_AES_192");
      else if (lpProjectionInfo->ikev2.dwEncryptionMethod ==
               IPSEC_CIPHER_TYPE_AES_256)
        wprintf(L"IPSEC_CIPHER_TYPE_AES_256");
      else
        wprintf(L"unknown (%d)", lpProjectionInfo->ikev2.dwEncryptionMethod);

      // -
      wprintf(L"\n\tnumIPv4ServerAddresses=%d",
              lpProjectionInfo->ikev2.numIPv4ServerAddresses);
      wprintf(L"\n\tipv4ServerAddresses=");
      for (DWORD j = 0; j < lpProjectionInfo->ikev2.numIPv4ServerAddresses;
           j++) {
        printf("%s", inet_ntoa(lpProjectionInfo->ikev2.ipv4ServerAddresses[j]));
        if ((j + 1) < lpProjectionInfo->ikev2.numIPv4ServerAddresses)
          wprintf(L", ");
      }
      wprintf(L"\n\tnumIPv6ServerAddresses=%d",
              lpProjectionInfo->ikev2.numIPv6ServerAddresses);
    } else if (lpProjectionInfo->type == PROJECTION_INFO_TYPE_PPP) {
      wprintf(L"\ttype=PROJECTION_INFO_TYPE_PPP");
    }

    HeapFree(GetProcessHeap(), 0, lpProjectionInfo);
    lpProjectionInfo = NULL;
  } else {
    wprintf(L"\tError calling RasGetProjectionInfoEx: ");
    PrintRasError(dwRet);
  }

  return dwRet;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumconnectionsa
int PrintConnections() {
  DWORD dwCb = 0;
  DWORD dwRet = ERROR_SUCCESS;
  DWORD dwConnections = 0;
  LPRASCONN lpRasConn = NULL;

  // Call RasEnumConnections with lpRasConn = NULL. dwCb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dwRet = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);
  if (dwRet == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    lpRasConn = (LPRASCONN)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
    if (lpRasConn == NULL) {
      LOG(ERROR) << "HeapAlloc failed!";
      return 0;
    }
    // The first RASCONN structure in the array must contain the RASCONN
    // structure size
    lpRasConn[0].dwSize = sizeof(RASCONN);

    // Call RasEnumConnections to enumerate active connections
    dwRet = RasEnumConnections(lpRasConn, &dwCb, &dwConnections);

    // If successful, print the names of the active connections.
    if (ERROR_SUCCESS == dwRet) {
      wprintf(L"The following RAS connections are currently active:\n");
      for (DWORD i = 0; i < dwConnections; i++) {
        wprintf(L"%s\n", lpRasConn[i].szEntryName);
        PrintConnectionDetails(lpRasConn[i].hrasconn);
      }
    }
    wprintf(L"\n");
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lpRasConn);
    lpRasConn = NULL;
    return 0;
  }

  // There was either a problem with RAS or there are no connections to
  // enumerate
  if (dwConnections >= 1) {
    wprintf(L"The operation failed to acquire the buffer size.\n\n");
  } else {
    wprintf(L"There are no active RAS connections.\n\n");
  }

  return 0;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumdevicesa
int PrintDevices() {
  DWORD dwCb = 0;
  DWORD dwRet = ERROR_SUCCESS;
  DWORD dwDevices = 0;
  LPRASDEVINFO lpRasDevInfo = NULL;

  // Call RasEnumDevices with lpRasDevInfo = NULL. dwCb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

  if (dwRet == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    lpRasDevInfo =
        (LPRASDEVINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
    if (lpRasDevInfo == NULL) {
      wprintf(L"HeapAlloc failed!\n");
      return 0;
    }
    // The first RASDEVINFO structure in the array must contain the structure
    // size
    lpRasDevInfo[0].dwSize = sizeof(RASDEVINFO);

    // Call RasEnumDevices to enumerate RAS devices
    dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

    // If successful, print the names of the RAS devices
    if (ERROR_SUCCESS == dwRet) {
      wprintf(L"The following RAS devices were found:\n");
      for (DWORD i = 0; i < dwDevices; i++) {
        wprintf(L"%s\n", lpRasDevInfo[i].szDeviceName);
      }
    }
    wprintf(L"\n");
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lpRasDevInfo);
    lpRasDevInfo = NULL;
    return 0;
  }

  // There was either a problem with RAS or there are no RAS devices to
  // enumerate
  if (dwDevices >= 1) {
    wprintf(L"The operation failed to acquire the buffer size.\n\n");
  } else {
    wprintf(L"There were no RAS devices found.\n\n");
  }

  return 0;
}

void PrintOptions(DWORD options) {
  wprintf(L"\tdwfOptions = {\n");
  if (options & RASEO_UseCountryAndAreaCodes)
    wprintf(L"\t\tRASEO_UseCountryAndAreaCodes\n");
  if (options & RASEO_SpecificIpAddr)
    wprintf(L"\t\tRASEO_SpecificIpAddr\n");
  if (options & RASEO_SpecificNameServers)
    wprintf(L"\t\tRASEO_SpecificNameServers\n");
  if (options & RASEO_IpHeaderCompression)
    wprintf(L"\t\tRASEO_IpHeaderCompression\n");
  if (options & RASEO_RemoteDefaultGateway)
    wprintf(L"\t\tRASEO_RemoteDefaultGateway\n");
  if (options & RASEO_DisableLcpExtensions)
    wprintf(L"\t\tRASEO_DisableLcpExtensions\n");
  if (options & RASEO_TerminalBeforeDial)
    wprintf(L"\t\tRASEO_TerminalBeforeDial\n");
  if (options & RASEO_TerminalAfterDial)
    wprintf(L"\t\tRASEO_TerminalAfterDial\n");
  if (options & RASEO_ModemLights)
    wprintf(L"\t\tRASEO_ModemLights\n");
  if (options & RASEO_SwCompression)
    wprintf(L"\t\tRASEO_SwCompression\n");
  if (options & RASEO_RequireEncryptedPw)
    wprintf(L"\t\tRASEO_RequireEncryptedPw\n");
  if (options & RASEO_RequireMsEncryptedPw)
    wprintf(L"\t\tRASEO_RequireMsEncryptedPw\n");
  if (options & RASEO_RequireDataEncryption)
    wprintf(L"\t\tRASEO_RequireDataEncryption\n");
  if (options & RASEO_NetworkLogon)
    wprintf(L"\t\tRASEO_NetworkLogon\n");
  if (options & RASEO_UseLogonCredentials)
    wprintf(L"\t\tRASEO_UseLogonCredentials\n");
  if (options & RASEO_PromoteAlternates)
    wprintf(L"\t\tRASEO_PromoteAlternates\n");

#if (WINVER >= 0x401)
  if (options & RASEO_SecureLocalFiles)
    wprintf(L"\t\tRASEO_SecureLocalFiles\n");
#endif

#if (WINVER >= 0x500)
  if (options & RASEO_RequireEAP)
    wprintf(L"\t\tRASEO_RequireEAP\n");
  if (options & RASEO_RequirePAP)
    wprintf(L"\t\tRASEO_RequirePAP\n");
  if (options & RASEO_RequireSPAP)
    wprintf(L"\t\tRASEO_RequireSPAP\n");
  if (options & RASEO_Custom)
    wprintf(L"\t\tRASEO_Custom\n");

  if (options & RASEO_PreviewPhoneNumber)
    wprintf(L"\t\tRASEO_PreviewPhoneNumber\n");
  if (options & RASEO_SharedPhoneNumbers)
    wprintf(L"\t\tRASEO_SharedPhoneNumbers\n");
  if (options & RASEO_PreviewUserPw)
    wprintf(L"\t\tRASEO_PreviewUserPw\n");
  if (options & RASEO_PreviewDomain)
    wprintf(L"\t\tRASEO_PreviewDomain\n");
  if (options & RASEO_ShowDialingProgress)
    wprintf(L"\t\tRASEO_ShowDialingProgress\n");
  if (options & RASEO_RequireCHAP)
    wprintf(L"\t\tRASEO_RequireCHAP\n");
  if (options & RASEO_RequireMsCHAP)
    wprintf(L"\t\tRASEO_RequireMsCHAP\n");
  if (options & RASEO_RequireMsCHAP2)
    wprintf(L"\t\tRASEO_RequireMsCHAP2\n");
  if (options & RASEO_RequireW95MSCHAP)
    wprintf(L"\t\tRASEO_RequireW95MSCHAP\n");
  if (options & RASEO_CustomScript)
    wprintf(L"\t\tRASEO_CustomScript\n");
#endif

  wprintf(L"\t};\n");
}

void PrintOptions2(DWORD options) {
  wprintf(L"\tdwfOptions2 = {\n");

#if (WINVER >= 0x501)
  if (options & RASEO2_SecureFileAndPrint)
    wprintf(L"\t\tRASEO2_SecureFileAndPrint\n");
  if (options & RASEO2_SecureClientForMSNet)
    wprintf(L"\t\tRASEO2_SecureClientForMSNet\n");
  if (options & RASEO2_DontNegotiateMultilink)
    wprintf(L"\t\tRASEO2_DontNegotiateMultilink\n");
  if (options & RASEO2_DontUseRasCredentials)
    wprintf(L"\t\tRASEO2_DontUseRasCredentials\n");
  if (options & RASEO2_UsePreSharedKey)
    wprintf(L"\t\tRASEO2_UsePreSharedKey\n");
  if (options & RASEO2_Internet)
    wprintf(L"\t\tRASEO2_Internet\n");
  if (options & RASEO2_DisableNbtOverIP)
    wprintf(L"\t\tRASEO2_DisableNbtOverIP\n");
  if (options & RASEO2_UseGlobalDeviceSettings)
    wprintf(L"\t\tRASEO2_UseGlobalDeviceSettings\n");
  if (options & RASEO2_ReconnectIfDropped)
    wprintf(L"\t\tRASEO2_ReconnectIfDropped\n");
  if (options & RASEO2_SharePhoneNumbers)
    wprintf(L"\t\tRASEO2_SharePhoneNumbers\n");
#endif

#if (WINVER >= 0x600)
  if (options & RASEO2_SecureRoutingCompartment)
    wprintf(L"\t\tRASEO2_SecureRoutingCompartment\n");
  if (options & RASEO2_UseTypicalSettings)
    wprintf(L"\t\tRASEO2_UseTypicalSettings\n");
  if (options & RASEO2_IPv6SpecificNameServers)
    wprintf(L"\t\tRASEO2_IPv6SpecificNameServers\n");
  if (options & RASEO2_IPv6RemoteDefaultGateway)
    wprintf(L"\t\tRASEO2_IPv6RemoteDefaultGateway\n");
  if (options & RASEO2_RegisterIpWithDNS)
    wprintf(L"\t\tRASEO2_RegisterIpWithDNS\n");
  if (options & RASEO2_UseDNSSuffixForRegistration)
    wprintf(L"\t\tRASEO2_UseDNSSuffixForRegistration\n");
  if (options & RASEO2_IPv4ExplicitMetric)
    wprintf(L"\t\tRASEO2_IPv4ExplicitMetric\n");
  if (options & RASEO2_IPv6ExplicitMetric)
    wprintf(L"\t\tRASEO2_IPv6ExplicitMetric\n");
  if (options & RASEO2_DisableIKENameEkuCheck)
    wprintf(L"\t\tRASEO2_DisableIKENameEkuCheck\n");
#endif

#if (WINVER >= 0x601)
  if (options & RASEO2_DisableClassBasedStaticRoute)
    wprintf(L"\t\tRASEO2_DisableClassBasedStaticRoute\n");
  if (options & RASEO2_SpecificIPv6Addr)
    wprintf(L"\t\tRASEO2_SpecificIPv6Addr\n");
  if (options & RASEO2_DisableMobility)
    wprintf(L"\t\tRASEO2_DisableMobility\n");
  if (options & RASEO2_RequireMachineCertificates)
    wprintf(L"\t\tRASEO2_RequireMachineCertificates\n");
#endif

#if (WINVER >= 0x602)
  if (options & RASEO2_UsePreSharedKeyForIkev2Initiator)
    wprintf(L"\t\tRASEO2_UsePreSharedKeyForIkev2Initiator\n");
  if (options & RASEO2_UsePreSharedKeyForIkev2Responder)
    wprintf(L"\t\tRASEO2_UsePreSharedKeyForIkev2Responder\n");
  if (options & RASEO2_CacheCredentials)
    wprintf(L"\t\tRASEO2_CacheCredentials\n");
#endif

#if (WINVER >= 0x603)
  if (options & RASEO2_AutoTriggerCapable)
    wprintf(L"\t\tRASEO2_AutoTriggerCapable\n");
  if (options & RASEO2_IsThirdPartyProfile)
    wprintf(L"\t\tRASEO2_IsThirdPartyProfile\n");
  if (options & RASEO2_AuthTypeIsOtp)
    wprintf(L"\t\tRASEO2_AuthTypeIsOtp\n");
#endif

#if (WINVER >= 0x604)
  if (options & RASEO2_IsAlwaysOn)
    wprintf(L"\t\tRASEO2_IsAlwaysOn\n");
  if (options & RASEO2_IsPrivateNetwork)
    wprintf(L"\t\tRASEO2_IsPrivateNetwork\n");
#endif

#if (WINVER >= 0xA00)
  if (options & RASEO2_PlumbIKEv2TSAsRoutes)
    wprintf(L"\t\tRASEO2_PlumbIKEv2TSAsRoutes\n");
#endif

  wprintf(L"\t};");
}

void PrintBytes(LPCWSTR name, LPBYTE bytes, DWORD len) {
  bool next_is_newline = false;
  const int bytes_per_line = 12;
  wprintf(L"\n\t[%s: %d bytes]\n\t\t", name, len);
  for (DWORD i = 0; i < len; i++) {
    if (i > 0 && !next_is_newline) {
      wprintf(L", ");
    }
    wprintf(L"0x%02x", bytes[i]);
    next_is_newline = ((i + 1) % bytes_per_line) == 0;
    if (next_is_newline) {
      wprintf(L"\n\t\t");
    }
  }
  wprintf(L"\n\t[/%s]", name);
}

int PrintEntryDetails(LPCTSTR entry_name) {
  DWORD dwCb = 0;
  DWORD dwRet = ERROR_SUCCESS;
  LPRASENTRY lpRasEntry = NULL;

  // Call RasGetEntryProperties with lpRasEntry = NULL. dwCb is returned with
  // the required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dwRet = RasGetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, lpRasEntry,
                                &dwCb, NULL, NULL);
  if (dwRet == ERROR_BUFFER_TOO_SMALL) {
    lpRasEntry =
        (LPRASENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
    if (lpRasEntry == NULL) {
      wprintf(L"HeapAlloc failed!\n");
      return 0;
    }

    // The first LPRASENTRY structure in the array must contain the structure
    // size
    lpRasEntry[0].dwSize = sizeof(RASENTRY);
    dwRet = RasGetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, lpRasEntry,
                                  &dwCb, NULL, NULL);
    switch (dwRet) {
      case ERROR_INVALID_SIZE:
        wprintf(L"An incorrect structure size was detected.\n");
        break;
    }

    // great place to set debug breakpoint when inspecting existing connections
    PrintOptions(lpRasEntry->dwfOptions);
    PrintOptions2(lpRasEntry->dwfOptions2);

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetcustomauthdataa
    LPBYTE custom_auth_data = NULL;
    dwRet = RasGetCustomAuthData(DEFAULT_PHONE_BOOK, entry_name,
                                 custom_auth_data, &dwCb);
    if (dwRet == ERROR_BUFFER_TOO_SMALL && dwCb > 0) {
      custom_auth_data =
          (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
      dwRet = RasGetCustomAuthData(DEFAULT_PHONE_BOOK, entry_name,
                                   custom_auth_data, &dwCb);
      if (dwRet != ERROR_SUCCESS) {
        PrintRasError(dwRet);
        if (custom_auth_data) {
          HeapFree(GetProcessHeap(), 0, custom_auth_data);
          custom_auth_data = NULL;
        }
        return dwRet;
      }
      PrintBytes(L"CustomAuthData", custom_auth_data, dwCb);
      HeapFree(GetProcessHeap(), 0, custom_auth_data);
    } else if (dwCb > 0) {
      wprintf(L"\n\tError calling RasGetCustomAuthData: ");
      PrintRasError(dwRet);
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgeteapuserdataa
    LPBYTE eap_user_data = NULL;
    dwRet = RasGetEapUserData(NULL, DEFAULT_PHONE_BOOK, entry_name,
                              eap_user_data, &dwCb);
    if (dwRet == ERROR_BUFFER_TOO_SMALL && dwCb > 0) {
      eap_user_data =
          (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
      dwRet = RasGetEapUserData(NULL, DEFAULT_PHONE_BOOK, entry_name,
                                eap_user_data, &dwCb);
      if (dwRet != ERROR_SUCCESS) {
        PrintRasError(dwRet);
        if (eap_user_data) {
          HeapFree(GetProcessHeap(), 0, eap_user_data);
          eap_user_data = NULL;
        }
        return dwRet;
      }
      PrintBytes(L"EapUserData", eap_user_data, dwCb);
      HeapFree(GetProcessHeap(), 0, eap_user_data);
    } else if (dwCb > 0) {
      wprintf(L"\n\tError calling RasGetEapUserData: ");
      PrintRasError(dwRet);
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetsubentrypropertiesa
    wprintf(L"\n\tdwSubEntries: %d", lpRasEntry->dwSubEntries);
    if (lpRasEntry->dwSubEntries > 0) {
      for (DWORD i = 0; i < lpRasEntry->dwSubEntries; i++) {
        LPRASSUBENTRY lpRasSubEntry = NULL;
        dwRet = RasGetSubEntryProperties(DEFAULT_PHONE_BOOK, entry_name, i + 1,
                                         lpRasSubEntry, &dwCb, NULL, NULL);
        if (dwRet == ERROR_BUFFER_TOO_SMALL && dwCb > 0) {
          lpRasSubEntry = (LPRASSUBENTRY)HeapAlloc(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY, dwCb);
          dwRet =
              RasGetSubEntryProperties(DEFAULT_PHONE_BOOK, entry_name, i + 1,
                                       lpRasSubEntry, &dwCb, NULL, NULL);
          if (dwRet != ERROR_SUCCESS) {
            PrintRasError(dwRet);
            if (lpRasSubEntry) {
              HeapFree(GetProcessHeap(), 0, lpRasSubEntry);
              lpRasSubEntry = NULL;
            }
            return dwRet;
          }
          wprintf(L"\n\t\tdwSize=%d", lpRasSubEntry->dwSize);
          wprintf(L"\n\t\tdwfFlags=%d", lpRasSubEntry->dwfFlags);
          wprintf(L"\n\t\tszDeviceType=%s", lpRasSubEntry->szDeviceType);
          wprintf(L"\n\t\tszDeviceName=%s", lpRasSubEntry->szDeviceName);
          wprintf(L"\n\t\tszLocalPhoneNumber=%s",
                  lpRasSubEntry->szLocalPhoneNumber);
          wprintf(L"\n\t\tdwAlternateOffset=%d",
                  lpRasSubEntry->dwAlternateOffset);
          HeapFree(GetProcessHeap(), 0, lpRasSubEntry);
          lpRasSubEntry = NULL;
        } else {
          wprintf(L"\n\tError calling RasGetSubEntryProperties: ");
          PrintRasError(dwRet);
        }
      }
    }

    wprintf(L"\n");
    // Deallocate memory for the entry buffer
    HeapFree(GetProcessHeap(), 0, lpRasEntry);
    lpRasEntry = NULL;
    return ERROR_SUCCESS;
  }

  return dwRet;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumentriesa
int PrintEntries() {
  DWORD dwCb = 0;
  DWORD dwRet = ERROR_SUCCESS;
  DWORD dwEntries = 0;
  LPRASENTRYNAME lpRasEntryName = NULL;

  // Call RasEnumEntries with lpRasEntryName = NULL. dwCb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dwRet = RasEnumEntries(NULL, NULL, lpRasEntryName, &dwCb, &dwEntries);
  if (dwRet == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS entry names.
    lpRasEntryName =
        (LPRASENTRYNAME)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
    if (lpRasEntryName == NULL) {
      LOG(ERROR) << "HeapAlloc failed!";
      return 0;
    }
    // The first RASENTRYNAME structure in the array must contain the structure
    // size
    lpRasEntryName[0].dwSize = sizeof(RASENTRYNAME);

    // Call RasEnumEntries to enumerate all RAS entry names
    dwRet = RasEnumEntries(NULL, NULL, lpRasEntryName, &dwCb, &dwEntries);

    // If successful, print the RAS entry names
    if (ERROR_SUCCESS == dwRet) {
      wprintf(L"The following RAS entry names were found:\n");
      for (DWORD i = 0; i < dwEntries; i++) {
        wprintf(L"%s\n", lpRasEntryName[i].szEntryName);
        dwRet = PrintEntryDetails(lpRasEntryName[i].szEntryName);
      }
    }
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lpRasEntryName);
    lpRasEntryName = NULL;
    return ERROR_SUCCESS;
  }

  // There was either a problem with RAS or there are RAS entry names to
  // enumerate
  if (dwEntries >= 1) {
    wprintf(L"The operation failed to acquire the buffer size.\n\n");
  } else {
    wprintf(L"There were no RAS entry names found:.\n\n");
  }

  return dwRet;
}

void copy_dword_bytes(BYTE* bytes, DWORD value) {
  if (bytes) {
    union {
      DWORD value;
      BYTE bytes[4];
    } converter;
    converter.value = value;
    memcpy(bytes, converter.bytes, 4);
  }
}

void Demo() {
  // These are the values set
  ROUTER_CUSTOM_IKEv2_POLICY0 policy;
  policy.dwIntegrityMethod = IKEEXT_INTEGRITY_SHA_256;
  policy.dwEncryptionMethod = IKEEXT_CIPHER_AES_GCM_256_16ICV;
  policy.dwCipherTransformConstant = IKEEXT_CIPHER_AES_GCM_256_16ICV;
  policy.dwAuthTransformConstant = IPSEC_CIPHER_CONFIG_GCM_AES_256;
  policy.dwPfsGroup = IPSEC_PFS_NONE;
  policy.dwDhGroup = IKEEXT_DH_ECP_384;

  // This is the byte order they are in `CustomIPSecPolicies` field
  // inside `%APPDATA%\Microsoft\Network\Connections\Pbk\rasphone.pbk`
  BYTE custom_ipsec_policies[24] = {0};
  copy_dword_bytes(&custom_ipsec_policies[0], policy.dwIntegrityMethod);
  copy_dword_bytes(&custom_ipsec_policies[4], policy.dwEncryptionMethod);
  copy_dword_bytes(&custom_ipsec_policies[8], policy.dwCipherTransformConstant);
  copy_dword_bytes(&custom_ipsec_policies[12], policy.dwAuthTransformConstant);
  copy_dword_bytes(&custom_ipsec_policies[16], policy.dwDhGroup);
  copy_dword_bytes(&custom_ipsec_policies[20], policy.dwPfsGroup);

  // characters are either written in 02d or 02x format
  wprintf(L"\nDEMO:\n");
  for (DWORD i = 0; i < 24; i++) {
    wprintf(L"%02d", custom_ipsec_policies[i]);
  }
  wprintf(L"\n");
}

// Test program for create/remove Windows VPN entry.
int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  auto* vpn_manager = brave_vpn::BraveVPNConnectionManager::GetInstance();

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kConnectionsCommand)) {
    PrintConnections();
    return 0;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kDevicesCommand)) {
    PrintDevices();
    return 0;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kEntriesCommand)) {
    PrintEntries();
    return 0;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kRemoveCommand)) {
    const std::string vpn_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kVPNName);
    if (vpn_name.empty()) {
      LOG(ERROR) << "missing parameters for remove!";
      LOG(ERROR) << "usage: winvpntool.exe --remove --vpn_name=entry_name";
      return 0;
    }
    vpn_manager->RemoveVPNConnection(vpn_name);
    return 0;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kCreateCommand)) {
    const std::string host_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kHostName);
    const std::string vpn_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kVPNName);
    const std::string user_name =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kUserName);
    const std::string password =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kPassword);
    if (host_name.empty() || vpn_name.empty() || user_name.empty() ||
        password.empty()) {
      LOG(ERROR) << "missing parameters for create!";
      LOG(ERROR) << "usage: winvpntool.exe --create --host_name=xxx "
                    "--vpn_name=xxx --user_name=xxx --passowrd=xxx";
      return 0;
    }
    brave_vpn::BraveVPNConnectionInfo info;
    info.url = host_name;
    info.name = vpn_name;
    info.id = user_name;
    info.pwd = password;
    vpn_manager->CreateVPNConnection(info);
    return 0;
  }

  return 0;
}
