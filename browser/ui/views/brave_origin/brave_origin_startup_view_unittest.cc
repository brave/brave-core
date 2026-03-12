/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_origin/brave_origin_startup_view.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveOriginStartupViewTest : public testing::Test {
 public:
  void SetUp() override {
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPurchaseValidated, false);
    local_state_.registry()->RegisterDictionaryPref(skus::prefs::kSkusState);
    BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  }

  void TearDown() override {
    BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  }

  // Builds a JSON string representing an SKU state entry with credentials.
  // If |has_items| is true, the credentials dict contains a non-empty "items"
  // dict; otherwise it is empty.
  std::string BuildSkuStateJson(bool has_items) {
    if (has_items) {
      return R"({
        "credentials": {
          "items": {
            "origin": "some-credential-value"
          }
        }
      })";
    }
    return R"({
      "credentials": {
        "items": {}
      }
    })";
  }

  void SetSkuCredentials(const std::string& env_key, const std::string& json) {
    base::DictValue skus_state;
    skus_state.Set(env_key, json);
    local_state_.SetDict(skus::prefs::kSkusState, std::move(skus_state));
  }

 protected:
  TestingPrefServiceSimple local_state_;
};

// --- ShouldShowDialog tests ---

TEST_F(BraveOriginStartupViewTest,
       ShouldShowDialogWhenPurchaseNotValidatedAndNoCredentials) {
  // Default state: pref is false, no SKU credentials -> should show.
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldNotShowDialogWhenPurchaseValidatedAndHasCredentials) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", BuildSkuStateJson(/*has_items=*/true));
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldShowDialogWhenPurchaseValidatedButNoCredentials) {
  // Pref validated but SKU credentials missing -> should show.
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldShowDialogWhenNotValidatedButHasCredentials) {
  // Has credentials but pref not set -> should show.
  SetSkuCredentials("production", BuildSkuStateJson(/*has_items=*/true));
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldNotShowDialogWhenValidatedAndMultipleEnvCredentials) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  base::DictValue skus_state;
  skus_state.Set("staging", BuildSkuStateJson(/*has_items=*/false));
  skus_state.Set("production", BuildSkuStateJson(/*has_items=*/true));
  local_state_.SetDict(skus::prefs::kSkusState, std::move(skus_state));

  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldShowDialogWhenCredentialsHaveEmptyItems) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", BuildSkuStateJson(/*has_items=*/false));
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest, ShouldShowDialogWhenSkuStateHasInvalidJson) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", "not-valid-json{{{");
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest,
       ShouldShowDialogWhenSkuStateHasNonStringValue) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  base::DictValue skus_state;
  skus_state.Set("production", 42);  // Not a string.
  local_state_.SetDict(skus::prefs::kSkusState, std::move(skus_state));
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest, ShouldShowDialogWhenCredentialsDictMissing) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", R"({"some_other_key": "value"})");
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest, ShouldShowDialogWhenItemsDictMissing) {
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", R"({"credentials": {"not_items": "value"}})");
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

// --- SetShouldShowDialogForTesting tests ---

TEST_F(BraveOriginStartupViewTest, TestOverrideForceShow) {
  // Even with valid state, the override forces show.
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  SetSkuCredentials("production", BuildSkuStateJson(/*has_items=*/true));

  BraveOriginStartupView::SetShouldShowDialogForTesting(true);
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest, TestOverrideForceHide) {
  // Even with invalid state, the override forces hide.
  BraveOriginStartupView::SetShouldShowDialogForTesting(false);
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

TEST_F(BraveOriginStartupViewTest, TestOverrideResetsToNormal) {
  BraveOriginStartupView::SetShouldShowDialogForTesting(false);
  EXPECT_FALSE(BraveOriginStartupView::ShouldShowDialog(&local_state_));

  BraveOriginStartupView::SetShouldShowDialogForTesting(std::nullopt);
  // Back to normal: default state should show.
  EXPECT_TRUE(BraveOriginStartupView::ShouldShowDialog(&local_state_));
}

// --- IsShowing tests (without needing a widget) ---

TEST_F(BraveOriginStartupViewTest, IsShowingReturnsFalseByDefault) {
  EXPECT_FALSE(BraveOriginStartupView::IsShowing());
}
