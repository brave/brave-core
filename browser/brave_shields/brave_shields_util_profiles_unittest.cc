// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
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
  BraveShieldsUtilProfilesTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveShieldsUtilProfilesTest() override = default;

  TestingProfile* regular_profile() { return &profile_; }
  ScopedIncognitoProfile incognito_profile() {
    return ScopedIncognitoProfile(regular_profile()->GetOffTheRecordProfile(
        Profile::OTRProfileID::PrimaryID(),
        /*create_if_needed=*/true));
  }

  HostContentSettingsMap* hcsm(Profile* profile) {
    return HostContentSettingsMapFactory::GetForProfile(profile);
  }

  HostContentSettingsMap* hcsm(ScopedIncognitoProfile& profile) {
    return hcsm(profile.get());
  }

  const GURL test_url{"https://example.com"};

  template <typename ValueT, typename SetFn, typename GetFn>
  void RunTest(base::span<const std::pair<ValueT, ValueT>> cases,
               SetFn setter,
               GetFn getter) {
    testing::Message issues;
    for (const auto& [value, expect] : cases) {
      setter(hcsm(regular_profile()), value);
      const auto get = getter(hcsm(regular_profile()));
      if (expect != get) {
        issues << "Regular profile: Set " << value << " Get " << get
               << " Expected " << expect << "\n";
      }

      auto incognito = incognito_profile();
      // Now change value for incognito and expect that values are not depended
      // on the regular profile.
      for (const auto& [ivalue, iexpect] : cases) {
        setter(hcsm(incognito), ivalue);
        const auto iget = getter(hcsm(incognito));
        if (iexpect != iget) {
          issues << "Incognito profile: Set " << ivalue << " Get " << iget
                 << " Expected " << iexpect << " Regular profile value "
                 << value << "\n";
        }
      }
    }
    const auto message = issues.GetString();
    EXPECT_TRUE(message.empty()) << message;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  ScopedTestingLocalState local_state_;
};

TEST_F(BraveShieldsUtilProfilesTest, SetBraveShieldsEnabled) {
  constexpr std::pair<bool, bool> kExpects[] = {{true, true}, {false, false}};

  auto set = [this](HostContentSettingsMap* map, bool value) {
    SetBraveShieldsEnabled(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetBraveShieldsEnabled(map, test_url);
  };

  RunTest<bool>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetAdControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {{ALLOW, ALLOW},
                                                              {BLOCK, BLOCK}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetAdControlType(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetAdControlType(map, test_url);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetCosmeticFilteringControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetCosmeticFilteringControlType(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetCosmeticFilteringControlType(map, test_url);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetCookieControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetCookieControlType(map, regular_profile()->GetPrefs(), value, test_url);
  };

  auto cookie_settings =
      CookieSettingsFactory::GetForProfile(regular_profile());

  auto get = [this, cookie_settings](HostContentSettingsMap* map) {
    return GetCookieControlType(map, cookie_settings.get(), test_url);
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
    SetFingerprintingControlType(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetFingerprintingControlType(map, test_url);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetHttpsUpgradeControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {BLOCK_THIRD_PARTY, BLOCK_THIRD_PARTY}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetHttpsUpgradeControlType(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetHttpsUpgradeControlType(map, test_url);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetNoScriptControlType) {
  constexpr std::pair<ControlType, ControlType> kExpects[] = {
      {ALLOW, ALLOW}, {BLOCK, BLOCK}, {DEFAULT, BLOCK}};

  auto set = [this](HostContentSettingsMap* map, ControlType value) {
    SetNoScriptControlType(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetNoScriptControlType(map, test_url);
  };

  RunTest<ControlType>(kExpects, std::move(set), std::move(get));
}

TEST_F(BraveShieldsUtilProfilesTest, SetForgetFirstPartyStorageEnabled) {
  constexpr std::pair<bool, bool> kExpects[] = {{true, true}, {false, false}};

  auto set = [this](HostContentSettingsMap* map, bool value) {
    SetForgetFirstPartyStorageEnabled(map, value, test_url);
  };
  auto get = [this](HostContentSettingsMap* map) {
    return GetForgetFirstPartyStorageEnabled(map, test_url);
  };

  RunTest<bool>(kExpects, std::move(set), std::move(get));
}

}  // namespace brave_shields
