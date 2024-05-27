/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/mac/ikev2_connection_api_impl_mac.h"

#include <memory>

#import <NetworkExtension/NetworkExtension.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <netinet/in.h>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

// Referenced GuardianConnect implementation.
// https://github.com/GuardianFirewall/GuardianConnect
namespace brave_vpn {

namespace {

const NSString* kBraveVPNKey = @"BraveVPNKey";

std::string NEVPNStatusToString(NEVPNStatus status) {
  switch (status) {
    case NEVPNStatusInvalid:
      return "NEVPNStatusInvalid";
    case NEVPNStatusDisconnected:
      return "NEVPNStatusDisconnected";
    case NEVPNStatusConnecting:
      return "NEVPNStatusConnecting";
    case NEVPNStatusConnected:
      return "NEVPNStatusConnected";
    case NEVPNStatusReasserting:
      return "NEVPNStatusReasserting";
    case NEVPNStatusDisconnecting:
      return "NEVPNStatusDisconnecting";
    default:
      NOTREACHED_IN_MIGRATION();
  }
  return "NEVPNStatusInvalid";
}

NSData* GetPasswordRefForAccount() {
  NSString* bundle_id = [[NSBundle mainBundle] bundleIdentifier];
  CFTypeRef copy_result = nullptr;
  NSDictionary* query = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : bundle_id,
    (__bridge id)kSecAttrAccount : kBraveVPNKey,
    (__bridge id)kSecMatchLimit : (__bridge id)kSecMatchLimitOne,
    (__bridge id)kSecReturnPersistentRef : (__bridge id)kCFBooleanTrue,
  };
  OSStatus results =
      SecItemCopyMatching((__bridge CFDictionaryRef)query, &copy_result);
  if (results != errSecSuccess) {
    LOG(ERROR) << "Error: obtaining password ref(status:" << results << ")";
  }
  return (__bridge NSData*)copy_result;
}

OSStatus StorePassword(const NSString* password, std::string* error_str) {
  DCHECK(error_str);
  std::string error;
  if (password == nil || [password length] == 0) {
    *error_str = "Error: password is empty";
    LOG(ERROR) << *error_str;
    return errSecParam;
  }

  CFTypeRef result = nullptr;
  NSString* bundle_id = [[NSBundle mainBundle] bundleIdentifier];
  NSData* password_data = [password dataUsingEncoding:NSUTF8StringEncoding];
  NSDictionary* sec_item = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : bundle_id,
    (__bridge id)kSecAttrSynchronizable : (__bridge id)kCFBooleanFalse,
    (__bridge id)kSecAttrAccount : kBraveVPNKey,
    (__bridge id)kSecValueData : password_data,
  };
  OSStatus status = SecItemAdd((__bridge CFDictionaryRef)sec_item, &result);
  if (status == errSecDuplicateItem) {
    VLOG(2) << "There is duplicated key in keychain. Update it.";
    NSDictionary* query = @{
      (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
      (__bridge id)kSecAttrService : bundle_id,
      (__bridge id)kSecAttrAccount : kBraveVPNKey,
      (__bridge id)kSecReturnPersistentRef : (__bridge id)kCFBooleanTrue,
    };
    status = SecItemUpdate((__bridge CFDictionaryRef)query,
                           (__bridge CFDictionaryRef)sec_item);
  }

  if (status != errSecSuccess) {
    *error_str = "Error: storing password";
    LOG(ERROR) << *error_str;
  }

  return status;
}

NEVPNProtocolIKEv2* CreateProtocolConfig(const BraveVPNConnectionInfo& info) {
  NSString* hostname = base::SysUTF8ToNSString(info.hostname());
  NSString* username = base::SysUTF8ToNSString(info.username());

  NEVPNProtocolIKEv2* protocol_config = [[NEVPNProtocolIKEv2 alloc] init];
  protocol_config.serverAddress = hostname;
  protocol_config.serverCertificateCommonName = hostname;
  protocol_config.remoteIdentifier = hostname;
  protocol_config.enablePFS = YES;
  protocol_config.disableMOBIKE = NO;
  protocol_config.disconnectOnSleep = NO;
  protocol_config.authenticationMethod =
      NEVPNIKEAuthenticationMethodCertificate;  // to validate the server-side
                                                // cert issued by LetsEncrypt
  protocol_config.certificateType = NEVPNIKEv2CertificateTypeECDSA256;
  protocol_config.useExtendedAuthentication = YES;
  protocol_config.username = username;
  protocol_config.passwordReference = GetPasswordRefForAccount();
  protocol_config.deadPeerDetectionRate =
      NEVPNIKEv2DeadPeerDetectionRateLow; /* increase DPD tolerance from default
                                             10min to 30min */
  protocol_config.useConfigurationAttributeInternalIPSubnet = false;

  [[protocol_config IKESecurityAssociationParameters]
      setEncryptionAlgorithm:NEVPNIKEv2EncryptionAlgorithmAES256];
  [[protocol_config IKESecurityAssociationParameters]
      setIntegrityAlgorithm:NEVPNIKEv2IntegrityAlgorithmSHA384];
  [[protocol_config IKESecurityAssociationParameters]
      setDiffieHellmanGroup:NEVPNIKEv2DiffieHellmanGroup20];
  [[protocol_config IKESecurityAssociationParameters]
      setLifetimeMinutes:1440];  // 24 hours
  [[protocol_config childSecurityAssociationParameters]
      setEncryptionAlgorithm:NEVPNIKEv2EncryptionAlgorithmAES256GCM];
  [[protocol_config childSecurityAssociationParameters]
      setDiffieHellmanGroup:NEVPNIKEv2DiffieHellmanGroup20];
  [[protocol_config childSecurityAssociationParameters]
      setLifetimeMinutes:480];  // 8 hours

  return protocol_config;
}

}  // namespace

IKEv2ConnectionAPIImplMac::IKEv2ConnectionAPIImplMac(
    BraveVPNConnectionManager* manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : SystemVPNConnectionAPIImplBase(manager, url_loader_factory) {
  ObserveVPNConnectionChange();
}

IKEv2ConnectionAPIImplMac::~IKEv2ConnectionAPIImplMac() {
  if (vpn_observer_) {
    [[NSNotificationCenter defaultCenter] removeObserver:vpn_observer_];
    vpn_observer_ = nil;
  }
}

void IKEv2ConnectionAPIImplMac::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  std::string store_pwd_error;
  if (StorePassword(base::SysUTF8ToNSString(info.password()),
                    &store_pwd_error) != errSecSuccess) {
    SetLastConnectionError(store_pwd_error);
    OnCreateFailed();
    return;
  }

  auto weak_ptr = weak_factory_.GetWeakPtr();
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (!weak_ptr) {
      return;
    }

    if (error) {
      SetLastConnectionError(
          base::SysNSStringToUTF8([error localizedDescription]));
      LOG(ERROR) << "Create - loadFromPrefs error: "
                 << GetLastConnectionError();
      OnCreateFailed();
      return;
    }

    [vpn_manager setEnabled:YES];
    [vpn_manager setProtocolConfiguration:CreateProtocolConfig(info)];
    [vpn_manager setLocalizedDescription:base::SysUTF8ToNSString(
                                             info.connection_name())];
    if (IsOnDemandEnabled()) {
      [vpn_manager setOnDemandEnabled:YES];
      NEOnDemandRuleConnect* vpn_server_connect_rule =
          [[NEOnDemandRuleConnect alloc] init];
      vpn_server_connect_rule.interfaceTypeMatch =
          NEOnDemandRuleInterfaceTypeAny;
      vpn_server_connect_rule.probeURL = [NSURL
          URLWithString:
              [NSString
                  stringWithFormat:@"https://%@/vpnsrv/api/server-status",
                                   base::SysUTF8ToNSString(info.hostname())]];
      [vpn_manager setOnDemandRules:@[ vpn_server_connect_rule ]];
    }

    [vpn_manager saveToPreferencesWithCompletionHandler:^(NSError* save_error) {
      if (!weak_ptr) {
        return;
      }

      if (save_error) {
        SetLastConnectionError(
            base::SysNSStringToUTF8([save_error localizedDescription]));
        LOG(ERROR) << "Create - saveToPrefs error: "
                   << GetLastConnectionError();
        OnCreateFailed();
        return;
      }
      // Load & save twice avoid connect failure.
      // This load & save twice hack could eliminate connect failure
      // when os vpn entry needs to be newly created during the connect
      // process.
      VLOG(2) << "Create - load & save again.";
      [vpn_manager loadFromPreferencesWithCompletionHandler:^(
                       NSError* load_again_error) {
        if (!weak_ptr) {
          return;
        }

        if (load_again_error) {
          SetLastConnectionError(
              base::SysNSStringToUTF8([load_again_error localizedDescription]));
          LOG(ERROR) << "Create - load again error: "
                     << GetLastConnectionError();
          OnCreateFailed();
          return;
        }

        [vpn_manager saveToPreferencesWithCompletionHandler:^(
                         NSError* save_again_error) {
          if (!weak_ptr) {
            return;
          }

          if (save_again_error) {
            SetLastConnectionError(base::SysNSStringToUTF8(
                [save_again_error localizedDescription]));
            LOG(ERROR) << "Create - save again error: "
                       << GetLastConnectionError();
            OnCreateFailed();
            return;
          }
          OnCreated();
        }];
      }];
    }];
  }];
}

void IKEv2ConnectionAPIImplMac::ConnectImpl(const std::string& name) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (!weak_ptr) {
      return;
    }

    if (error) {
      SetLastConnectionError(
          base::SysNSStringToUTF8([error localizedDescription]));
      LOG(ERROR) << "Connect - loadFromPrefs error: "
                 << GetLastConnectionError();
      OnConnectFailed();
      return;
    }

    NEVPNStatus current_status = [[vpn_manager connection] status];
    // Early return if already connected.
    if (current_status == NEVPNStatusConnected) {
      VLOG(2) << "Connect - Already connected";
      OnConnected();
      return;
    }

    NSError* start_error;
    [[vpn_manager connection] startVPNTunnelAndReturnError:&start_error];
    if (start_error != nil) {
      LOG(ERROR) << "Connect - startVPNTunnel error: "
                 << base::SysNSStringToUTF8([start_error localizedDescription]);
      OnConnectFailed();
      return;
    }
  }];
}

void IKEv2ConnectionAPIImplMac::DisconnectImpl(const std::string& name) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (!weak_ptr) {
      return;
    }

    if (error) {
      LOG(ERROR) << "Disconnect - loadFromPrefs: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      return;
    }

    NEVPNStatus current_status = [[vpn_manager connection] status];
    if (current_status == NEVPNStatusDisconnecting ||
        current_status == NEVPNStatusDisconnected) {
      VLOG(2) << "Don't need to ask disconnect: " << current_status;
      return;
    }

    // Always clear on-demand bit when user disconnect vpn connection.
    [vpn_manager setOnDemandEnabled:NO];
    [vpn_manager saveToPreferencesWithCompletionHandler:^(NSError* save_error) {
      if (!weak_ptr) {
        return;
      }

      if (save_error) {
        SetLastConnectionError(
            base::SysNSStringToUTF8([save_error localizedDescription]));
        LOG(ERROR) << "Create - saveToPrefs error: "
                   << GetLastConnectionError();
      }
      [[vpn_manager connection] stopVPNTunnel];
    }];
  }];
}

void IKEv2ConnectionAPIImplMac::CheckConnectionImpl(const std::string& name) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (!weak_ptr) {
      return;
    }

    if (error) {
      LOG(ERROR) << "Connect - loadFromPrefs error: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      return;
    }

    NEVPNStatus current_status = [[vpn_manager connection] status];
    VLOG(2) << "CheckConnection: " << NEVPNStatusToString(current_status);
    switch (current_status) {
      case NEVPNStatusReasserting:
        // See this link for more details about why this status is ingored.
        // https://github.com/brave/brave-browser/issues/29500#issuecomment-1989762108
        VLOG(2) << "Ignore NEVPNStatusReasserting";
        break;
      case NEVPNStatusConnected:
        OnConnected();
        break;
      case NEVPNStatusConnecting:
        OnIsConnecting();
        break;
      case NEVPNStatusDisconnected:
      case NEVPNStatusInvalid:
        OnDisconnected();
        break;
      case NEVPNStatusDisconnecting:
        OnIsDisconnecting();
        break;
      default:
        break;
    }
  }];
}

void IKEv2ConnectionAPIImplMac::ObserveVPNConnectionChange() {
  vpn_observer_ = [[NSNotificationCenter defaultCenter]
      addObserverForName:NEVPNStatusDidChangeNotification
                  object:nil
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                VLOG(2) << "Received VPN connection status change notification";
                CheckConnectionImpl(std::string());
              }];
}

bool IKEv2ConnectionAPIImplMac::IsPlatformNetworkAvailable() {
  // 0.0.0.0 is a special token that causes reachability to monitor the general
  // routing status of the device, both IPv4 and IPv6.
  struct sockaddr_in addr = {0};
  addr.sin_len = sizeof(addr);
  addr.sin_family = AF_INET;
  base::apple::ScopedCFTypeRef<SCNetworkReachabilityRef> reachability(
      SCNetworkReachabilityCreateWithAddress(
          kCFAllocatorDefault, reinterpret_cast<struct sockaddr*>(&addr)));
  SCNetworkReachabilityFlags flags;
  BOOL success = SCNetworkReachabilityGetFlags(reachability.get(), &flags);
  if (!success) {
    return false;
  }
  BOOL isReachable = flags & kSCNetworkReachabilityFlagsReachable;
  BOOL needsConnection = flags & kSCNetworkReachabilityFlagsConnectionRequired;
  return isReachable && !needsConnection;
}

bool IKEv2ConnectionAPIImplMac::IsOnDemandEnabled() const {
  return manager_->local_prefs()->GetBoolean(prefs::kBraveVPNOnDemandEnabled);
}

}  // namespace brave_vpn
