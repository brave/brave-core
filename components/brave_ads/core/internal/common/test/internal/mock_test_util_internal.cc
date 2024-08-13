/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/mock_test_util_internal.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/current_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_value_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_value_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/database/database.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"

namespace brave_ads::test {

namespace {

using AdEventHistoryMap =
    base::flat_map</*type_id*/ std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map</*uuid*/ std::string, AdEventHistoryMap>;

AdEventMap& AdEventCache() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

}  // namespace

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

void MockAdsClientNotifierAddObserver(AdsClientMock& ads_client_mock,
                                      TestBase& test_base) {
  ON_CALL(ads_client_mock, AddObserver)
      .WillByDefault(::testing::Invoke(
          [&test_base](AdsClientNotifierObserver* const observer) {
            CHECK(observer);
            test_base.AddObserver(observer);
          }));
}

void MockShowNotificationAd(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ShowNotificationAd)
      .WillByDefault(::testing::Invoke([](const NotificationAdInfo& ad) {
        // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple
        // reminders from push notification ads.
        const bool is_reminder_valid = !ad.placement_id.empty() &&
                                       !ad.title.empty() && !ad.body.empty() &&
                                       ad.target_url.is_valid();

        CHECK(ad.IsValid() || is_reminder_valid);
      }));
}

void MockCloseNotificationAd(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, CloseNotificationAd)
      .WillByDefault(::testing::Invoke([](const std::string& placement_id) {
        CHECK(!placement_id.empty());
      }));
}

void MockCacheAdEventForInstanceId(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, CacheAdEventForInstanceId)
      .WillByDefault(::testing::Invoke(
          [](const std::string& id, const std::string& ad_type,
             const std::string& confirmation_type, const base::Time time) {
            CHECK(!id.empty());
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string uuid = GetUuidForCurrentTestAndValue(id);
            const std::string type_id =
                base::StrCat({ad_type, confirmation_type});
            AdEventCache()[uuid][type_id].push_back(time);
          }));
}

void MockGetCachedAdEvents(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, GetCachedAdEvents)
      .WillByDefault(::testing::Invoke([](const std::string& ad_type,
                                          const std::string& confirmation_type)
                                           -> std::vector<base::Time> {
        CHECK(!ad_type.empty());
        CHECK(!confirmation_type.empty());

        const std::string uuid_for_current_test = GetUuidForCurrentTest();

        const std::string type_id = base::StrCat({ad_type, confirmation_type});

        std::vector<base::Time> cached_ad_events;

        for (const auto& [ad_event_uuid, ad_event_history] : AdEventCache()) {
          if (!ad_event_uuid.ends_with(
                  base::StrCat({":", uuid_for_current_test}))) {
            // Only get ad events for current test.
            continue;
          }

          for (const auto& [ad_event_type_id, ad_event_timestamps] :
               ad_event_history) {
            if (ad_event_type_id == type_id) {
              base::Extend(cached_ad_events, ad_event_timestamps);
            }
          }
        }

        return cached_ad_events;
      }));
}

void MockResetAdEventCacheForInstanceId(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ResetAdEventCacheForInstanceId)
      .WillByDefault(::testing::Invoke([](const std::string& id) {
        CHECK(!id.empty());

        const std::string uuid = GetUuidForCurrentTestAndValue(id);
        AdEventCache()[uuid].clear();
      }));
}

void MockSave(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, Save)
      .WillByDefault(::testing::Invoke([](const std::string& /*name*/,
                                          const std::string& /*value*/,
                                          SaveCallback callback) {
        std::move(callback).Run(/*success=*/true);
      }));
}

void MockLoad(AdsClientMock& ads_client_mock,
              const base::FilePath& profile_path) {
  ON_CALL(ads_client_mock, Load)
      .WillByDefault(::testing::Invoke(
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
          }));
}

void MockLoadResourceComponent(AdsClientMock& ads_client_mock,
                               const base::FilePath& profile_path) {
  ON_CALL(ads_client_mock, LoadResourceComponent)
      .WillByDefault(::testing::Invoke(
          [&profile_path](const std::string& id, const int /*version*/,
                          LoadFileCallback callback) {
            base::FilePath path = profile_path.AppendASCII(id);

            if (!base::PathExists(path)) {
              // If path does not exist attempt to load the file from the test
              // resource components data path.
              path = ResourceComponentsDataPath().AppendASCII(id);
            }

            base::File file(path, base::File::Flags::FLAG_OPEN |
                                      base::File::Flags::FLAG_READ);
            std::move(callback).Run(std::move(file));
          }));
}

void MockLoadDataResource(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, LoadDataResource)
      .WillByDefault(
          ::testing::Invoke([](const std::string& name) -> std::string {
            return MaybeReadDataResourceToString(name).value_or("");
          }));
}

void MockRunDBTransaction(AdsClientMock& ads_client_mock, Database& database) {
  ON_CALL(ads_client_mock, RunDBTransaction)
      .WillByDefault(::testing::Invoke(
          [&database](mojom::DBTransactionInfoPtr mojom_transaction,
                      RunDBTransactionCallback callback) {
            CHECK(mojom_transaction);

            mojom::DBStatementResultInfoPtr mojom_statement_result =
                database.RunTransaction(std::move(mojom_transaction));

            std::move(callback).Run(std::move(mojom_statement_result));
          }));
}

void MockGetProfilePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, GetProfilePref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) -> std::optional<base::Value> {
            return GetProfilePrefValue(path);
          }));
}

void MockSetProfilePref(const AdsClientMock& ads_client_mock,
                        TestBase& test_base) {
  ON_CALL(ads_client_mock, SetProfilePref)
      .WillByDefault(::testing::Invoke(
          [&test_base](const std::string& path, base::Value value) {
            SetProfilePrefValue(path, std::move(value));
            test_base.NotifyPrefDidChange(path);
          }));
}

void MockClearProfilePref(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ClearProfilePref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) { ClearProfilePrefValue(path); }));
}

void MockHasProfilePrefPath(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, HasProfilePrefPath)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> bool {
        return HasProfilePrefPathValue(path);
      }));
}

void MockGetLocalStatePref(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, GetLocalStatePref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) -> std::optional<base::Value> {
            return GetLocalStatePrefValue(path);
          }));
}

void MockSetLocalStatePref(const AdsClientMock& ads_client_mock,
                           TestBase& test_base) {
  ON_CALL(ads_client_mock, SetLocalStatePref)
      .WillByDefault(::testing::Invoke(
          [&test_base](const std::string& path, base::Value value) {
            SetLocalStatePrefValue(path, std::move(value));
            test_base.NotifyPrefDidChange(path);
          }));
}

void MockClearLocalStatePref(AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, ClearLocalStatePref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) { ClearLocalStatePrefValue(path); }));
}

void MockHasLocalStatePrefPath(const AdsClientMock& ads_client_mock) {
  ON_CALL(ads_client_mock, HasLocalStatePrefPath)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> bool {
        return HasLocalStatePrefPathValue(path);
      }));
}

}  // namespace brave_ads::test
