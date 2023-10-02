/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/keychain_utils_mac.h"

#include <string>

#import <NetworkExtension/NetworkExtension.h>
#import <objc/runtime.h>

#include "base/apple/bridging.h"
#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"

using ScopedMutableDictionary =
    base::apple::ScopedCFTypeRef<CFMutableDictionaryRef>;

namespace brave_vpn {

namespace {
const char kNetworkExtensionName[] = "WireGuardNetworkExtension.appex";
}  // namespace

bool VerifyPersistentRefData(NSData* data) {
  VLOG(1) << __func__;
  ScopedMutableDictionary query =
      ScopedMutableDictionary(CFDictionaryCreateMutable(
          kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
          &kCFTypeDictionaryValueCallBacks));
  CFDictionarySetValue(query.get(), kSecValuePersistentRef,
                       base::apple::NSToCFPtrCast(data));

  return (SecItemCopyMatching(query, (CFTypeRef*)0) != errSecItemNotFound);
}

bool DeleteReference(NSData* data) {
  LOG(ERROR) << __func__ << ":" << data;
  ScopedMutableDictionary query =
      ScopedMutableDictionary(CFDictionaryCreateMutable(
          kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
          &kCFTypeDictionaryValueCallBacks));
  CFDictionarySetValue(query.get(), kSecValuePersistentRef,
                       base::apple::NSToCFPtrCast(data));
  return SecItemDelete(query) == errSecSuccess;
}

NSData* CreatePersistentReference(const std::string& config,
                                  const std::string& label,
                                  const std::string& account,
                                  const std::string& description) {
  VLOG(1) << __func__ << ": config:" << config;
  VLOG(1) << __func__ << ": label:" << label;
  VLOG(1) << __func__ << ": account:" << account;
  base::FilePath plugins(
      base::apple::MainBundlePath().Append("Contents").Append("PlugIns"));
  base::FilePath extension_path = plugins.Append(kNetworkExtensionName);
  VLOG(1) << __func__ << ":" << extension_path;
// TODO(spylogsster): Remove SecTrustedApplicationCreateFromPath
// https://github.com/brave/brave-browser/issues/33138
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  base::apple::ScopedCFTypeRef<SecTrustedApplicationRef> extensionApp;
  OSStatus status = SecTrustedApplicationCreateFromPath(
      extension_path.MaybeAsASCII().c_str(), extensionApp.InitializeInto());
  if (status != errSecSuccess) {
    VLOG(1) << __func__ << ": Failed to get trusted application path for "
            << extension_path << ", error:" << status;
    return nil;
  }
  VLOG(1) << __func__;
  base::apple::ScopedCFTypeRef<SecTrustedApplicationRef> mainApp;
  status = SecTrustedApplicationCreateFromPath(
      base::apple::MainBundlePath().MaybeAsASCII().c_str(),
      mainApp.InitializeInto());
  if (status != errSecSuccess) {
    VLOG(1) << __func__ << ": Failed to get trusted application path for "
            << base::apple::MainBundlePath() << ", error:" << status;
    return nil;
  }
  VLOG(1) << __func__;
  const SecTrustedApplicationRef apps_array[] = {extensionApp.get(),
                                                 mainApp.get()};
  base::apple::ScopedCFTypeRef<CFArrayRef> apps(
      CFArrayCreate(NULL, reinterpret_cast<const void**>(&apps_array),
                    std::size(apps_array), &kCFTypeArrayCallBacks));
  base::apple::ScopedCFTypeRef<SecAccessRef> access_ref;
  status = SecAccessCreate(base::SysUTF8ToCFStringRef(label), apps.get(),
                           access_ref.InitializeInto());
  if (status != errSecSuccess) {
    VLOG(1) << __func__ << ": Failed to create Keychain access ref";
    return nil;
  }
  VLOG(1) << __func__;
#pragma clang diagnostic pop

  NSDictionary* items = @{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrLabel : base::SysUTF8ToNSString(label),
    (__bridge id)kSecAttrAccount : base::SysUTF8ToNSString(account),
    (__bridge id)kSecAttrDescription : base::SysUTF8ToNSString(description),
    (__bridge id)
    kSecAttrService : base::SysUTF8ToNSString(base::apple::BaseBundleID()),
    (__bridge id)kSecValueData : base::SysUTF8ToNSString(config),
    (__bridge id)kSecReturnPersistentRef : (__bridge id)kCFBooleanTrue,
    (__bridge id)kSecAttrSynchronizable : (__bridge id)kCFBooleanFalse,
    (__bridge id)kSecAttrAccessible :
        (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly,
    (__bridge id)kSecAttrAccess : (__bridge id)access_ref.get(),
  };
  VLOG(1) << __func__;
  base::apple::ScopedCFTypeRef<CFDataRef> cf_result;
  status = SecItemAdd((__bridge CFDictionaryRef)items,
                      (CFTypeRef*)cf_result.InitializeInto());
  VLOG(1) << __func__ << ":" << status;
  if (status == errSecDuplicateItem) {
    VLOG(1) << __func__
            << ": There is duplicated key in keychain:" << cf_result.get();
    NOTREACHED();
    return nil;
  }
  VLOG(1) << __func__ << ":" << status;
  if (status != errSecSuccess) {
    VLOG(1) << __func__
            << ": Failed to save persistent ref in Keychain:" << status;
    return nil;
  }
  VLOG(1) << __func__ << ":" << status;
  NSData* persistent_ref_data = (__bridge NSData*)cf_result.release();
  VLOG(1) << __func__ << ":" << status;
  CHECK(VerifyPersistentRefData(persistent_ref_data));
  return persistent_ref_data;
}
}  // namespace brave_vpn
