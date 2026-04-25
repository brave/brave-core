/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_state.h"

#include "net/base/network_anonymization_key.h"
#include "net/base/schemeful_site.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

net::NetworkAnonymizationKey CreateNetworkAnonymizationKey(
    std::string_view top_frame_url) {
  net::SchemefulSite schemeful_site(url::Origin::Create(GURL(top_frame_url)));
  return net::NetworkAnonymizationKey::CreateFromFrameSite(schemeful_site,
                                                           schemeful_site);
}

}  // namespace

#define GetSSLUpgradeDecision(HOST, IS_TOP_LEVEL_NAV)              \
  GetSSLUpgradeDecision(CreateNetworkAnonymizationKey(HOST), HOST, \
                        IS_TOP_LEVEL_NAV)

#include <net/http/transport_security_state_unittest.cc>
#undef GetSSLUpgradeDecision
