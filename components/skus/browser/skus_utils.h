/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_

#include <string>

class PrefRegistrySimple;
class PrefService;

namespace skus {

constexpr char kEnvProduction[] = "production";
constexpr char kEnvStaging[] = "staging";
constexpr char kEnvDevelopment[] = "development";

std::string GetDefaultEnvironment();
std::string GetDomain(const std::string& prefix, const std::string& domain);
std::string GetEnvironmentForDomain(const std::string& domain);
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
void MigrateSkusSettings(PrefService* profile_prefs, PrefService* local_prefs);
}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_UTILS_H_
