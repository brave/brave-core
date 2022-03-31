/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_

#include <string>

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace skus {

constexpr char kEnvProduction[] = "production";
constexpr char kEnvStaging[] = "staging";
constexpr char kEnvDevelopment[] = "development";

std::string GetEnvironment();
std::string GetDomain(std::string prefix);
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_
