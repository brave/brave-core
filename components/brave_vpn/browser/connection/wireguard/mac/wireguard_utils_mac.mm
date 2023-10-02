/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/wireguard_utils_mac.h"

#include <cstddef>
#include <utility>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "base/task/sequenced_task_runner.h"
#include "base/task/sequenced_task_runner.h"
#include "base/uuid.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/mac/keychain_utils_mac.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/mac/utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {
void RemoveTunnelProviderManager(
    NETunnelProviderManager* tunnel_provider_manager) {
  DCHECK(tunnel_provider_manager);
  [tunnel_provider_manager removeFromPreferencesWithCompletionHandler:^(
                               NSError* _Nullable removeError) {
    if (removeError) {
      VLOG(1) << __func__ << ":" << brave_vpn::NSErrorToUTF8(removeError);
      return;
    }
  }];
}

}  // namespace

std::string NSErrorToUTF8(NSError* error) {
  return std::string([NSString stringWithFormat:@"%@", error].UTF8String);
}

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

void FindTunnelProviderManager(const std::string& entry_name,
                               TunnelProviderManagerCallback callback) {
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();
  __block auto internal_callback = std::move(callback);
  __block auto localized_description = entry_name;
  auto handler = ^(NSArray<NETunnelProviderManager*>* _Nullable managers,
                   NSError* _Nullable error) {
    if (error) {
      VLOG(1) << "Providers load error:" << NSErrorToUTF8(error);
      task_runner->PostTask(
          FROM_HERE, base::BindOnce(std::move(internal_callback), nullptr));
      return;
    }

    if (managers.count == 0) {
      VLOG(1) << "Existing provider not found";
      task_runner->PostTask(
          FROM_HERE, base::BindOnce(std::move(internal_callback), nullptr));
      return;
    }
    for (size_t i = 0; (size_t)i < managers.count; i++) {
      NETunnelProviderManager* tunnel_provider_manager = managers[i];
      VLOG(1) << "Checking:" << base::SysNSStringToUTF8(
              [tunnel_provider_manager localizedDescription]) << " vs " << localized_description;
      if (base::SysNSStringToUTF8(
              [tunnel_provider_manager localizedDescription]) != localized_description) {
        VLOG(1) << "Skip:" << [tunnel_provider_manager localizedDescription];
        continue;
      }
      NETunnelProviderProtocol* protocol =
          (NETunnelProviderProtocol*)
              tunnel_provider_manager.protocolConfiguration;
      if (!protocol || !protocol.providerConfiguration) {
        VLOG(1) << "Remove orphaned tunnel";
        RemoveTunnelProviderManager(tunnel_provider_manager);
        continue;
      }
      task_runner->PostTask(FROM_HERE,
                            base::BindOnce(std::move(internal_callback),
                                           tunnel_provider_manager));
      return;
    }
    VLOG(1) << "Existing provider not found";
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(internal_callback), nullptr));
  };
  [NETunnelProviderManager loadAllFromPreferencesWithCompletionHandler:handler];
}

void CreateNewTunnelProviderManager(const std::string& config,
                                    const std::string& entry_name,
                                    const std::string& account_name,
                                    const std::string& entry_description,
                                    TunnelProviderManagerCallback callback) {
  VLOG(1) << __func__ << ":" << config;
  auto endpoint = brave_vpn::GetConfigStringValue("Endpoint", config);
  if (!endpoint.has_value()) {
    VLOG(1) << "Endpoint not found";
    std::move(callback).Run(nullptr);
    return;
  }
  VLOG(1) << __func__ << ": NETunnelProviderProtocol";
  NETunnelProviderProtocol* protocol = [[NETunnelProviderProtocol alloc] init];
  std::string protocolBundlerID = base::apple::BaseBundleID();
  protocolBundlerID += ".network-extension";
  protocol.providerBundleIdentifier =
      base::SysUTF8ToNSString(protocolBundlerID);
  protocol.serverAddress = base::SysUTF8ToNSString(endpoint.value());
  VLOG(1) << protocol.serverAddress;
  protocol.providerConfiguration =
      @{@"UID" : [NSNumber numberWithInt:getuid()]};

  NETunnelProviderManager* tunnel_provider_manager =
      [[NETunnelProviderManager alloc] init];
  [tunnel_provider_manager setEnabled:(BOOL)YES];
  [tunnel_provider_manager setProtocolConfiguration:protocol];
  tunnel_provider_manager.localizedDescription =
      base::SysUTF8ToNSString(entry_name);
  std::string account = account_name;
  account += base::Uuid::GenerateRandomV4().AsLowercaseString().c_str();
  NSData* persistentRef =
      CreatePersistentReference(config, entry_name, account, entry_description);
  if (!persistentRef) {
    VLOG(1) << "Failed to create persistent ref in Keychain";
    std::move(callback).Run(nullptr);
    return;
  }
  [[tunnel_provider_manager protocolConfiguration]
      setPasswordReference:persistentRef];
  VLOG(1) << "Persistent ref created";
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::SequencedTaskRunner::GetCurrentDefault();
  __block auto internal_callback = std::move(callback);

  auto handler = ^(NSError* _Nullable error) {
    if (error != 0) {
      VLOG(1) << __func__ << ":" << NSErrorToUTF8(error);
      task_runner->PostTask(
          FROM_HERE, base::BindOnce(std::move(internal_callback), nullptr));
      return;
    }
    VLOG(1) << "Create - save & load again.";
    [tunnel_provider_manager loadFromPreferencesWithCompletionHandler:^(
                                 NSError* load_again_error) {
      if (load_again_error) {
        VLOG(1) << "load_again_error:" << NSErrorToUTF8(load_again_error);
        task_runner->PostTask(
            FROM_HERE, base::BindOnce(std::move(internal_callback), nullptr));
        return;
      }
      task_runner->PostTask(FROM_HERE,
                            base::BindOnce(std::move(internal_callback),
                                           tunnel_provider_manager));
    }];
  };

  [tunnel_provider_manager saveToPreferencesWithCompletionHandler:handler];
}

}  // namespace brave_vpn
