/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"

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
constexpr char kCheckConnectionCommand[] = "check-connection";
constexpr char kDevicesCommand[] = "devices";
constexpr char kEntriesCommand[] = "entries";
constexpr char kCreateCommand[] = "create";
constexpr char kRemoveCommand[] = "remove";
constexpr char kConnectCommand[] = "connect";
constexpr char kDisconnectCommand[] = "disconnect";
constexpr char kHostName[] = "host_name";
constexpr char kVPNName[] = "vpn_name";
constexpr char kUserName[] = "user_name";
constexpr char kPassword[] = "password";

using brave_vpn::ras::CheckConnection;
using brave_vpn::ras::CheckConnectionResult;
using brave_vpn::ras::ConnectEntry;
using brave_vpn::ras::CreateEntry;
using brave_vpn::ras::DisconnectEntry;
using brave_vpn::ras::GetPhonebookPath;
using brave_vpn::ras::GetRasErrorMessage;
using brave_vpn::ras::RemoveEntry;

void PrintRasError(DWORD error) {
  LOG(ERROR) << GetRasErrorMessage(error);
}

int PrintConnectionDetails(HRASCONN connection) {
  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  PRAS_PROJECTION_INFO lp_projection_info = NULL;

  // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetprojectioninfoex
  dw_ret = RasGetProjectionInfoEx(connection, lp_projection_info, &dw_cb);
  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    lp_projection_info = (PRAS_PROJECTION_INFO)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
    lp_projection_info->version = RASAPIVERSION_CURRENT;
    dw_ret = RasGetProjectionInfoEx(connection, lp_projection_info, &dw_cb);
    if (dw_ret != ERROR_SUCCESS) {
      PrintRasError(dw_ret);
      if (lp_projection_info) {
        HeapFree(GetProcessHeap(), 0, lp_projection_info);
        lp_projection_info = NULL;
      }
      return dw_ret;
    }

    if (lp_projection_info->type == PROJECTION_INFO_TYPE_IKEv2) {
      // See _RASIKEV2_PROJECTION_INFO in Ras.h for full list of fields.
      // Fields commented out are not implemented (ex: IPv6).
      VLOG(2) << "\ttype=PROJECTION_INFO_TYPE_IKEv2";

      // IPv4 Projection Parameters
      VLOG(2) << "\n\tdwIPv4NegotiationError="
              << lp_projection_info->ikev2.dwIPv4NegotiationError;
      VLOG(2) << "\n\tipv4Address=";
      VLOG(2) << inet_ntoa(lp_projection_info->ikev2.ipv4Address);
      VLOG(2) << "\n\tipv4ServerAddress=";
      VLOG(2) << inet_ntoa(lp_projection_info->ikev2.ipv4ServerAddress);

      // AUTH
      VLOG(2) << "\n\tdwAuthenticationProtocol=";
      if (lp_projection_info->ikev2.dwAuthenticationProtocol ==
          RASIKEv2_AUTH_MACHINECERTIFICATES) {
        VLOG(2) << "RASIKEv2_AUTH_MACHINECERTIFICATES";
      } else if (lp_projection_info->ikev2.dwAuthenticationProtocol ==
                 RASIKEv2_AUTH_EAP) {
        VLOG(2) << "RASIKEv2_AUTH_EAP";
      }
      VLOG(2) << "\n\tdwEapTypeId=" << lp_projection_info->ikev2.dwEapTypeId;

      VLOG(2) << "\n\tdwFlags=";
      if (lp_projection_info->ikev2.dwFlags & RASIKEv2_FLAGS_MOBIKESUPPORTED) {
        VLOG(2) << "RASIKEv2_FLAGS_MOBIKESUPPORTED, ";
      }
      if (lp_projection_info->ikev2.dwFlags & RASIKEv2_FLAGS_BEHIND_NAT) {
        VLOG(2) << "RASIKEv2_FLAGS_BEHIND_NAT, ";
      }
      if (lp_projection_info->ikev2.dwFlags & RASIKEv2_FLAGS_SERVERBEHIND_NAT) {
        VLOG(2) << "RASIKEv2_FLAGS_SERVERBEHIND_NAT";
      }
      VLOG(2) << "\n\tdwEncryptionMethod=";
      // https://docs.microsoft.com/en-us/windows/win32/api/ipsectypes/ne-ipsectypes-ipsec_cipher_type
      if (lp_projection_info->ikev2.dwEncryptionMethod ==
          IPSEC_CIPHER_TYPE_DES) {
        VLOG(2) << "IPSEC_CIPHER_TYPE_DES";
      } else if (lp_projection_info->ikev2.dwEncryptionMethod ==
                 IPSEC_CIPHER_TYPE_3DES) {
        VLOG(2) << "IPSEC_CIPHER_TYPE_3DES";
      } else if (lp_projection_info->ikev2.dwEncryptionMethod ==
                 IPSEC_CIPHER_TYPE_AES_128) {
        VLOG(2) << "IPSEC_CIPHER_TYPE_AES_128";
      } else if (lp_projection_info->ikev2.dwEncryptionMethod ==
                 IPSEC_CIPHER_TYPE_AES_192) {
        VLOG(2) << "IPSEC_CIPHER_TYPE_AES_192";
      } else if (lp_projection_info->ikev2.dwEncryptionMethod ==
                 IPSEC_CIPHER_TYPE_AES_256) {
        VLOG(2) << "IPSEC_CIPHER_TYPE_AES_256";
      } else {
        VLOG(2) << "unknown (" << lp_projection_info->ikev2.dwEncryptionMethod
                << ")";
      }

      // -
      VLOG(2) << "\n\tnumIPv4ServerAddresses="
              << lp_projection_info->ikev2.numIPv4ServerAddresses;
      VLOG(2) << "\n\tipv4ServerAddresses=";
      for (DWORD j = 0; j < lp_projection_info->ikev2.numIPv4ServerAddresses;
           j++) {
        VLOG(2) << inet_ntoa(lp_projection_info->ikev2.ipv4ServerAddresses[j]);
        if ((j + 1) < lp_projection_info->ikev2.numIPv4ServerAddresses) {
          VLOG(2) << ", ";
        }
      }
      VLOG(2) << "\n\tnumIPv6ServerAddresses="
              << lp_projection_info->ikev2.numIPv6ServerAddresses;
    } else if (lp_projection_info->type == PROJECTION_INFO_TYPE_PPP) {
      VLOG(2) << "\ttype=PROJECTION_INFO_TYPE_PPP";
    }

    HeapFree(GetProcessHeap(), 0, lp_projection_info);
    lp_projection_info = NULL;
  } else {
    VLOG(2) << "\tError calling RasGetProjectionInfoEx: ";
    PrintRasError(dw_ret);
  }

  return dw_ret;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumconnectionsa
int PrintConnections() {
  DWORD dw_cb = 0;
  DWORD dw_ret = dw_cb;
  DWORD dw_connections = 0;
  LPRASCONN lp_ras_conn = NULL;

  // Call RasEnumConnections with lp_ras_conn = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);
  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    lp_ras_conn =
        (LPRASCONN)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
    if (lp_ras_conn == NULL) {
      LOG(ERROR) << "HeapAlloc failed!";
      return 0;
    }
    // The first RASCONN structure in the array must contain the RASCONN
    // structure size
    lp_ras_conn[0].dwSize = sizeof(RASCONN);

    // Call RasEnumConnections to enumerate active connections
    dw_ret = RasEnumConnections(lp_ras_conn, &dw_cb, &dw_connections);

    // If successful, print the names of the active connections.
    if (ERROR_SUCCESS == dw_ret) {
      VLOG(2) << "The following RAS connections are currently active:\n";
      for (DWORD i = 0; i < dw_connections; i++) {
        VLOG(2) << lp_ras_conn[i].szEntryName;
        PrintConnectionDetails(lp_ras_conn[i].hrasconn);
      }
    }
    VLOG(2) << "\n";
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lp_ras_conn);
    lp_ras_conn = NULL;
    return 0;
  }

  // There was either a problem with RAS or there are no connections to
  // enumerate
  if (dw_connections >= 1) {
    VLOG(2) << "The operation failed to acquire the buffer size.\n\n";
  } else {
    VLOG(2) << "There are no active RAS connections.\n\n";
  }

  return 0;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumdevicesa
int PrintDevices() {
  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  DWORD dw_devices = 0;
  LPRASDEVINFO lp_ras_dev_info = NULL;

  // Call RasEnumDevices with lp_ras_dev_info = NULL. dw_cb is returned with the
  // required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumDevices(lp_ras_dev_info, &dw_cb, &dw_devices);

  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS structure(s).
    lp_ras_dev_info =
        (LPRASDEVINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
    if (lp_ras_dev_info == NULL) {
      VLOG(2) << "HeapAlloc failed!\n";
      return 0;
    }
    // The first RASDEVINFO structure in the array must contain the structure
    // size
    lp_ras_dev_info[0].dwSize = sizeof(RASDEVINFO);

    // Call RasEnumDevices to enumerate RAS devices
    dw_ret = RasEnumDevices(lp_ras_dev_info, &dw_cb, &dw_devices);

    // If successful, print the names of the RAS devices
    if (ERROR_SUCCESS == dw_ret) {
      VLOG(2) << "The following RAS devices were found:\n";
      for (DWORD i = 0; i < dw_devices; i++) {
        VLOG(2) << lp_ras_dev_info[i].szDeviceName;
      }
    }
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lp_ras_dev_info);
    lp_ras_dev_info = NULL;
    return 0;
  }

  // There was either a problem with RAS or there are no RAS devices to
  // enumerate
  if (dw_devices >= 1) {
    VLOG(2) << "The operation failed to acquire the buffer size.\n\n";
  } else {
    VLOG(2) << "There were no RAS devices found.\n\n";
  }

  return 0;
}

void PrintOptions(DWORD options) {
  VLOG(2) << "\tdwfOptions = {\n";
  if (options & RASEO_UseCountryAndAreaCodes) {
    VLOG(2) << "\t\tRASEO_UseCountryAndAreaCodes\n";
  }
  if (options & RASEO_SpecificIpAddr) {
    VLOG(2) << "\t\tRASEO_SpecificIpAddr\n";
  }
  if (options & RASEO_SpecificNameServers) {
    VLOG(2) << "\t\tRASEO_SpecificNameServers\n";
  }
  if (options & RASEO_IpHeaderCompression) {
    VLOG(2) << "\t\tRASEO_IpHeaderCompression\n";
  }
  if (options & RASEO_RemoteDefaultGateway) {
    VLOG(2) << "\t\tRASEO_RemoteDefaultGateway\n";
  }
  if (options & RASEO_DisableLcpExtensions) {
    VLOG(2) << "\t\tRASEO_DisableLcpExtensions\n";
  }
  if (options & RASEO_TerminalBeforeDial) {
    VLOG(2) << "\t\tRASEO_TerminalBeforeDial\n";
  }
  if (options & RASEO_TerminalAfterDial) {
    VLOG(2) << "\t\tRASEO_TerminalAfterDial\n";
  }
  if (options & RASEO_ModemLights) {
    VLOG(2) << "\t\tRASEO_ModemLights\n";
  }
  if (options & RASEO_SwCompression) {
    VLOG(2) << "\t\tRASEO_SwCompression\n";
  }
  if (options & RASEO_RequireEncryptedPw) {
    VLOG(2) << "\t\tRASEO_RequireEncryptedPw\n";
  }
  if (options & RASEO_RequireMsEncryptedPw) {
    VLOG(2) << "\t\tRASEO_RequireMsEncryptedPw\n";
  }
  if (options & RASEO_RequireDataEncryption) {
    VLOG(2) << "\t\tRASEO_RequireDataEncryption\n";
  }
  if (options & RASEO_NetworkLogon) {
    VLOG(2) << "\t\tRASEO_NetworkLogon\n";
  }
  if (options & RASEO_UseLogonCredentials) {
    VLOG(2) << "\t\tRASEO_UseLogonCredentials\n";
  }
  if (options & RASEO_PromoteAlternates) {
    VLOG(2) << "\t\tRASEO_PromoteAlternates\n";
  }

#if (WINVER >= 0x401)
  if (options & RASEO_SecureLocalFiles) {
    VLOG(2) << "\t\tRASEO_SecureLocalFiles\n";
  }
#endif

#if (WINVER >= 0x500)
  if (options & RASEO_RequireEAP) {
    VLOG(2) << "\t\tRASEO_RequireEAP\n";
  }
  if (options & RASEO_RequirePAP) {
    VLOG(2) << "\t\tRASEO_RequirePAP\n";
  }
  if (options & RASEO_RequireSPAP) {
    VLOG(2) << "\t\tRASEO_RequireSPAP\n";
  }
  if (options & RASEO_Custom) {
    VLOG(2) << "\t\tRASEO_Custom\n";
  }

  if (options & RASEO_PreviewPhoneNumber) {
    VLOG(2) << "\t\tRASEO_PreviewPhoneNumber\n";
  }
  if (options & RASEO_SharedPhoneNumbers) {
    VLOG(2) << "\t\tRASEO_SharedPhoneNumbers\n";
  }
  if (options & RASEO_PreviewUserPw) {
    VLOG(2) << "\t\tRASEO_PreviewUserPw\n";
  }
  if (options & RASEO_PreviewDomain) {
    VLOG(2) << "\t\tRASEO_PreviewDomain\n";
  }
  if (options & RASEO_ShowDialingProgress) {
    VLOG(2) << "\t\tRASEO_ShowDialingProgress\n";
  }
  if (options & RASEO_RequireCHAP) {
    VLOG(2) << "\t\tRASEO_RequireCHAP\n";
  }
  if (options & RASEO_RequireMsCHAP) {
    VLOG(2) << "\t\tRASEO_RequireMsCHAP\n";
  }
  if (options & RASEO_RequireMsCHAP2) {
    VLOG(2) << "\t\tRASEO_RequireMsCHAP2\n";
  }
  if (options & RASEO_RequireW95MSCHAP) {
    VLOG(2) << "\t\tRASEO_RequireW95MSCHAP\n";
  }
  if (options & RASEO_CustomScript) {
    VLOG(2) << "\t\tRASEO_CustomScript\n";
  }
#endif

  VLOG(2) << "\t};\n";
}

void PrintOptions2(DWORD options) {
  VLOG(2) << "\tdwfOptions2 = {\n";

#if (WINVER >= 0x501)
  if (options & RASEO2_SecureFileAndPrint) {
    VLOG(2) << "\t\tRASEO2_SecureFileAndPrint\n";
  }
  if (options & RASEO2_SecureClientForMSNet) {
    VLOG(2) << "\t\tRASEO2_SecureClientForMSNet\n";
  }
  if (options & RASEO2_DontNegotiateMultilink) {
    VLOG(2) << "\t\tRASEO2_DontNegotiateMultilink\n";
  }
  if (options & RASEO2_DontUseRasCredentials) {
    VLOG(2) << "\t\tRASEO2_DontUseRasCredentials\n";
  }
  if (options & RASEO2_UsePreSharedKey) {
    VLOG(2) << "\t\tRASEO2_UsePreSharedKey\n";
  }
  if (options & RASEO2_Internet) {
    VLOG(2) << "\t\tRASEO2_Internet\n";
  }
  if (options & RASEO2_DisableNbtOverIP) {
    VLOG(2) << "\t\tRASEO2_DisableNbtOverIP\n";
  }
  if (options & RASEO2_UseGlobalDeviceSettings) {
    VLOG(2) << "\t\tRASEO2_UseGlobalDeviceSettings\n";
  }
  if (options & RASEO2_ReconnectIfDropped) {
    VLOG(2) << "\t\tRASEO2_ReconnectIfDropped\n";
  }
  if (options & RASEO2_SharePhoneNumbers) {
    VLOG(2) << "\t\tRASEO2_SharePhoneNumbers\n";
  }
#endif

#if (WINVER >= 0x600)
  if (options & RASEO2_SecureRoutingCompartment) {
    VLOG(2) << "\t\tRASEO2_SecureRoutingCompartment\n";
  }
  if (options & RASEO2_UseTypicalSettings) {
    VLOG(2) << "\t\tRASEO2_UseTypicalSettings\n";
  }
  if (options & RASEO2_IPv6SpecificNameServers) {
    VLOG(2) << "\t\tRASEO2_IPv6SpecificNameServers\n";
  }
  if (options & RASEO2_IPv6RemoteDefaultGateway) {
    VLOG(2) << "\t\tRASEO2_IPv6RemoteDefaultGateway\n";
  }
  if (options & RASEO2_RegisterIpWithDNS) {
    VLOG(2) << "\t\tRASEO2_RegisterIpWithDNS\n";
  }
  if (options & RASEO2_UseDNSSuffixForRegistration) {
    VLOG(2) << "\t\tRASEO2_UseDNSSuffixForRegistration\n";
  }
  if (options & RASEO2_IPv4ExplicitMetric) {
    VLOG(2) << "\t\tRASEO2_IPv4ExplicitMetric\n";
  }
  if (options & RASEO2_IPv6ExplicitMetric) {
    VLOG(2) << "\t\tRASEO2_IPv6ExplicitMetric\n";
  }
  if (options & RASEO2_DisableIKENameEkuCheck) {
    VLOG(2) << "\t\tRASEO2_DisableIKENameEkuCheck\n";
  }
#endif

#if (WINVER >= 0x601)
  if (options & RASEO2_DisableClassBasedStaticRoute) {
    VLOG(2) << "\t\tRASEO2_DisableClassBasedStaticRoute\n";
  }
  if (options & RASEO2_SpecificIPv6Addr) {
    VLOG(2) << "\t\tRASEO2_SpecificIPv6Addr\n";
  }
  if (options & RASEO2_DisableMobility) {
    VLOG(2) << "\t\tRASEO2_DisableMobility\n";
  }
  if (options & RASEO2_RequireMachineCertificates) {
    VLOG(2) << "\t\tRASEO2_RequireMachineCertificates\n";
  }
#endif

#if (WINVER >= 0x602)
  if (options & RASEO2_UsePreSharedKeyForIkev2Initiator) {
    VLOG(2) << "\t\tRASEO2_UsePreSharedKeyForIkev2Initiator\n";
  }
  if (options & RASEO2_UsePreSharedKeyForIkev2Responder) {
    VLOG(2) << "\t\tRASEO2_UsePreSharedKeyForIkev2Responder\n";
  }
  if (options & RASEO2_CacheCredentials) {
    VLOG(2) << "\t\tRASEO2_CacheCredentials\n";
  }
#endif

#if (WINVER >= 0x603)
  if (options & RASEO2_AutoTriggerCapable) {
    VLOG(2) << "\t\tRASEO2_AutoTriggerCapable\n";
  }
  if (options & RASEO2_IsThirdPartyProfile) {
    VLOG(2) << "\t\tRASEO2_IsThirdPartyProfile\n";
  }
  if (options & RASEO2_AuthTypeIsOtp) {
    VLOG(2) << "\t\tRASEO2_AuthTypeIsOtp\n";
  }
#endif

#if (WINVER >= 0x604)
  if (options & RASEO2_IsAlwaysOn) {
    VLOG(2) << "\t\tRASEO2_IsAlwaysOn\n";
  }
  if (options & RASEO2_IsPrivateNetwork) {
    VLOG(2) << "\t\tRASEO2_IsPrivateNetwork\n";
  }
#endif

#if (WINVER >= 0xA00)
  if (options & RASEO2_PlumbIKEv2TSAsRoutes) {
    VLOG(2) << "\t\tRASEO2_PlumbIKEv2TSAsRoutes\n";
  }
#endif

  VLOG(2) << "\t};";
}

void PrintPolicyValue(LPCTSTR entry_name) {
  std::string error;
  std::wstring phone_book_path = GetPhonebookPath(entry_name, &error);
  if (phone_book_path.empty()) {
    return;
  }

  wchar_t policy_value[1024] = {0};
  DWORD dw_ret =
      GetPrivateProfileString(entry_name, L"CustomIPSecPolicies", L"",
                              policy_value, 1024, phone_book_path.c_str());

  if (dw_ret != 0) {
    VLOG(2) << "\n\n\tCustomIPSecPolicies=" << policy_value;
  }
}

void PrintBytes(LPCWSTR name, LPBYTE bytes, DWORD len) {
  bool next_is_newline = false;
  constexpr int bytes_per_line = 12;
  VLOG(2) << "\n\t[%s: %d bytes]\n\t\t" << name << "," << len;
  for (DWORD i = 0; i < len; i++) {
    if (i > 0 && !next_is_newline) {
      VLOG(2) << ", ";
    }
    VLOG(2) << std::hex << bytes[i];
    next_is_newline = ((i + 1) % bytes_per_line) == 0;
    if (next_is_newline) {
      VLOG(2) << "\n\t\t";
    }
  }
  VLOG(2) << "\n\t[/" << name << "]";
}

int PrintEntryDetails(LPCTSTR entry_name) {
  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  LPRASENTRY lp_ras_entry = NULL;

  // Call RasGetEntryProperties with lp_ras_entry = NULL. dw_cb is returned with
  // the required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasGetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, lp_ras_entry,
                                 &dw_cb, NULL, NULL);
  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    lp_ras_entry =
        (LPRASENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
    if (lp_ras_entry == NULL) {
      VLOG(2) << "HeapAlloc failed!\n";
      return 0;
    }

    // The first lp_ras_entry structure in the array must contain the structure
    // size
    lp_ras_entry[0].dwSize = sizeof(RASENTRY);
    dw_ret = RasGetEntryProperties(DEFAULT_PHONE_BOOK, entry_name, lp_ras_entry,
                                   &dw_cb, NULL, NULL);
    switch (dw_ret) {
      case ERROR_INVALID_SIZE:
        VLOG(2) << "An incorrect structure size was detected.\n";
        break;
    }

    // great place to set debug breakpoint when inspecting existing connections
    PrintOptions(lp_ras_entry->dwfOptions);
    PrintOptions2(lp_ras_entry->dwfOptions2);

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetcustomauthdataa
    LPBYTE custom_auth_data = NULL;
    dw_ret = RasGetCustomAuthData(DEFAULT_PHONE_BOOK, entry_name,
                                  custom_auth_data, &dw_cb);
    if (dw_ret == ERROR_BUFFER_TOO_SMALL && dw_cb > 0) {
      custom_auth_data =
          (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
      dw_ret = RasGetCustomAuthData(DEFAULT_PHONE_BOOK, entry_name,
                                    custom_auth_data, &dw_cb);
      if (dw_ret != ERROR_SUCCESS) {
        PrintRasError(dw_ret);
        if (custom_auth_data) {
          HeapFree(GetProcessHeap(), 0, custom_auth_data);
          custom_auth_data = NULL;
        }
        return dw_ret;
      }
      PrintBytes(L"CustomAuthData", custom_auth_data, dw_cb);
      HeapFree(GetProcessHeap(), 0, custom_auth_data);
    } else if (dw_cb > 0) {
      VLOG(2) << "\n\tError calling RasGetCustomAuthData: ";
      PrintRasError(dw_ret);
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgeteapuserdataa
    LPBYTE eap_user_data = NULL;
    dw_ret = RasGetEapUserData(NULL, DEFAULT_PHONE_BOOK, entry_name,
                               eap_user_data, &dw_cb);
    if (dw_ret == ERROR_BUFFER_TOO_SMALL && dw_cb > 0) {
      eap_user_data =
          (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
      dw_ret = RasGetEapUserData(NULL, DEFAULT_PHONE_BOOK, entry_name,
                                 eap_user_data, &dw_cb);
      if (dw_ret != ERROR_SUCCESS) {
        PrintRasError(dw_ret);
        if (eap_user_data) {
          HeapFree(GetProcessHeap(), 0, eap_user_data);
          eap_user_data = NULL;
        }
        return dw_ret;
      }
      PrintBytes(L"EapUserData", eap_user_data, dw_cb);
      HeapFree(GetProcessHeap(), 0, eap_user_data);
    } else if (dw_cb > 0) {
      VLOG(2) << "\n\tError calling RasGetEapUserData: ";
      PrintRasError(dw_ret);
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgetsubentrypropertiesa
    VLOG(2) << "\n\tdwSubEntries: " << lp_ras_entry->dwSubEntries;
    if (lp_ras_entry->dwSubEntries > 0) {
      for (DWORD i = 0; i < lp_ras_entry->dwSubEntries; i++) {
        LPRASSUBENTRY lp_ras_sub_entry = NULL;
        dw_ret = RasGetSubEntryProperties(DEFAULT_PHONE_BOOK, entry_name, i + 1,
                                          lp_ras_sub_entry, &dw_cb, NULL, NULL);
        if (dw_ret == ERROR_BUFFER_TOO_SMALL && dw_cb > 0) {
          lp_ras_sub_entry = (LPRASSUBENTRY)HeapAlloc(GetProcessHeap(),
                                                      HEAP_ZERO_MEMORY, dw_cb);
          dw_ret =
              RasGetSubEntryProperties(DEFAULT_PHONE_BOOK, entry_name, i + 1,
                                       lp_ras_sub_entry, &dw_cb, NULL, NULL);
          if (dw_ret != ERROR_SUCCESS) {
            PrintRasError(dw_ret);
            if (lp_ras_sub_entry) {
              HeapFree(GetProcessHeap(), 0, lp_ras_sub_entry);
              lp_ras_sub_entry = NULL;
            }
            return dw_ret;
          }
          VLOG(2) << "\n\t\tdwSize=" << lp_ras_sub_entry->dwSize;
          VLOG(2) << "\n\t\tdwfFlags=" << lp_ras_sub_entry->dwfFlags;
          VLOG(2) << "\n\t\tszDeviceType=" << lp_ras_sub_entry->szDeviceType;
          VLOG(2) << "\n\t\tszDeviceName=" << lp_ras_sub_entry->szDeviceName;
          VLOG(2) << "\n\t\tszLocalPhoneNumber="
                  << lp_ras_sub_entry->szLocalPhoneNumber;
          VLOG(2) << "\n\t\tdwAlternateOffset="
                  << lp_ras_sub_entry->dwAlternateOffset;
          HeapFree(GetProcessHeap(), 0, lp_ras_sub_entry);
          lp_ras_sub_entry = NULL;
        } else {
          VLOG(2) << "\n\tError calling RasGetSubEntryProperties: ";
          PrintRasError(dw_ret);
        }
      }
    }
    PrintPolicyValue(entry_name);
    VLOG(2) << "\n";
    // Deallocate memory for the entry buffer
    HeapFree(GetProcessHeap(), 0, lp_ras_entry);
    lp_ras_entry = NULL;
    return ERROR_SUCCESS;
  }

  return dw_ret;
}

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasenumentriesa
int PrintEntries() {
  DWORD dw_cb = 0;
  DWORD dw_ret = ERROR_SUCCESS;
  DWORD dw_entries = 0;
  LPRASENTRYNAME lp_ras_entry_name = NULL;

  // Call RasEnumEntries with lp_ras_entry_name = NULL. dw_cb is returned with
  // the required buffer size and a return code of ERROR_BUFFER_TOO_SMALL
  dw_ret = RasEnumEntries(NULL, NULL, lp_ras_entry_name, &dw_cb, &dw_entries);
  if (dw_ret == ERROR_BUFFER_TOO_SMALL) {
    // Allocate the memory needed for the array of RAS entry names.
    lp_ras_entry_name =
        (LPRASENTRYNAME)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dw_cb);
    if (lp_ras_entry_name == NULL) {
      LOG(ERROR) << "HeapAlloc failed!";
      return 0;
    }
    // The first RASENTRYNAME structure in the array must contain the structure
    // size
    lp_ras_entry_name[0].dwSize = sizeof(RASENTRYNAME);

    // Call RasEnumEntries to enumerate all RAS entry names
    dw_ret = RasEnumEntries(NULL, NULL, lp_ras_entry_name, &dw_cb, &dw_entries);

    // If successful, print the RAS entry names
    if (ERROR_SUCCESS == dw_ret) {
      VLOG(2) << "The following RAS entry names were found:\n";
      for (DWORD i = 0; i < dw_entries; i++) {
        VLOG(2) << lp_ras_entry_name[i].szEntryName;
        dw_ret = PrintEntryDetails(lp_ras_entry_name[i].szEntryName);
      }
    }
    // Deallocate memory for the connection buffer
    HeapFree(GetProcessHeap(), 0, lp_ras_entry_name);
    lp_ras_entry_name = NULL;
    return ERROR_SUCCESS;
  }

  // There was either a problem with RAS or there are RAS entry names to
  // enumerate
  if (dw_entries >= 1) {
    VLOG(2) << "The operation failed to acquire the buffer size.\n\n";
  } else {
    VLOG(2) << "There were no RAS entry names found:.\n\n";
  }

  return dw_ret;
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

// NOTE: This code is never called, but this is how the magic number set in
// brave_vpn::CreateEntry() can be created for the `CustomIPSecPolicies` field.
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
  VLOG(2) << "\nDEMO:\n";
  for (DWORD i = 0; i < 24; i++) {
    VLOG(2) << custom_ipsec_policies[i];
  }
  VLOG(2) << "\n";
}

// Test program for create/remove Windows VPN entry.
int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);

  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->GetSwitches().empty()) {
    LOG(ERROR) << "usage: vpntool.exe [--connections] [--devices] [--entries] "
                  "[--connect --vpn_name=xxx] [--disconnect --vpn_name=xxx] "
                  "[--create --vpn_name=xxx --host_name=xxx user_name=xxx "
                  "password=xxx] [--remove --vpn_name=xxx] [--check-connection "
                  "--vpn_name=xxx]";
    return 0;
  }

  if (command_line->HasSwitch(kConnectionsCommand)) {
    PrintConnections();
  }

  if (command_line->HasSwitch(kCheckConnectionCommand)) {
    const std::wstring vpn_name = command_line->GetSwitchValueNative(kVPNName);
    if (vpn_name.empty()) {
      LOG(ERROR) << "missing parameters for has-connection!";
      LOG(ERROR) << "usage: vpntool.exe --has-connection --vpn_name=entry_name";
      return 0;
    }

    if (CheckConnection(vpn_name) == CheckConnectionResult::CONNECTED) {
      VLOG(2) << "\tFound " << vpn_name << " connection";
    } else {
      VLOG(2) << "\tNot found " << vpn_name << " connection";
    }
  }

  if (command_line->HasSwitch(kDevicesCommand)) {
    PrintDevices();
  }

  if (command_line->HasSwitch(kEntriesCommand)) {
    PrintEntries();
  }

  if (command_line->HasSwitch(kConnectCommand)) {
    const std::wstring vpn_name = command_line->GetSwitchValueNative(kVPNName);
    if (vpn_name.empty()) {
      LOG(ERROR) << "missing parameters for connect!";
      LOG(ERROR) << "usage: vpntool.exe --connect --vpn_name=entry_name";
      return 0;
    }

    ConnectEntry(vpn_name);
    return 0;
  }

  if (command_line->HasSwitch(kDisconnectCommand)) {
    const std::wstring vpn_name = command_line->GetSwitchValueNative(kVPNName);
    if (vpn_name.empty()) {
      LOG(ERROR) << "missing parameters for disconnect!";
      LOG(ERROR) << "usage: vpntool.exe --disconnect --vpn_name=entry_name";
      return 0;
    }

    DisconnectEntry(vpn_name);
    return 0;
  }

  if (command_line->HasSwitch(kRemoveCommand)) {
    const std::wstring vpn_name = command_line->GetSwitchValueNative(kVPNName);
    if (vpn_name.empty()) {
      LOG(ERROR) << "missing parameters for remove!";
      LOG(ERROR) << "usage: vpntool.exe --remove --vpn_name=entry_name";
      return 0;
    }

    RemoveEntry(vpn_name);
    return 0;
  }

  if (command_line->HasSwitch(kCreateCommand)) {
    const std::wstring host_name =
        command_line->GetSwitchValueNative(kHostName);
    const std::wstring vpn_name = command_line->GetSwitchValueNative(kVPNName);
    const std::wstring user_name =
        command_line->GetSwitchValueNative(kUserName);
    const std::wstring password = command_line->GetSwitchValueNative(kPassword);
    if (host_name.empty() || vpn_name.empty() || user_name.empty() ||
        password.empty()) {
      LOG(ERROR) << "missing parameters for create!";
      LOG(ERROR) << "usage: vpntool.exe --create --host_name=xxx "
                    "--vpn_name=xxx --user_name=xxx --password=xxx";
      return 0;
    }

    brave_vpn::BraveVPNConnectionInfo info;
    info.SetConnectionInfo(
        base::WideToUTF8(vpn_name), base::WideToUTF8(host_name),
        base::WideToUTF8(user_name), base::WideToUTF8(password));
    CreateEntry(info);
    return 0;
  }

  return 0;
}
