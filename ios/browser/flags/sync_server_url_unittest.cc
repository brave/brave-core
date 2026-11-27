// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/flags/sync_server_url.h"

#include <string>

#include "base/command_line.h"
#include "base/values.h"
#include "brave/components/brave_sync/buildflags.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/command_line_switches.h"
#include "components/webui/flags/pref_service_flags_storage.h"
#include "ios/chrome/browser/flags/about_flags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {
namespace {

constexpr char kCustomUrl[] = "https://sync.example.com:8443/prefix/v2";

class SyncServerUrlTest : public testing::Test {
 protected:
  SyncServerUrlTest() {
    flags_ui::PrefServiceFlagsStorage::RegisterPrefs(prefs_.registry());
  }

  void EnterUrl(const std::string& value) {
    SetSyncServerUrlFlag(
        &storage_,
        base::ListValue().Append(kSyncServerUrlFlagName).Append(value));
  }

  std::string ApplyOnStartup() {
    // Use a new storage and command line to model a browser relaunch.
    flags_ui::PrefServiceFlagsStorage startup_storage(&prefs_);
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    ConvertFlagsToSwitches(&startup_storage, &command_line);
    // The custom URL must be absent when startup captures the command line.
    EXPECT_FALSE(command_line.HasSwitch(syncer::kSyncServiceURL));
    EXPECT_EQ(command_line.GetCommandLineString().find(kCustomUrl),
              std::string::npos);
    ApplySyncServerUrlFromFlags(startup_storage, command_line);
    return command_line.GetSwitchValueASCII(syncer::kSyncServiceURL);
  }

  TestingPrefServiceSimple prefs_;
  flags_ui::PrefServiceFlagsStorage storage_{&prefs_};
};

TEST_F(SyncServerUrlTest, UsesBraveEndpointByDefault) {
  EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));
}

TEST_F(SyncServerUrlTest, PersistsFullUrlAndAppliesOnlyWhenEnabled) {
  EnterUrl(kCustomUrl);
  EXPECT_EQ(storage_.GetStringFlag(kSyncServerUrlFlagName), kCustomUrl);
  EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));

  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, true);
  EXPECT_EQ(ApplyOnStartup(), kCustomUrl);

  EnterUrl("https://other.example.com/v2/");
  EXPECT_EQ(ApplyOnStartup(), "https://other.example.com/v2/");

  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, false);
  EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));
}

TEST_F(SyncServerUrlTest, EmptyInputAndResetRestoreDefault) {
  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, true);
  EnterUrl("");
  EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));

  EnterUrl(kCustomUrl);
  ResetAllFlags(&storage_);
  EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));
}

TEST_F(SyncServerUrlTest, CanonicalizesUrlsAndAllowsHttpOnLoopback) {
  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, true);
  EnterUrl("  HTTPS://SYNC.EXAMPLE.COM:8443/prefix/v2  ");
  EXPECT_EQ(ApplyOnStartup(), kCustomUrl);

  for (const char* loopback_url :
       {"http://localhost:8295/v2", "http://127.0.0.1:8295/v2",
        "http://[::1]:8295/v2"}) {
    EnterUrl(loopback_url);
    EXPECT_EQ(ApplyOnStartup(), loopback_url);
  }
}

TEST_F(SyncServerUrlTest, RejectsInsecureAndMalformedValues) {
  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, true);
  for (const char* invalid_url :
       {" ", "sync.example.com/v2", "https:sync.example.com/v2",
        "https:sync.example.com/a://b", "https:///sync.example.com/v2",
        "https://sync.example.com:65536/v2", "file:///tmp/v2",
        "http://sync.example.com/v2", "http://localhost.example.com/v2",
        "http://192.168.1.100:8295/v2",
        "https://user:password@sync.example.com/v2",
        "https://@sync.example.com/v2",
        "https://sync.example.com/v2?auth=token",
        "https://sync.example.com/v2#fragment", "https://sync.example.com\\v2",
        "https://sync.exa\tmple.com/v2"}) {
    SCOPED_TRACE(invalid_url);
    EnterUrl(invalid_url);
    EXPECT_EQ(ApplyOnStartup(), BUILDFLAG(BRAVE_SYNC_ENDPOINT));
  }
}

TEST_F(SyncServerUrlTest, PreservesExplicitLaunchOverride) {
  SetFeatureEntryEnabled(&storage_, kSyncServerUrlFlagName, true);
  EnterUrl(kCustomUrl);

  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(syncer::kSyncServiceURL,
                                 "https://launch.example.com/v2");
  ApplySyncServerUrlFromFlags(storage_, command_line);
  EXPECT_EQ(command_line.GetSwitchValueASCII(syncer::kSyncServiceURL),
            "https://launch.example.com/v2");
}

TEST_F(SyncServerUrlTest, IgnoresMalformedOrUnrelatedMessages) {
  EnterUrl(kCustomUrl);
  SetSyncServerUrlFlag(&storage_, base::ListValue());
  SetSyncServerUrlFlag(&storage_,
                       base::ListValue().Append("other").Append("x"));
  SetSyncServerUrlFlag(
      &storage_, base::ListValue().Append(kSyncServerUrlFlagName).Append(1));
  EXPECT_EQ(storage_.GetStringFlag(kSyncServerUrlFlagName), kCustomUrl);
  EXPECT_TRUE(storage_.GetStringFlag("other").empty());
}

}  // namespace
}  // namespace brave_sync
