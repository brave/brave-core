/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/util.h"

#include "brave/brave_domains/service_domains.h"

namespace web_discovery {

namespace {
constexpr char kCollectorHostPrefix[] = "collector.wdp";
}  // namespace

std::string GetCollectorHost() {
  return brave_domains::GetServicesDomain(kCollectorHostPrefix);
}

}  // namespace web_discovery
