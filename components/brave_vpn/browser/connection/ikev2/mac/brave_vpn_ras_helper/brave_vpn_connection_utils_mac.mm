/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/mac/brave_vpn_ras_helper/brave_vpn_connection_utils_mac.h"
#include "base/run_loop.h"

#import <NetworkExtension/NetworkExtension.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/single_thread_task_executor.h"

namespace {
NEVPNProtocolIKEv2* CreateProtocolConfig(const std::string& username_in,
                                         const std::string& hostname_in) {
  NSString* hostname = base::SysUTF8ToNSString(hostname_in);
  NSString* username = base::SysUTF8ToNSString(username_in);

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
  // protocol_config.passwordReference = GetPasswordRefForAccount();
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

namespace brave {

int CreateVPN(const std::string& connection_name) {
  base::SingleThreadTaskExecutor service_task_executor(
      base::MessagePumpType::UI);

  NEVPNManager* vpn_manager = [NEVPNManager sharedManager];
  LOG(ERROR) << __func__ << ":" << connection_name;
  base::RunLoop loop;
  base::OnceClosure quit = loop.QuitClosure();
  [vpn_manager loadFromPreferencesWithCompletionHandler:^(NSError* error) {
    if (error) {
      LOG(ERROR) << "Create - loadFromPrefs error: "
                 << base::SysNSStringToUTF8([error localizedDescription]);
      return;
    }
    LOG(ERROR) << "No error in load from preferences";

    [vpn_manager setEnabled:YES];
    [vpn_manager setProtocolConfiguration:CreateProtocolConfig("a", "b")];
    [vpn_manager
        setLocalizedDescription:base::SysUTF8ToNSString(connection_name)];

    [vpn_manager saveToPreferencesWithCompletionHandler:^(NSError* save_error) {
      if (save_error) {
        LOG(ERROR) << "Create - saveToPrefs error: "
                   << base::SysNSStringToUTF8(
                          [save_error localizedDescription]);
        return;
      }
      // Load & save twice avoid connect failure.
      // This load & save twice hack could eliminate connect failure
      // when os vpn entry needs to be newly created during the connect
      // process.
      LOG(ERROR) << "Create - load & save again.";
      [vpn_manager loadFromPreferencesWithCompletionHandler:^(
                       NSError* load_again_error) {
        if (load_again_error) {
          LOG(ERROR) << "Create - load & save again error: "
                     << base::SysNSStringToUTF8(
                            [load_again_error localizedDescription]);
          return;
        }

        [vpn_manager saveToPreferencesWithCompletionHandler:^(
                         NSError* save_again_error) {
          if (save_again_error) {
            LOG(ERROR) << "Create - save_again_error error: "
                       << base::SysNSStringToUTF8(
                              [save_again_error localizedDescription]);
            return;
          }
        }];
      }];
    }];
    //    exit_signal.Signal();
  }];

  loop.Run();
  return 0;
}

}  // namespace brave
