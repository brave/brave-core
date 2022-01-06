/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/rewards_sync_scoped_persistent_pref_store.h"

#include <utility>

#include "base/test/task_environment.h"
#include "components/prefs/pref_store_observer_mock.h"
#include "components/prefs/testing_pref_store.h"
#include "testing/gtest/include/gtest/gtest.h"

// clang-format off
// NOLINTNEXTLINE
// npm run test -- brave_unit_tests --filter=RewardsSyncScopedPersistentPrefStore*
// clang-format on

using brave::RewardsSyncScopedPersistentPrefStore;
using testing::Test;
using testing::TestParamInfo;
using testing::Values;
using testing::WithParamInterface;

namespace {
const char kSyncBookmarks[] =
    "sync.bookmarks";  // mapped key (original location)
const char kBraveRewardsSyncBookmarks[] =
    "brave.rewards.sync.bookmarks";  // kSyncBookmarks' mapped location

const char kSyncPasswords[] =
    "sync.passwords";  // mapped key (original location)
const char kBraveRewardsSyncPasswords[] =
    "brave.rewards.sync.passwords";  // kSyncPasswords' mapped location

const char kBraveRewardsACEnabled[] =
    "brave.rewards.ac.enabled";  // in-scope key (in brave.rewards)

const char kBraveRewardsExternalWalletType[] =
    "brave.rewards.external_wallet_type";  // in-scope key (in brave.rewards)

const char kBraveNewTabPageHideAllWidgets[] =
    "brave.new_tab_page.hide_all_widgets";  // out-of-scope key (not in
                                            // brave.rewards)

const char kBrowserHasSeenWelcomePage[] =
    "browser.has_seen_welcome_page";  // out-of-scope key (not in brave.rewards)

const char kBrave[] = "brave";
const char kBraveRewards[] = "brave.rewards";
const char kBraveRewardsSync[] = "brave.rewards.sync";
const char kSync[] = "sync";
}  // namespace

class RewardsSyncScopedPersistentPrefStoreTest : public Test {
 protected:
  RewardsSyncScopedPersistentPrefStoreTest()
      : underlay_(new TestingPrefStore),
        overlay_(new RewardsSyncScopedPersistentPrefStore(underlay_.get())) {}

  ~RewardsSyncScopedPersistentPrefStoreTest() override {}

  base::test::TaskEnvironment task_environment_;
  scoped_refptr<TestingPrefStore> underlay_;
  scoped_refptr<RewardsSyncScopedPersistentPrefStore> overlay_;
};

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, Observer) {
  PrefStoreObserverMock obs;
  overlay_->AddObserver(&obs);

  EXPECT_TRUE(overlay_->HasObservers());

  // Check that underlay SetValue() is reported for mapped key.
  underlay_->SetValue(kBraveRewardsSyncBookmarks,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kSyncBookmarks);

  // Check that overlay SetValue() is reported for mapped key.
  overlay_->SetValue(kSyncPasswords, std::make_unique<base::Value>(false),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kSyncPasswords);

  // Check that underlay SetValue() is reported for in-scope key.
  underlay_->SetValue(kBraveRewardsACEnabled,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kBraveRewardsACEnabled);

  // Check that overlay SetValue() is reported for in-scope key.
  overlay_->SetValue(kBraveRewardsExternalWalletType,
                     std::make_unique<base::Value>("uphold"),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kBraveRewardsExternalWalletType);

  // Check that underlay SetValue() is not reported for out-of-scope key.
  underlay_->SetValue(kBraveNewTabPageHideAllWidgets,
                      std::make_unique<base::Value>(false),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // "Check that overlay SetValue() is not reported for out-of-scope key."
  // is not a valid test case (SetValue() doesn't even succeed in this case).

  // Check that underlay SetValueSilently() respects silence for mapped key.
  underlay_->SetValueSilently(kBraveRewardsSyncBookmarks,
                              std::make_unique<base::Value>(false),
                              WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // Check that overlay SetValueSilently() respects silence for mapped key.
  overlay_->SetValueSilently(kSyncPasswords,
                             std::make_unique<base::Value>(true),
                             WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // Check that underlay SetValueSilently() respects silence for in-scope key.
  underlay_->SetValueSilently(kBraveRewardsACEnabled,
                              std::make_unique<base::Value>(false),
                              WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // Check that overlay SetValueSilently() respects silence for in-scope key.
  overlay_->SetValueSilently(kBraveRewardsExternalWalletType,
                             std::make_unique<base::Value>("gemini"),
                             WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // Check that underlay SetValueSilently() is not reported for out-of-scope
  // key (even SetValue() is not reported for out-of-scope keys, since they're
  // out of scope).
  underlay_->SetValueSilently(kBraveNewTabPageHideAllWidgets,
                              std::make_unique<base::Value>(true),
                              WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // "Check that overlay SetValueSilently() is not reported for out-of-scope
  // key." is not a valid test case (SetValueSilently() doesn't even succeed in
  // this case).

  // Check that underlay RemoveValue() is reported for mapped key.
  underlay_->RemoveValue(kBraveRewardsSyncBookmarks,
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kSyncBookmarks);

  // Check that overlay RemoveValue() is reported for mapped key.
  overlay_->RemoveValue(kSyncPasswords,
                        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kSyncPasswords);

  // Check that underlay RemoveValue() is reported for in-scope key.
  underlay_->RemoveValue(kBraveRewardsACEnabled,
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kBraveRewardsACEnabled);

  // Check that overlay RemoveValue() is reported for in-scope key.
  overlay_->RemoveValue(kBraveRewardsExternalWalletType,
                        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  obs.VerifyAndResetChangedKey(kBraveRewardsExternalWalletType);

  // Check that underlay RemoveValue() is not reported for out-of-scope key.
  underlay_->RemoveValue(kBraveNewTabPageHideAllWidgets,
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());

  // "Check that overlay RemoveValue() is not reported for out-of-scope key." is
  // not a valid test case (RemoveValue() doesn't even succeed in this
  // case).

  // Check successful unsubscription.
  overlay_->RemoveObserver(&obs);
  underlay_->SetValue(kBraveRewardsSyncBookmarks,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  overlay_->SetValue(kSyncPasswords, std::make_unique<base::Value>(false),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, GetValue) {
  // non-existent mapped key
  EXPECT_FALSE(overlay_->GetValue(kSyncPasswords, nullptr));
  const base::Value* value = nullptr;
  EXPECT_FALSE(overlay_->GetValue(kSyncPasswords, &value));
  EXPECT_EQ(value, nullptr);

  // existing mapped key
  underlay_->SetValue(kBraveRewardsSyncPasswords,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(overlay_->GetValue(kSyncPasswords, nullptr));
  EXPECT_TRUE(overlay_->GetValue(kSyncPasswords, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));

  // non-existent in-scope key
  EXPECT_FALSE(overlay_->GetValue(kBraveRewardsExternalWalletType, nullptr));
  value = nullptr;
  EXPECT_FALSE(overlay_->GetValue(kBraveRewardsExternalWalletType, &value));
  EXPECT_EQ(value, nullptr);

  // existing in-scope key
  underlay_->SetValue(kBraveRewardsExternalWalletType,
                      std::make_unique<base::Value>("uphold"),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(overlay_->GetValue(kBraveRewardsExternalWalletType, nullptr));
  EXPECT_TRUE(overlay_->GetValue(kBraveRewardsExternalWalletType, &value));
  EXPECT_TRUE(base::Value("uphold").Equals(value));

  // non-existent out-of-scope key
  EXPECT_FALSE(overlay_->GetValue(kBrowserHasSeenWelcomePage, nullptr));
  value = nullptr;
  EXPECT_FALSE(overlay_->GetValue(kBrowserHasSeenWelcomePage, &value));
  EXPECT_EQ(value, nullptr);

  // existing out-of-scope key
  underlay_->SetValue(kBrowserHasSeenWelcomePage,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_FALSE(overlay_->GetValue(kBrowserHasSeenWelcomePage, nullptr));
  EXPECT_FALSE(overlay_->GetValue(kBrowserHasSeenWelcomePage, &value));
  EXPECT_EQ(value, nullptr);
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, GetValues) {
  // adding mapped keys
  underlay_->SetValue(kBraveRewardsSyncBookmarks,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBraveRewardsSyncPasswords,
                      std::make_unique<base::Value>(false),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  // adding in-scope keys
  underlay_->SetValue(kBraveRewardsACEnabled,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBraveRewardsExternalWalletType,
                      std::make_unique<base::Value>("bitflyer"),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  // adding out-of-scope keys
  underlay_->SetValue(kBraveNewTabPageHideAllWidgets,
                      std::make_unique<base::Value>(false),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBrowserHasSeenWelcomePage,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  auto values = overlay_->GetValues();
  ASSERT_TRUE(values);

  // Check that mapped keys are in their original location.
  const base::Value* value = nullptr;
  EXPECT_TRUE(values->Get(kSyncBookmarks, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));
  value = nullptr;
  EXPECT_TRUE(values->Get(kSyncPasswords, &value));
  EXPECT_TRUE(base::Value(false).Equals(value));

  // Check that in-scope keys are in place.
  value = nullptr;
  EXPECT_TRUE(values->Get(kBraveRewardsACEnabled, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));
  value = nullptr;
  EXPECT_TRUE(values->Get(kBraveRewardsExternalWalletType, &value));
  EXPECT_TRUE(base::Value("bitflyer").Equals(value));

  // Check that out-of-scope keys are not present.
  value = nullptr;
  EXPECT_FALSE(values->Get(kBraveNewTabPageHideAllWidgets, &value));
  EXPECT_EQ(value, nullptr);
  EXPECT_FALSE(values->Get(kBrowserHasSeenWelcomePage, &value));
  EXPECT_EQ(value, nullptr);
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, SetValue) {
  // mapped key
  overlay_->SetValue(kSyncBookmarks, std::make_unique<base::Value>(true),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  const base::Value* value = nullptr;
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsSyncBookmarks, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));

  // in-scope key
  overlay_->SetValue(kBraveRewardsACEnabled,
                     std::make_unique<base::Value>(true),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  value = nullptr;
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsACEnabled, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));

  // out-of-scope key
  overlay_->SetValue(kBraveNewTabPageHideAllWidgets,
                     std::make_unique<base::Value>(false),
                     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  value = nullptr;
  EXPECT_FALSE(underlay_->GetValue(kBraveNewTabPageHideAllWidgets, &value));
  EXPECT_EQ(value, nullptr);
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, RemoveValue) {
  // mapped key
  underlay_->SetValue(kBraveRewardsSyncPasswords,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsSyncPasswords, nullptr));
  overlay_->RemoveValue(kSyncPasswords,
                        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsSyncPasswords, nullptr));

  // in-scope key
  underlay_->SetValue(kBraveRewardsExternalWalletType,
                      std::make_unique<base::Value>("gemini"),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsExternalWalletType, nullptr));
  overlay_->RemoveValue(kBraveRewardsExternalWalletType,
                        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsExternalWalletType, nullptr));

  // out-of-scope key
  underlay_->SetValue(kBrowserHasSeenWelcomePage,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(underlay_->GetValue(kBrowserHasSeenWelcomePage, nullptr));
  overlay_->RemoveValue(kBrowserHasSeenWelcomePage,
                        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(underlay_->GetValue(kBrowserHasSeenWelcomePage, nullptr));
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, GetMutableValue) {
  // non-existent mapped key (not supported - see implementation)
  EXPECT_FALSE(overlay_->GetMutableValue(kSyncBookmarks, nullptr));
  base::Value* overlay_value = nullptr;
  EXPECT_FALSE(overlay_->GetMutableValue(kSyncBookmarks, &overlay_value));
  EXPECT_EQ(overlay_value, nullptr);

  // existing mapped key (not supported - see implementation)
  underlay_->SetValue(
      kBraveRewardsSyncBookmarks,
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY),
      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_FALSE(overlay_->GetMutableValue(kSyncBookmarks, nullptr));
  EXPECT_FALSE(overlay_->GetMutableValue(kSyncBookmarks, &overlay_value));
  EXPECT_EQ(overlay_value, nullptr);

  // non-existent in-scope key
  EXPECT_FALSE(overlay_->GetMutableValue(kBraveRewardsACEnabled, nullptr));
  EXPECT_FALSE(
      overlay_->GetMutableValue(kBraveRewardsACEnabled, &overlay_value));
  EXPECT_EQ(overlay_value, nullptr);

  // existing in-scope key
  underlay_->SetValue(
      kBraveRewardsACEnabled,
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY),
      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(overlay_->GetMutableValue(kBraveRewardsACEnabled, nullptr));
  EXPECT_TRUE(
      overlay_->GetMutableValue(kBraveRewardsACEnabled, &overlay_value));
  ASSERT_TRUE(overlay_value->is_dict());
  static_cast<base::DictionaryValue*>(overlay_value)
      ->SetBoolean(kBraveRewardsACEnabled, true);

  const base::Value* underlay_value = nullptr;
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsACEnabled, &underlay_value));
  EXPECT_EQ(*overlay_value, *underlay_value);

  // non-existent out-of-scope key
  EXPECT_FALSE(
      overlay_->GetMutableValue(kBraveNewTabPageHideAllWidgets, nullptr));
  overlay_value = nullptr;
  EXPECT_FALSE(overlay_->GetMutableValue(kBraveNewTabPageHideAllWidgets,
                                         &overlay_value));
  EXPECT_EQ(overlay_value, nullptr);

  // existing out-of-scope key
  underlay_->SetValue(
      kBraveNewTabPageHideAllWidgets,
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY),
      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_FALSE(
      overlay_->GetMutableValue(kBraveNewTabPageHideAllWidgets, nullptr));
  EXPECT_FALSE(overlay_->GetMutableValue(kBraveNewTabPageHideAllWidgets,
                                         &overlay_value));
  EXPECT_EQ(overlay_value, nullptr);
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, SetValueSilently) {
  PrefStoreObserverMock obs;
  overlay_->AddObserver(&obs);

  // mapped key
  overlay_->SetValueSilently(kSyncBookmarks,
                             std::make_unique<base::Value>(true),
                             WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());
  const base::Value* value = nullptr;
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsSyncBookmarks, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));

  // in-scope key
  overlay_->SetValueSilently(kBraveRewardsACEnabled,
                             std::make_unique<base::Value>(true),
                             WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());
  value = nullptr;
  EXPECT_TRUE(underlay_->GetValue(kBraveRewardsACEnabled, &value));
  EXPECT_TRUE(base::Value(true).Equals(value));

  // out-of-scope key
  overlay_->SetValueSilently(kBraveNewTabPageHideAllWidgets,
                             std::make_unique<base::Value>(false),
                             WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  EXPECT_TRUE(obs.changed_keys.empty());
  value = nullptr;
  EXPECT_FALSE(underlay_->GetValue(kBraveNewTabPageHideAllWidgets, &value));
  EXPECT_EQ(value, nullptr);

  overlay_->RemoveObserver(&obs);
}

TEST_F(RewardsSyncScopedPersistentPrefStoreTest, RemoveValuesByPrefixSilently) {
  // adding mapped keys
  underlay_->SetValue(kBraveRewardsSyncBookmarks,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBraveRewardsSyncPasswords,
                      std::make_unique<base::Value>(false),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  // adding in-scope keys
  underlay_->SetValue(kBraveRewardsACEnabled,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBraveRewardsExternalWalletType,
                      std::make_unique<base::Value>("bitflyer"),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  // adding out-of-scope keys
  underlay_->SetValue(kBraveNewTabPageHideAllWidgets,
                      std::make_unique<base::Value>(false),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  underlay_->SetValue(kBrowserHasSeenWelcomePage,
                      std::make_unique<base::Value>(true),
                      WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  PrefStoreObserverMock obs;
  overlay_->AddObserver(&obs);

  overlay_->RemoveValuesByPrefixSilently(kSync);
  EXPECT_TRUE(obs.changed_keys.empty());
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsSyncBookmarks, nullptr));
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsSyncPasswords, nullptr));

  overlay_->RemoveValuesByPrefixSilently(kBraveRewards);
  EXPECT_TRUE(obs.changed_keys.empty());
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsACEnabled, nullptr));
  EXPECT_FALSE(underlay_->GetValue(kBraveRewardsExternalWalletType, nullptr));

  overlay_->RemoveValuesByPrefixSilently(kBrave);
  EXPECT_TRUE(obs.changed_keys.empty());
  EXPECT_TRUE(underlay_->GetValue(kBraveNewTabPageHideAllWidgets, nullptr));
  EXPECT_TRUE(underlay_->GetValue(kBrowserHasSeenWelcomePage, nullptr));

  overlay_->RemoveObserver(&obs);
}

using MapSyncParamType = std::pair<std::string, std::string>;

struct MapSync : RewardsSyncScopedPersistentPrefStoreTest,
                 WithParamInterface<MapSyncParamType> {};

template <typename ParamType>
std::string NameSuffixGenerator(const TestParamInfo<ParamType>& info) {
  auto input = std::get<0>(info.param);
  std::replace(input.begin(), input.end(), '.', '_');
  return input.empty() ? "empty" : input;
}

INSTANTIATE_TEST_SUITE_P(
    RewardsSyncScopedPersistentPrefStoreTest,
    MapSync,
    Values(MapSyncParamType("", ""),
           MapSyncParamType(kBrave, kBrave),
           MapSyncParamType(kBraveRewards, kBraveRewards),
           MapSyncParamType(kBraveRewardsSync, kBraveRewardsSync),
           MapSyncParamType(kSync, kBraveRewardsSync),
           MapSyncParamType(kSyncBookmarks, kBraveRewardsSyncBookmarks),
           MapSyncParamType(kSyncPasswords, kBraveRewardsSyncPasswords)),
    NameSuffixGenerator<MapSyncParamType>);

TEST_P(MapSync, Input) {
  EXPECT_EQ(overlay_->MapSync(std::get<0>(GetParam())),
            std::get<1>(GetParam()));
}
