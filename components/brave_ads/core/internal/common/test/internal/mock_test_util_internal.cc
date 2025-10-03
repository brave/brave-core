/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/mock_test_util_internal.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_storage_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_value_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_storage_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_value_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"

namespace brave_ads::test {

void MockFlags() {
  CHECK(GlobalState::HasInstance());

  GlobalState::GetInstance()->Flags() = *BuildFlags();

  if (!DidAppendCommandLineSwitches()) {
    // Force the test environment to staging if we did not append command-line
    // switches in `SetUpMocks`, or if the test environment does not support
    // passing command-line switches.
    GlobalState::GetInstance()->Flags().environment_type =
        mojom::EnvironmentType::kStaging;
  }
}

void MockContentSettings() {
  GlobalState::GetInstance()->ContentSettings().allow_javascript = true;
}

void MockAdsClientNotifierAddObserver(AdsClientMock& ads_client_mock,
                                      TestBase& test_base) {
  ON_CALL(ads_client_mock, AddObserver)
      .WillByDefault([&test_base](AdsClientNotifierObserver* const observer) {
        CHECK(observer);
        test_base.AddObserver(observer);
      });
}

void MockNotifyPendingObservers(AdsClientMock& ads_client_mock,
                                TestBase& test_base) {
  ON_CALL(ads_client_mock, NotifyPendingObservers)
      .WillByDefault([&test_base]() { test_base.NotifyPendingObservers(); });
}

void MockShowNotificationAd(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ShowNotificationAd)
      .WillByDefault([](const NotificationAdInfo& ad) {
        // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple
        // reminders from push notification ads.
        const bool is_reminder_valid = !ad.placement_id.empty() &&
                                       !ad.title.empty() && !ad.body.empty() &&
                                       ad.target_url.is_valid();

        CHECK(ad.IsValid() || is_reminder_valid);
      });
}

void MockCloseNotificationAd(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, CloseNotificationAd)
      .WillByDefault([](const std::string& placement_id) {
        CHECK(!placement_id.empty());
      });
}

void MockSave(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, Save)
      .WillByDefault([](const std::string& /*name*/,
                        const std::string& /*value*/, SaveCallback callback) {
        std::move(callback).Run(/*success=*/true);
      });
}

void MockLoad(AdsClientMock& ads_client_mock,
              const base::FilePath& profile_path) {
  ON_CALL(ads_client_mock, Load)
      .WillByDefault(
          [&profile_path](const std::string& name, LoadCallback callback) {
            base::FilePath path = profile_path.AppendASCII(name);
            if (!base::PathExists(path)) {
              // If path does not exist attempt to load the file from the test
              // data path.
              path = DataPath().AppendASCII(name);
            }

            std::string value;
            if (!base::ReadFileToString(path, &value)) {
              return std::move(callback).Run(/*value=*/std::nullopt);
            }

            std::move(callback).Run(value);
          });
}

void MockLoadResourceComponent(AdsClientMock& ads_client_mock,
                               const base::FilePath& profile_path) {
  ON_CALL(ads_client_mock, LoadResourceComponent)
      .WillByDefault([&profile_path](const std::string& id, int /*version*/,
                                     LoadFileCallback callback) {
        base::FilePath path = profile_path.AppendASCII(id);

        if (!base::PathExists(path)) {
          // If path does not exist attempt to load the file from the test
          // resource components data path.
          path = ResourceComponentsDataPath().AppendASCII(id);
        }

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      });
}

void MockFindProfilePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, FindProfilePref)
      .WillByDefault([](const std::string& path) -> bool {
        return FindProfilePref(path);
      });
}

void MockGetProfilePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, GetProfilePref)
      .WillByDefault([](const std::string& path) -> std::optional<base::Value> {
        return GetProfilePrefValue(path);
      });
}

void MockSetProfilePref(const AdsClientMock& ads_client_mock,
                        TestBase& test_base) {
  ON_CALL(ads_client_mock, SetProfilePref)
      .WillByDefault([&test_base](const std::string& path, base::Value value) {
        SetProfilePrefValue(path, std::move(value));
        test_base.NotifyPrefDidChange(path);
      });
}

void MockClearProfilePref(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ClearProfilePref)
      .WillByDefault(
          [](const std::string& path) { ClearProfilePrefValue(path); });
}

void MockHasProfilePrefPath(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, HasProfilePrefPath)
      .WillByDefault([](const std::string& path) -> bool {
        return HasProfilePrefPathValue(path);
      });
}

void MockFindLocalStatePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, FindLocalStatePref)
      .WillByDefault([](const std::string& path) -> bool {
        return FindLocalStatePref(path);
      });
}

void MockGetLocalStatePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, GetLocalStatePref)
      .WillByDefault([](const std::string& path) -> std::optional<base::Value> {
        return GetLocalStatePrefValue(path);
      });
}

void MockSetLocalStatePref(const AdsClientMock& ads_client_mock,
                           TestBase& test_base) {
  ON_CALL(ads_client_mock, SetLocalStatePref)
      .WillByDefault([&test_base](const std::string& path, base::Value value) {
        SetLocalStatePrefValue(path, std::move(value));
        test_base.NotifyPrefDidChange(path);
      });
}

void MockClearLocalStatePref(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ClearLocalStatePref)
      .WillByDefault(
          [](const std::string& path) { ClearLocalStatePrefValue(path); });
}

void MockHasLocalStatePrefPath(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, HasLocalStatePrefPath)
      .WillByDefault([](const std::string& path) -> bool {
        return HasLocalStatePrefPathValue(path);
      });
}

}  // namespace brave_ads::test
