/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_mac.h"

#import <Foundation/Foundation.h>
#import <NetworkExtension/NetworkExtension.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"

namespace brave_vpn {

// static
BraveVPNConnectionManager* BraveVPNConnectionManager::GetInstance() {
  static base::NoDestructor<BraveVPNConnectionManagerMac> s_manager;
  return s_manager.get();
}

BraveVPNConnectionManagerMac::BraveVPNConnectionManagerMac() {
  NEVPNManager *vpnManager = [NEVPNManager sharedManager];
  [vpnManager loadFromPreferencesWithCompletionHandler:^(NSError *loadError) {
    if (loadError) {
      LOG(ERROR) << __func__ << "############## Load error";
      return;
    }
    LOG(ERROR) << __func__ << "################ Load success";
    BraveVPNConnectionInfo info;
    info.connection_name = "Brave VPN";
    info.hostname = "tokyo-ipsec-1.sudosecuritygroup.com";
    info.username = "4ae94bc1ff636868";
    info.password = "CuStiAYPanDc";
    CreateVPNConnection(info);
  }];
}

BraveVPNConnectionManagerMac::~BraveVPNConnectionManagerMac() = default;

void BraveVPNConnectionManagerMac::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NSString *hostname = base::SysUTF8ToNSString(info.hostname);
  NSString *username = base::SysUTF8ToNSString(info.username);
  NSString *password = base::SysUTF8ToNSString(info.password);
  NSData* password_data = [password dataUsingEncoding:NSUTF8StringEncoding];

  NEVPNProtocolIKEv2 *protocolConfig = [[NEVPNProtocolIKEv2 alloc] init];
  protocolConfig.serverAddress = hostname;
  protocolConfig.serverCertificateCommonName = hostname;
  protocolConfig.remoteIdentifier = hostname;
  protocolConfig.enablePFS = YES;
  protocolConfig.disableMOBIKE = NO;
  protocolConfig.disconnectOnSleep = NO;
  protocolConfig.authenticationMethod = NEVPNIKEAuthenticationMethodCertificate; // to validate the server-side cert issued by LetsEncrypt
  protocolConfig.certificateType = NEVPNIKEv2CertificateTypeECDSA256;
  protocolConfig.useExtendedAuthentication = YES;
  protocolConfig.username = username;
  protocolConfig.passwordReference = password_data;
  protocolConfig.deadPeerDetectionRate = NEVPNIKEv2DeadPeerDetectionRateLow; /* increase DPD tolerance from default 10min to 30min */

  protocolConfig.useConfigurationAttributeInternalIPSubnet = false;

  // TO DO - find out if this all works fine with Always On VPN (allegedly uses two open tunnels at once, for wifi/cellular interfaces)
  // - may require settings "uniqueids" in VPN-side of config to "never" otherwise same EAP creds on both tunnels may cause an issue
  /*
   Params for VPN: AES-256, SHA-384, ECDH over the curve P-384 (DH Group 20)
   TLS for PKI: TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
   */
  [[protocolConfig IKESecurityAssociationParameters] setEncryptionAlgorithm:NEVPNIKEv2EncryptionAlgorithmAES256];
  [[protocolConfig IKESecurityAssociationParameters] setIntegrityAlgorithm:NEVPNIKEv2IntegrityAlgorithmSHA384];
  [[protocolConfig IKESecurityAssociationParameters] setDiffieHellmanGroup:NEVPNIKEv2DiffieHellmanGroup20];
  [[protocolConfig IKESecurityAssociationParameters] setLifetimeMinutes:1440]; // 24 hours
  [[protocolConfig childSecurityAssociationParameters] setEncryptionAlgorithm:NEVPNIKEv2EncryptionAlgorithmAES256GCM];
  [[protocolConfig childSecurityAssociationParameters] setDiffieHellmanGroup:NEVPNIKEv2DiffieHellmanGroup20];
  [[protocolConfig childSecurityAssociationParameters] setLifetimeMinutes:480]; // 8 hours

  NEVPNManager *vpnManager = [NEVPNManager sharedManager];
  vpnManager.enabled = YES;
  vpnManager.protocolConfiguration = protocolConfig;
  //vpnManager.protocolConfiguration = [self prepareIKEv2ParametersForServer:vpnServer eapUsername:eapUsername eapPasswordRef:eapPassword withCertificateType:NEVPNIKEv2CertificateTypeECDSA256];
  vpnManager.localizedDescription = base::SysUTF8ToNSString(info.connection_name);
  vpnManager.onDemandEnabled = YES;
  // RULE: connect to VPN automatically if server reports that it is running OK
  NEOnDemandRuleConnect *vpnServerConnectRule = [[NEOnDemandRuleConnect alloc] init];
  vpnServerConnectRule.interfaceTypeMatch = NEOnDemandRuleInterfaceTypeAny;
  vpnServerConnectRule.probeURL = [NSURL URLWithString:[NSString stringWithFormat:@"https://%@/vpnsrv/api/server-status", hostname]];
  vpnManager.onDemandRules = @[vpnServerConnectRule];

  [vpnManager saveToPreferencesWithCompletionHandler:^(NSError *saveErr) {
    if (saveErr) {
      NSLog(@"[DEBUG] saveErr = %@", saveErr);
      return;
    } else {
      [vpnManager loadFromPreferencesWithCompletionHandler:^(NSError *error) {
        NSError *vpnErr;
        [[vpnManager connection] startVPNTunnelAndReturnError:&vpnErr];
        if (vpnErr != nil) {
          NSLog(@"[DEBUG] vpnErr from connection() = %@", vpnErr);
          return;
        } else {
          NSLog(@"[DEBUG] created successful VPN connection");
          return;
        }
      }];
    }
  }];
}

void BraveVPNConnectionManagerMac::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {}

void BraveVPNConnectionManagerMac::RemoveVPNConnection(
    const std::string& name) {}

void BraveVPNConnectionManagerMac::Connect(const std::string& name) {}

void BraveVPNConnectionManagerMac::Disconnect(const std::string& name) {}

}  // namespace brave_vpn
