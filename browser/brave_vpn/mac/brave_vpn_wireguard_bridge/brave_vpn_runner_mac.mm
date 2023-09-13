/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/mac/brave_vpn_wireguard_bridge/brave_vpn_runner_mac.h"

#include <utility>

#import <NetworkExtension/NetworkExtension.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <objc/runtime.h>

#include "base/apple/bridging.h"
#include "base/apple/bundle_locations.h"
#include "base/logging.h"
#include "base/apple/foundation_util.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/uuid.h"
#include "base/values.h"
#import "brave/browser/brave_vpn/mac/brave_vpn_wireguard_bridge/keychain_mac.h"
#include "brave/browser/brave_vpn/mac/brave_vpn_wireguard_bridge/utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {
const char kBraveVpnEntryName[] = "BraveVPNWireguard";
const char kBraveVpnEntryAccountName[] = "Brave Vpn Tunnel: ";
const char kBraveVpnEntryDescription[] = "wg-quick(8) config";

std::string NSErrorToUTF8(NSError* error) {
  return std::string([NSString stringWithFormat:@"%@", error].UTF8String);
}

bool ActivateTunnelProvider(NETunnelProviderManager* tunnel_provider_manager) {
  LOG(ERROR) << __func__ << ":"
             << [tunnel_provider_manager localizedDescription];
  NSError* activation_error;
  [[tunnel_provider_manager connection]
      startVPNTunnelWithOptions:nil
                 andReturnError:&activation_error];
  if (activation_error) {
    LOG(ERROR) << "Tunnel activation error:" << NSErrorToUTF8(activation_error);
  }
  return activation_error == nil;
}

}  // namespace

BraveVpnRunnerMac* BraveVpnRunnerMac::GetInstance() {
  static base::NoDestructor<BraveVpnRunnerMac> instance;
  return instance.get();
}

BraveVpnRunnerMac::BraveVpnRunnerMac() {
  NSBundle* bundle = base::apple::OuterBundle();
  base::apple::SetBaseBundleID(bundle.bundleIdentifier.UTF8String);
  if (std::string(base::apple::BaseBundleID()).empty()) {
    LOG(ERROR) << "No bundle identifiers found";
    NOTREACHED();
  }
}

BraveVpnRunnerMac::~BraveVpnRunnerMac() = default;

int BraveVpnRunnerMac::RemoveVPN() {
  LOG(ERROR) << __func__;
  [NETunnelProviderManager
      loadAllFromPreferencesWithCompletionHandler:^(
          NSArray<NETunnelProviderManager*>* _Nullable managers,
          NSError* _Nullable error) {
        if (error) {
          LOG(ERROR) << "Providers load error:" << NSErrorToUTF8(error);
          return SignalExit(1, __func__);
        }

        if (managers.count == 0) {
          LOG(ERROR) << "No tunnel mangers found";
          return SignalExit(0, __func__);
        }
        for (size_t i = 0; (size_t)i < managers.count; i++) {
          NETunnelProviderManager* tunnel_provider_manager = managers[i];
          if (base::SysNSStringToUTF8(
                  [tunnel_provider_manager localizedDescription]) !=
              kBraveVpnEntryName) {
            LOG(ERROR) << "Skip:"
                       << [tunnel_provider_manager localizedDescription];
            continue;
          }
          NETunnelProviderProtocol* protocol =
              (NETunnelProviderProtocol*)
                  tunnel_provider_manager.protocolConfiguration;

          LOG(ERROR) << "Remove tunnel:"
                     << [tunnel_provider_manager localizedDescription];
          [tunnel_provider_manager removeFromPreferencesWithCompletionHandler:^(
                                       NSError* _Nullable removeError) {
            if (removeError) {
              LOG(ERROR) << __func__ << ":" << NSErrorToUTF8(removeError);
              return;
            }
          }];
          if (protocol) {
            DeleteReference([protocol passwordReference]);
          }
        }
        return SignalExit(0, __func__);
      }];
  return RunLoop();
}

int BraveVpnRunnerMac::EnableVPN(const std::string& config) {
  LOG(ERROR) << __func__;
  [NETunnelProviderManager
      loadAllFromPreferencesWithCompletionHandler:^(
          NSArray<NETunnelProviderManager*>* _Nullable managers,
          NSError* _Nullable error) {
        if (error) {
          LOG(ERROR) << "Providers load error:" << NSErrorToUTF8(error);
          return SignalExit(1, __func__);
        }

        if (managers.count == 0) {
          return CreateTunnelProvider(config);
        }
        for (size_t i = 0; (size_t)i < managers.count; i++) {
          NETunnelProviderManager* tunnel_provider_manager = managers[i];
          if (base::SysNSStringToUTF8(
                  [tunnel_provider_manager localizedDescription]) !=
              kBraveVpnEntryName) {
            LOG(ERROR) << "Skip:"
                       << [tunnel_provider_manager localizedDescription];
            continue;
          }
          NETunnelProviderProtocol* protocol =
              (NETunnelProviderProtocol*)
                  tunnel_provider_manager.protocolConfiguration;
          if (protocol && protocol.providerConfiguration) {
            NSNumber* uid =
                [protocol.providerConfiguration objectForKey:@"UID"];
            if (uid && static_cast<uint32_t>([uid intValue]) == getuid() &&
                VerifyPersistentRefData([protocol passwordReference])) {
              LOG(ERROR) << "Activating existing tunnel";
              ActivateTunnelProvider(tunnel_provider_manager);
              return SignalExit(0, __func__);
            }
          }
          LOG(ERROR) << "Remove orphaned tunnel";
          [tunnel_provider_manager removeFromPreferencesWithCompletionHandler:^(
                                       NSError* _Nullable removeError) {
            if (removeError) {
              LOG(ERROR) << __func__ << ":" << NSErrorToUTF8(removeError);
              return;
            }
          }];
          continue;
        }
        return CreateTunnelProvider(config);
      }];

  return RunLoop();
}

int BraveVpnRunnerMac::RunLoop() {
  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::UI);
  base::RunLoop loop;
  quit_ = loop.QuitClosure();
  loop.Run();
  return 0;
}

void BraveVpnRunnerMac::SignalExit(int code, const std::string& message) {
  LOG(ERROR) << __func__ << ": " << code << ", from " << message;
  std::move(quit_).Run();
}

void BraveVpnRunnerMac::CreateTunnelProvider(const std::string& config) {
  LOG(ERROR) << __func__;
  auto endpoint = brave_vpn::GetConfigStringValue("Endpoint", config);
  if (!endpoint.has_value()) {
    LOG(ERROR) << "Endpoint not found";
    return SignalExit(1, __func__);
  }
  NETunnelProviderProtocol* protocol = [[NETunnelProviderProtocol alloc] init];
  std::string protocolBundlerID = base::apple::BaseBundleID();
  protocolBundlerID += ".network-extension";
  protocol.providerBundleIdentifier =
      base::SysUTF8ToNSString(protocolBundlerID);
  protocol.serverAddress = base::SysUTF8ToNSString(endpoint.value());
  LOG(ERROR) << protocol.serverAddress;
  protocol.providerConfiguration =
      @{@"UID" : [NSNumber numberWithInt:getuid()]};

  NETunnelProviderManager* tunnel_provider_manager =
      [[NETunnelProviderManager alloc] init];
  [tunnel_provider_manager setEnabled:(BOOL)YES];
  [tunnel_provider_manager setProtocolConfiguration:protocol];
  tunnel_provider_manager.localizedDescription =
      base::SysUTF8ToNSString(kBraveVpnEntryName);
  std::string account = kBraveVpnEntryAccountName;
  account += base::Uuid::GenerateRandomV4().AsLowercaseString().c_str();
  NSData* persistentRef = CreatePersistentReference(
      config, kBraveVpnEntryName, account, kBraveVpnEntryDescription);
  if (!persistentRef) {
    LOG(ERROR) << "Failed to create persistent ref in Keychain";
    return SignalExit(1, __func__);
  }
  [[tunnel_provider_manager protocolConfiguration]
      setPasswordReference:persistentRef];
  LOG(ERROR) << "Persistent ref created";
  [tunnel_provider_manager
      saveToPreferencesWithCompletionHandler:^(NSError* _Nullable error) {
        if (error != 0) {
          LOG(ERROR) << __func__ << ":" << NSErrorToUTF8(error);
          return SignalExit(1, __func__);
        }
        LOG(ERROR) << "Create - save & load again.";
        [tunnel_provider_manager loadFromPreferencesWithCompletionHandler:^(
                                     NSError* load_again_error) {
          if (load_again_error) {
            LOG(ERROR) << "load_again_error:"
                       << NSErrorToUTF8(load_again_error);
            return SignalExit(1, __func__);
          }
          ActivateTunnelProvider(tunnel_provider_manager);
          return SignalExit(0, __func__);
        }];
      }];
}

}  // namespace brave_vpn
