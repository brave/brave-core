/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api_mac.h"

#import <NetworkExtension/NetworkExtension.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"

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
      NOTREACHED();
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
  if (results != errSecSuccess)
    LOG(ERROR) << "Error: obtaining password ref(status:" << results << ")";
  return (__bridge NSData*)copy_result;
}

OSStatus RemoveKeychainItemForAccount() {
  NSString* bundle_id = [[NSBundle mainBundle] bundleIdentifier];
  NSDictionary* query = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : bundle_id,
    (__bridge id)kSecAttrAccount : kBraveVPNKey,
    (__bridge id)kSecReturnPersistentRef : (__bridge id)kCFBooleanTrue,
  };
  OSStatus result = SecItemDelete((__bridge CFDictionaryRef)query);
  if (result != errSecSuccess && result != errSecItemNotFound)
    LOG(ERROR) << "Error: deleting password entry(status:" << result << ")";
  return result;
}

OSStatus StorePassword(const NSString* password) {
  if (password == nil || [password length] == 0) {
    LOG(ERROR) << "Error: password is empty";
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

  if (status != errSecSuccess)
    LOG(ERROR) << "Error: storing password";

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

// static
BraveVPNOSConnectionAPI* BraveVPNOSConnectionAPI::GetInstance() {
  static base::NoDestructor<BraveVPNOSConnectionAPIMac> s_manager;
  return s_manager.get();
}

BraveVPNOSConnectionAPIMac::BraveVPNOSConnectionAPIMac() {
  ObserveVPNConnectionChange();
}

BraveVPNOSConnectionAPIMac::~BraveVPNOSConnectionAPIMac() {
  if (vpn_observer_) {
    [[NSNotificationCenter defaultCenter] removeObserver:vpn_observer_];
    vpn_observer_ = nil;
  }
}

void BraveVPNOSConnectionAPIMac::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  info_ = info;

  if (StorePassword(base::SysUTF8ToNSString(info_.password())) !=
      errSecSuccess) {
    for (Observer& obs : observers_)
      obs.OnCreateFailed();
    return;
  }

  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (error) {
      LOG(ERROR) << "Create - loadFromPrefs error: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      for (Observer& obs : observers_)
        obs.OnCreateFailed();
      return;
    }

    [vpn_manager setEnabled:YES];
    [vpn_manager setProtocolConfiguration:CreateProtocolConfig(info_)];
    [vpn_manager setLocalizedDescription:base::SysUTF8ToNSString(
                                             info_.connection_name())];

    [vpn_manager saveToPreferencesWithCompletionHandler:^(NSError* save_error) {
      if (save_error) {
        LOG(ERROR) << "Create - saveToPrefs error: "
                   << base::SysNSStringToUTF8(
                          [save_error localizedDescription]);
        for (Observer& obs : observers_)
          obs.OnCreateFailed();

        return;
      }
      VLOG(2) << "Create - saveToPrefs success";
      for (Observer& obs : observers_)
        obs.OnCreated();
    }];
  }];
}

void BraveVPNOSConnectionAPIMac::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
}

void BraveVPNOSConnectionAPIMac::RemoveVPNConnection(const std::string& name) {
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (error) {
      LOG(ERROR) << "RemoveVPNConnection - loadFromPrefs: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
    } else {
      [vpn_manager
          removeFromPreferencesWithCompletionHandler:^(NSError* remove_error) {
            if (remove_error) {
              LOG(ERROR) << "RemoveVPNConnection - removeFromPrefs: "
                         << base::SysNSStringToUTF8(
                                [remove_error localizedDescription]);
            }
            VLOG(2) << "RemoveVPNConnection - successfully removed";
            for (Observer& obs : observers_)
              obs.OnRemoved();
          }];
    }
    RemoveKeychainItemForAccount();
  }];
}

void BraveVPNOSConnectionAPIMac::Connect(const std::string& name) {
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (error) {
      LOG(ERROR) << "Connect - loadFromPrefs error: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      for (Observer& obs : observers_)
        obs.OnConnectFailed();
      return;
    }

    NEVPNStatus current_status = [[vpn_manager connection] status];
    // Early return if already connected.
    if (current_status == NEVPNStatusConnected) {
      VLOG(2) << "Connect - Already connected";
      for (Observer& obs : observers_)
        obs.OnConnected();
      return;
    }

    NSError* start_error;
    [[vpn_manager connection] startVPNTunnelAndReturnError:&start_error];
    if (start_error != nil) {
      LOG(ERROR) << "Connect - startVPNTunnel error: "
                 << base::SysNSStringToUTF8([start_error localizedDescription]);
      for (Observer& obs : observers_)
        obs.OnConnectFailed();
      return;
    }
  }];
}

void BraveVPNOSConnectionAPIMac::Disconnect(const std::string& name) {
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
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

    [[vpn_manager connection] stopVPNTunnel];
  }];
}

void BraveVPNOSConnectionAPIMac::CheckConnection(const std::string& name) {
  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (error) {
      LOG(ERROR) << "Connect - loadFromPrefs error: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      return;
    }

    NEVPNStatus current_status = [[vpn_manager connection] status];
    VLOG(2) << "CheckConnection: " << NEVPNStatusToString(current_status);
    switch (current_status) {
      case NEVPNStatusConnected:
        for (Observer& obs : observers_)
          obs.OnConnected();
        break;
      case NEVPNStatusConnecting:
      case NEVPNStatusReasserting:
        for (Observer& obs : observers_)
          obs.OnIsConnecting();
        break;
      case NEVPNStatusDisconnected:
      case NEVPNStatusInvalid:
        for (Observer& obs : observers_)
          obs.OnDisconnected();
        break;
      case NEVPNStatusDisconnecting:
        for (Observer& obs : observers_)
          obs.OnIsDisconnecting();
        break;
      default:
        break;
    }
  }];
}

void BraveVPNOSConnectionAPIMac::ObserveVPNConnectionChange() {
  vpn_observer_ = [[NSNotificationCenter defaultCenter]
      addObserverForName:NEVPNStatusDidChangeNotification
                  object:nil
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                VLOG(2) << "Received VPN connection status change notification";
                CheckConnection(std::string());
              }];
}

}  // namespace brave_vpn
