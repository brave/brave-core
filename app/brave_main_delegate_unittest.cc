/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <string>

#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "brave/base/buildflag_config.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/brave_sync/buildflags.h"
#include "brave/components/update_client/buildflags.h"
#include "brave/components/variations/buildflags.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/embedder_support/switches.h"
#include "components/sync/base/command_line_switches.h"
#include "components/variations/variations_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";
constexpr char kUpdaterProdEndpoint[] = "https://go-prod.com";
constexpr char kUpdaterDevEndpoint[] = "https://go-dev.com";
constexpr char kBraveSyncEndpoint[] = "https://sync.com";
constexpr char kVariationsServerURL[] = "https://variations.com";

TEST(BraveMainDelegateUnitTest, DefaultCommandLineOverrides) {
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(UPDATER_PROD_ENDPOINT, kUpdaterProdEndpoint)
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(BRAVE_SYNC_ENDPOINT, kBraveSyncEndpoint)
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(BRAVE_VARIATIONS_SERVER_URL,
                                   kVariationsServerURL)

  // These overrides are in different methods because the component updater
  // override needs to be called later during startup when the FeatureList has
  // been initialized
  BraveMainDelegate::AppendCommandLineOptions();
  BraveMainDelegate::OverrideComponentUpdaterURL();

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  ASSERT_STREQ(
      base::StrCat({"url-source=", kUpdaterProdEndpoint}).c_str(),
      command_line.GetSwitchValueASCII(switches::kComponentUpdater).c_str());
  ASSERT_STREQ(
      kBraveSyncEndpoint,
      command_line.GetSwitchValueASCII(syncer::kSyncServiceURL).c_str());
  ASSERT_STREQ(
      kBraveOriginTrialsPublicKey,
      command_line.GetSwitchValueASCII(embedder_support::kOriginTrialPublicKey)
          .c_str());
  ASSERT_STREQ(
      kVariationsServerURL,
      command_line
          .GetSwitchValueASCII(variations::switches::kVariationsServerURL)
          .c_str());
  ASSERT_STREQ(kVariationsServerURL,
               command_line
                   .GetSwitchValueASCII(
                       variations::switches::kVariationsInsecureServerURL)
                   .c_str());
}

TEST(BraveMainDelegateUnitTest, OverrideSwitchFromCommandLine) {
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(UPDATER_PROD_ENDPOINT, kUpdaterProdEndpoint)
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(BRAVE_SYNC_ENDPOINT, kBraveSyncEndpoint)
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(BRAVE_VARIATIONS_SERVER_URL,
                                   kVariationsServerURL)

  constexpr char kOverrideUpdaterProdEndpoint[] =
      "https://go-prod-override.com";
  constexpr char kOverrideSyncUrl[] = "https://sync-override.com";
  constexpr char kOverrideVariationsServerURL[] =
      "https://variations.com-override";
  constexpr char kOverrideInsecureVariationsServerURL[] =
      "https://variations-override.com";
  constexpr char kOverrideOriginTrialPublicKey[] = "public_key-override";

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitchASCII(switches::kComponentUpdater,
                                 kOverrideUpdaterProdEndpoint);
  command_line.AppendSwitchASCII(syncer::kSyncServiceURL, kOverrideSyncUrl);
  command_line.AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                 kOverrideOriginTrialPublicKey);
  command_line.AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                 kOverrideVariationsServerURL);
  command_line.AppendSwitchASCII(
      variations::switches::kVariationsInsecureServerURL,
      kOverrideInsecureVariationsServerURL);

  // These overrides are in different methods because the component updater
  // override needs to be called later during startup when the FeatureList has
  // been initialized
  BraveMainDelegate::AppendCommandLineOptions();
  BraveMainDelegate::OverrideComponentUpdaterURL();

  ASSERT_STREQ(
      base::StrCat({kOverrideUpdaterProdEndpoint, ",",
                    "url-source=", kUpdaterProdEndpoint})
          .c_str(),
      command_line.GetSwitchValueASCII(switches::kComponentUpdater).c_str());
  ASSERT_STREQ(
      kOverrideSyncUrl,
      command_line.GetSwitchValueASCII(syncer::kSyncServiceURL).c_str());
  ASSERT_STREQ(
      kOverrideOriginTrialPublicKey,
      command_line.GetSwitchValueASCII(embedder_support::kOriginTrialPublicKey)
          .c_str());
  ASSERT_STREQ(
      kOverrideVariationsServerURL,
      command_line
          .GetSwitchValueASCII(variations::switches::kVariationsServerURL)
          .c_str());
  ASSERT_STREQ(kOverrideInsecureVariationsServerURL,
               command_line
                   .GetSwitchValueASCII(
                       variations::switches::kVariationsInsecureServerURL)
                   .c_str());
}

TEST(BraveMainDelegateUnitTest, UseDevUpdaterEndpoint) {
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(UPDATER_PROD_ENDPOINT, kUpdaterProdEndpoint)
  SCOPED_BUILDFLAG_CONFIG_OVERRIDE(UPDATER_DEV_ENDPOINT, kUpdaterDevEndpoint)

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitch(brave_component_updater::kUseGoUpdateDev);

  BraveMainDelegate::OverrideComponentUpdaterURL();

  ASSERT_STREQ(
      base::StrCat({"url-source=", kUpdaterDevEndpoint}).c_str(),
      command_line.GetSwitchValueASCII(switches::kComponentUpdater).c_str());
}
