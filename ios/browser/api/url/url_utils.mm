/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/url/url_utils.h"

#include <optional>

#include "base/strings/sys_string_conversions.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#import "net/base/apple/url_conversions.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@implementation NSURL (Utilities)

std::string GetRegistry(const GURL& url) {
  if (url.host().empty() || url.HostIsIPAddress()) {
    return std::string();  // No registry.
  }

  const std::optional<size_t> registry_length =
      net::registry_controlled_domains::GetRegistry(
          url, net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)
          .transform(&std::string_view::size);

  if (!registry_length.has_value() || *registry_length == 0 ||
      *registry_length >= url.GetHost().length()) {
    return std::string();  // No registry.
  }
  return std::string(url.host(), url.host().length() - *registry_length,
                     *registry_length);
}

+ (NSURL*)URLFromIDNString:(NSString*)idnString {
  return net::NSURLWithGURL(GURL(base::SysNSStringToUTF8(idnString)));
}

- (NSString*)brave_registry {
  return base::SysUTF8ToNSString(GetRegistry(net::GURLWithNSURL(self)));
}

- (NSString*)brave_domainAndRegistry {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      net::GURLWithNSURL(self),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

- (NSString*)brave_domainAndRegistryExcludingPrivateRegistries {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      net::GURLWithNSURL(self),
      net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

- (bool)brave_isHostIPAddress {
  return net::GURLWithNSURL(self).HostIsIPAddress();
}

- (NSString*)brave_hostPatternString {
  GURL gurl = net::GURLWithNSURL(self);
  const auto pattern = content_settings::CreateHostPattern(gurl);
  if (!pattern.IsValid()) {
    return @"";
  }
  return base::SysUTF8ToNSString(pattern.GetHost());
}

- (NSString*)brave_domainPatternString {
  GURL gurl = net::GURLWithNSURL(self);
  const auto pattern = content_settings::CreateDomainPattern(gurl);
  if (!pattern.IsValid()) {
    return @"";
  }
  return base::SysUTF8ToNSString(pattern.GetHost());
}

- (NSString*)brave_spec {
  return base::SysUTF8ToNSString(net::GURLWithNSURL(self).spec());
}

- (NSURL*)brave_addingQueryParameter:(NSString*)key value:(NSString*)value {
  GURL gurl_ = net::AppendQueryParameter(net::GURLWithNSURL(self),
                                         base::SysNSStringToUTF8(key),
                                         base::SysNSStringToUTF8(value));
  return net::NSURLWithGURL(gurl_);
}

- (NSURL*)brave_replacingQueryParameter:(NSString*)key value:(NSString*)value {
  GURL gurl_ = net::AppendOrReplaceQueryParameter(
      net::GURLWithNSURL(self), base::SysNSStringToUTF8(key),
      base::SysNSStringToUTF8(value));
  return net::NSURLWithGURL(gurl_);
}

- (nullable NSString*)brave_valueForQueryParameter:(NSString*)key {
  std::string result;
  bool success = net::GetValueForKeyInQuery(
      net::GURLWithNSURL(self), base::SysNSStringToUTF8(key), &result);
  if (success) {
    return base::SysUTF8ToNSString(result);
  }
  return nullptr;
}

- (bool)brave_hasScheme:(NSString*)scheme {
  return net::GURLWithNSURL(self).SchemeIs(
      base::SysNSStringToUTF8([scheme lowercaseString]));
}
@end

@implementation NSURL (StaticUtilities)

std::string GetRegistryFromHost(const std::string& host) {
  if (host.empty() || url::HostIsIPAddress(host)) {
    return std::string();  // No registry.
  }

  std::optional<std::string_view> registry =
      net::registry_controlled_domains::PermissiveGetHostRegistry(
          host, net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (!registry.has_value() || registry->empty()) {
    return std::string();  // No registry.
  }
  return std::string(*registry);
}

+ (NSString*)brave_registryFromHost:(NSString*)host {
  return base::SysUTF8ToNSString(
      GetRegistryFromHost(base::SysNSStringToUTF8(host)));
}

+ (NSString*)brave_domainAndRegistryFromHost:(NSString*)host {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      base::SysNSStringToUTF8(host),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

+ (NSString*)brave_domainAndRegistryExcludingPrivateRegistriesFromHost:
    (NSString*)host {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      base::SysNSStringToUTF8(host),
      net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

+ (bool)brave_isHostIPAddressFromHost:(NSString*)host {
  return url::HostIsIPAddress(base::SysNSStringToUTF8(host));
}
@end
