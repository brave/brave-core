// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

using ScopedIncognitoProfile =
    std::unique_ptr<Profile, decltype([](Profile* p) {
                      ASSERT_TRUE(!!p->IsOffTheRecord());
                      p->GetOriginalProfile()->DestroyOffTheRecordProfile(p);
                    })>;

testing::Message& operator<<(testing::Message& m,
                             brave_shields::ControlType c) {
  return m << brave_shields::ControlTypeToString(c);
}

}  // namespace

namespace brave_shields {

class BraveShieldsUtilProfilesTest : public testing::Test {
 public:
  BraveShieldsUtilProfilesTest() = default;
  ~BraveShieldsUtilProfilesTest() override = default;

  TestingProfile* regular_profile() { return &profile_; }
  ScopedIncognitoProfile incognito_profile() {
    return ScopedIncognitoProfile(regular_profile()->GetOffTheRecordProfile(
        Profile::OTRProfileID::PrimaryID(),
        /*create_if_needed=*/true));
  }

  template <typename ValueT>
  void RunTest(base::span<const std::pair<ValueT, ValueT>> cases,
               base::FunctionRef<void(HostContentSettingsMap*, ValueT)> setter,
               base::FunctionRef<ValueT(HostContentSettingsMap*)> getter) {
    for (const auto& [value, expect] : cases) {
      auto* regular_map =
          HostContentSettingsMapFactory::GetForProfile(regular_profile());
      setter(regular_map, value);
      const auto get = getter(regular_map);
      if (expect != get) {
        ADD_FAILURE() << "Regular profile: Set " << value << " Get " << get
                      << " Expected " << expect << "\n";
      }

      auto incognito = incognito_profile();
      // Now change value for incognito and expect that values are not depended
      // on the regular profile.
      for (const auto& [ivalue, iexpect] : cases) {
        auto* incognito_map =
            HostContentSettingsMapFactory::GetForProfile(incognito.get());

        setter(incognito_map, ivalue);
        const auto iget = getter(incognito_map);
        if (iexpect != iget) {
          ADD_FAILURE() << "Incognito profile: Set " << ivalue << " Get "
                        << iget << " Expected " << iexpect
                        << " Regular profile value " << value << "\n";
        }
      }
    }
  }

  const GURL kTestUrl{"https://example.com"};

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveShieldsUtilProfilesTest, SetBraveShieldsEnabled) {
  constexpr std::pair<bool, bool> kExpects[] = {{true, true}, {false, false}};

  auto set = [this](HostContentSettingsMap* map, bool value) {
    SetBraveShieldsEnabled(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetBraveShieldsEnabled(map, kTestUrl);
  };

  RunTest<bool>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetAdControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {{ALLOW, ALLOW},
                                                              {BLOCK, BLOCK}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetAdControlType(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetAdControlType(map, kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetCosmeticFilteringControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetCosmeticFilteringControlType(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetCosmeticFilteringControlType(map, kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetCookieControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetCookieControlType(map, regular_profile()->GetPrefs(), value, kTestUrl);
  };

  auto get = [this](HostContentSettingsMap* map) {
    return GetCookieControlType(
        map, CookieSettingsFactory::GetForProfile(regular_profile()).get(),
        kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetFingerprintingControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW},
      {BLOCK, DEFAULT},
      {BLOCK_THIRD_PARTY, DEFAULT},
      {DEFAULT, DEFAULT}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetFingerprintingControlType(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetFingerprintingControlType(map, kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetHttpsUpgradeControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetHttpsUpgradeControlType(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetHttpsUpgradeControlType(map, kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetNoScriptControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {DEFAULT, BLOCK}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetNoScriptControlType(map, value, kTestUrl);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetNoScriptControlType(map, kTestUrl);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

}  // namespace brave_shields
