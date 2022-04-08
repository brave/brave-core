/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_ADBLOCK_DOMAIN_RESOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_ADBLOCK_DOMAIN_RESOLVER_H_

#include <stdint.h>

namespace brave_shields {

void AdBlockServiceDomainResolver(const char* host,
                                  uint32_t* start,
                                  uint32_t* end);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_ADBLOCK_DOMAIN_RESOLVER_H_
