/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UTILS_H_
#define BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UTILS_H_

class GURL;
class PrefService;

namespace unstoppable_domains {

bool IsUnstoppableDomainsTLD(const GURL& url);
bool IsUnstoppableDomainsEnabled();
bool IsResolveMethodAsk(PrefService* local_state);
bool IsResolveMethodDoH(PrefService* local_state);

}  // namespace unstoppable_domains

#endif  // BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UTILS_H_
