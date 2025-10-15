/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/pref_intermediate.h"

#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {
constexpr char kTestPrefName[] = "test_pref";
constexpr char kDefaultValue[] = "default_value";
constexpr char kNewValue[] = "new_value";
}  // namespace

class P3APrefIntermediateTest : public testing::Test,
                                public RemoteMetricIntermediate::Delegate {
 public:
  P3APrefIntermediateTest() = default;

  // RemoteMetricIntermediate::Delegate implementation
  void TriggerUpdate() override { trigger_update_called_ = true; }
  TimePeriodStorage* GetTimePeriodStorage(std::string_view, int) override {
    return nullptr;
  }
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value&) override {
    return nullptr;
  }

 protected:
  PrefIntermediateDefinition ParseDefinition(std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    PrefIntermediateDefinition definition;
    base::JSONValueConverter<PrefIntermediateDefinition> converter;
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  void SetUp() override {
    local_state_.registry()->RegisterStringPref(kTestPrefName, kDefaultValue);
    profile_prefs_.registry()->RegisterStringPref(kTestPrefName, kDefaultValue);
  }

  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  bool trigger_update_called_ = false;
};

TEST_F(P3APrefIntermediateTest, InitFailsIfPrefNameEmpty) {
  // Test with empty pref name using JSON
  const char json[] = R"({
    "pref_name": "",
    "use_profile_prefs": false
  })";

  auto def = ParseDefinition(json);
  EXPECT_TRUE(def.pref_name.empty());
  EXPECT_FALSE(def.use_profile_prefs);

  PrefIntermediate pref(std::move(def), &local_state_, &profile_prefs_, this);
  EXPECT_FALSE(pref.Init());
}

TEST_F(P3APrefIntermediateTest, InitFailsIfPrefDoesNotExist) {
  // Test with non-existent pref name
  const char json[] = R"({
    "pref_name": "non_existent_pref",
    "use_profile_prefs": false
  })";

  auto def = ParseDefinition(json);
  EXPECT_EQ(def.pref_name, "non_existent_pref");

  PrefIntermediate pref(std::move(def), &local_state_, &profile_prefs_, this);
  EXPECT_FALSE(pref.Init());
}

TEST_F(P3APrefIntermediateTest, ProcessReturnsLocalStatePrefValue) {
  // Test with valid pref name using local state
  const char json[] = R"({
    "pref_name": "test_pref",
    "use_profile_prefs": false
  })";

  auto def = ParseDefinition(json);
  EXPECT_EQ(def.pref_name, kTestPrefName);
  EXPECT_FALSE(def.use_profile_prefs);

  PrefIntermediate pref(std::move(def), &local_state_, &profile_prefs_, this);
  ASSERT_TRUE(pref.Init());
  EXPECT_TRUE(pref.GetStorageKeys().empty());

  // Verify the initial value
  auto result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kDefaultValue);

  EXPECT_FALSE(trigger_update_called_);
  // Change the pref value
  local_state_.SetString(kTestPrefName, kNewValue);
  EXPECT_TRUE(trigger_update_called_);

  trigger_update_called_ = false;
  // Verify the updated value
  result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kNewValue);

  EXPECT_FALSE(trigger_update_called_);
}

TEST_F(P3APrefIntermediateTest, ProcessReturnsProfilePrefValue) {
  // Test with valid pref name using profile prefs
  const char json[] = R"({
    "pref_name": "test_pref",
    "use_profile_prefs": true
  })";

  auto def = ParseDefinition(json);
  EXPECT_EQ(def.pref_name, kTestPrefName);
  EXPECT_TRUE(def.use_profile_prefs);

  PrefIntermediate pref(std::move(def), &local_state_, &profile_prefs_, this);
  ASSERT_TRUE(pref.Init());

  // Verify the initial value
  auto result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kDefaultValue);

  EXPECT_FALSE(trigger_update_called_);
  // Change the pref value
  profile_prefs_.SetString(kTestPrefName, kNewValue);
  EXPECT_TRUE(trigger_update_called_);
  trigger_update_called_ = false;

  // Verify the updated value
  result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kNewValue);
  EXPECT_FALSE(trigger_update_called_);
}

TEST_F(P3APrefIntermediateTest, OnLastUsedProfilePrefsChanged) {
  // Test profile prefs change handling
  const char json[] = R"({
    "pref_name": "test_pref",
    "use_profile_prefs": true
  })";

  auto def = ParseDefinition(json);

  // Start with null profile prefs
  PrefIntermediate pref(std::move(def), &local_state_, nullptr, this);
  EXPECT_FALSE(pref.Init());

  // Now set the profile prefs
  EXPECT_FALSE(trigger_update_called_);
  pref.OnLastUsedProfilePrefsChanged(&profile_prefs_);
  EXPECT_TRUE(trigger_update_called_);

  // Verify we can now get the value
  auto result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kDefaultValue);

  // Ensure pref watcher is registered
  profile_prefs_.SetString(kTestPrefName, kNewValue);
  EXPECT_TRUE(trigger_update_called_);

  result = pref.Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), kNewValue);
}

}  // namespace p3a
