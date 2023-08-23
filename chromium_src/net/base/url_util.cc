// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/net/base/url_util.cc"

#include <iostream>
#include <string>

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon_ip.h"

namespace net {

namespace {
constexpr char kOnionDomain[] = "onion";
}  // namespace

std::string URLToEphemeralStorageDomain(const GURL& url) {
  std::string domain = registry_controlled_domains::GetDomainAndRegistry(
      url, registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  // GetDomainAndRegistry might return an empty string if this host is an IP
  // address or a file URL.
  if (domain.empty())
    domain = url::Origin::Create(url).Serialize();

  return domain;
}

bool EphemeralStorageOriginUtils::CanUseNonceForEphemeralStorageKeying(
    const url::Origin& origin) {
  return origin.opaque() && !origin.nonce_->raw_token().is_empty();
}

const base::UnguessableToken&
EphemeralStorageOriginUtils::GetNonceForEphemeralStorageKeying(
    const url::Origin& origin) {
  CHECK(CanUseNonceForEphemeralStorageKeying(origin));
  return origin.nonce_->raw_token();
}

bool IsOnion(const GURL& url) {
  return url.DomainIs(kOnionDomain);
}

bool IsLocalhostOrOnion(const GURL& url) {
  return IsLocalhost(url) || IsOnion(url);
}

}  // namespace net
