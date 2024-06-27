// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_service.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class ModelServiceTest : public ::testing::Test {
 public:
  void SetUp() override {
    OSCryptMocker::SetUp();
    prefs::RegisterProfilePrefs(pref_service_.registry());
    prefs::RegisterProfilePrefsForMigration(pref_service_.registry());
    ModelService::RegisterProfilePrefs(pref_service_.registry());

    service_ = std::make_unique<ModelService>(&pref_service_);
  }

  void TearDown() override { OSCryptMocker::TearDown(); }

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ModelService> service_;
};

TEST_F(ModelServiceTest, ChangeOldDefaultKey) {
  service_->SetDefaultModelKeyWithoutValidationForTesting("chat-default");
  ModelService::MigrateProfilePrefs(&pref_service_);

  EXPECT_EQ(service_->GetDefaultModelKey(), "chat-basic");
}

TEST_F(ModelServiceTest, AddAndModifyCustomModel) {
  static const char kRequestName[] = "request_name";
  static const GURL kEndpoint = GURL("http://brave.com");
  static const char kAPIKey[] = "foo_api_key";
  static const char kDisplayName[] = "Custom display name";

  {
    mojom::ModelPtr model = mojom::Model::New();
    model->display_name = kDisplayName;
    model->options = mojom::ModelOptions::NewCustomModelOptions(
        mojom::CustomModelOptions::New(kRequestName, kEndpoint, kAPIKey));

    service_->AddCustomModel(std::move(model));
  }

  const std::vector<mojom::ModelPtr>& models = service_->GetModels();

  EXPECT_EQ(models.back()->display_name, kDisplayName);
  EXPECT_EQ(
      models.back()->options->get_custom_model_options()->model_request_name,
      kRequestName);
  EXPECT_EQ(models.back()->options->get_custom_model_options()->endpoint.spec(),
            kEndpoint.spec());
  EXPECT_EQ(models.back()->options->get_custom_model_options()->api_key,
            kAPIKey);
}

TEST_F(ModelServiceTest, ChangeDefaultModelKey) {
  service_->SetDefaultModelKey("chat-basic");
  EXPECT_EQ(service_->GetDefaultModelKey(), "chat-basic");
  service_->SetDefaultModelKey("bad-key");
  // Default model key should not change if the key is invalid.
  EXPECT_EQ(service_->GetDefaultModelKey(), "chat-basic");
}

}  // namespace ai_chat
