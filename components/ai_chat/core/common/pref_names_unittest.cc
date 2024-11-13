// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/pref_names.h"

#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::prefs {

class AIChatPrefMigrationTest : public ::testing::Test {
 public:
  void SetUp() override {
    RegisterProfilePrefs(pref_service_.registry());
    RegisterProfilePrefsForMigration(pref_service_.registry());
  }

  TestingPrefServiceSimple pref_service_;
};

}  // namespace ai_chat::prefs
