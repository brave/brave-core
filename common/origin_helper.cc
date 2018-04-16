/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "brave/common/origin_helper.h"

#include <string>

#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace brave {

bool IsSameTLDPlus1(const GURL& url1, const GURL& url2,
                    bool *result) {
  // Fetch the eTLD+1 of both origins.
  const std::string origin1_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url1,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  const std::string origin2_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url2,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (origin1_etldp1.empty()) {
    return false;
  }
  if (origin2_etldp1.empty()) {
    return false;
  }
  *result = origin1_etldp1 == origin2_etldp1;
  return true;
}

}  // namespace brave
