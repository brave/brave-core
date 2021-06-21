/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_mac.h"

#import <Foundation/Foundation.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_vpn/guardian_helper.h"

namespace brave_vpn {

// static
BraveVPNConnectionManager* BraveVPNConnectionManager::GetInstance(
    bool for_tool) {
  static base::NoDestructor<BraveVPNConnectionManagerMac> s_manager(for_tool);
  return s_manager.get();
}

BraveVPNConnectionManagerMac::BraveVPNConnectionManagerMac(bool for_tool) {
  NSString* guardian_connect_path = nil;

  if (for_tool) {
    base::FilePath cwd;
    base::GetCurrentDirectory(&cwd);
    cwd = cwd.AppendASCII("../out/Component/GuardianConnect.framework");
    guardian_connect_path = base::SysUTF8ToNSString(cwd.AsUTF8Unsafe());
  } else {
    guardian_connect_path =
        [[base::mac::FrameworkBundle() privateFrameworksPath]
            stringByAppendingPathComponent:@"GuardianConnect.framework"];
  }
  DCHECK(guardian_connect_path);

  NSBundle* bundle = [NSBundle bundleWithPath:guardian_connect_path];
  [bundle load];

  vpn_helper_class_ = [bundle classNamed:@"GRDVPNHelper"];
  helper_ = [vpn_helper_class_ sharedInstance];
  DCHECK(helper_);
}

BraveVPNConnectionManagerMac::~BraveVPNConnectionManagerMac() = default;

void BraveVPNConnectionManagerMac::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NSString* email = base::SysUTF8ToNSString(info.username);
  NSString* password = base::SysUTF8ToNSString(info.password);

  [helper_ proLoginWithEmail:email
                    password:password
                  completion:^(NSDictionary* _Nullable response,
                               NSString* _Nullable errorMessage, BOOL success) {
                    if (success) {
                      LOG(ERROR) << __func__ << " Create success!";
                    }
                  }];
}

void BraveVPNConnectionManagerMac::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {}

void BraveVPNConnectionManagerMac::RemoveVPNConnection(
    const std::string& name) {
  [vpn_helper_class_ clearVpnConfiguration];
}

void BraveVPNConnectionManagerMac::Connect(const std::string& name) {
  [helper_
      configureFirstTimeUserWithRegion:nil
                            completion:^(BOOL success,
                                         NSString* _Nullable errorMessage) {
                              if (success) {
                                LOG(ERROR) << __func__ << " Connect success!";
                              }
                            }];
}

void BraveVPNConnectionManagerMac::Disconnect(const std::string& name) {
  [helper_ disconnectVPN];
}

}  // namespace brave_vpn
