/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_CONSTANTS_H_
#define BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_CONSTANTS_H_

namespace unstoppable_domains {

constexpr char kCryptoDomain[] = ".crypto";
constexpr char kDoHResolver[] =
    "https://resolver.unstoppable.io/dns-query{?brave_UD}";

enum class ResolveMethodTypes {
  ASK,
  DISABLED,
  DNS_OVER_HTTPS,
};

}  // namespace unstoppable_domains

#endif  // BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_CONSTANTS_H_
