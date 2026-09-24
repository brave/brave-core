// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_on_device_head_provider.h"

#include <memory>
#include <string_view>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/on_device_model_update_listener.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveOnDeviceHeadProviderTest : public testing::Test,
                                      public AutocompleteProviderListener {
 public:
  AutocompleteInput CreateAutocompleteInput(std::string_view text) {
    AutocompleteInput input(base::UTF8ToUTF16(text),
                            metrics::OmniboxEventProto::OTHER, classifier_);
    return input;
  }

  void SetUp() override {
    provider_ =
        base::WrapRefCounted(BraveOnDeviceHeadProvider::Create(&client_, this));
    SetupTestOnDeviceHeadModel();
  }

  void SetupTestOnDeviceHeadModel() {
    base::FilePath file_path;
    base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &file_path);
    // The same test model also used in ./on_device_head_model_unittest.cc.
    file_path = file_path.AppendASCII("components/test/data/omnibox");
    ASSERT_TRUE(base::PathExists(file_path));
    auto* update_listener = OnDeviceModelUpdateListener::GetInstance();
    if (update_listener) {
      update_listener->OnHeadModelUpdate(file_path);
    }
    task_environment_.RunUntilIdle();
  }

  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {}

  PrefService* prefs() { return client_.GetPrefs(); }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestSchemeClassifier classifier_;
  BraveFakeAutocompleteProviderClient client_;
  scoped_refptr<BraveOnDeviceHeadProvider> provider_;
};

TEST_F(BraveOnDeviceHeadProviderTest, SuggestionsDisabledNoResults) {
  prefs()->SetBoolean(omnibox::kOnDeviceSuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("Hello"), false);
  EXPECT_TRUE(provider_->done());
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(BraveOnDeviceHeadProviderTest, SuggestionsEnabledRunsProvider) {
  prefs()->SetBoolean(omnibox::kOnDeviceSuggestionsEnabled, true);
  provider_->Start(CreateAutocompleteInput("Hello"), false);
  EXPECT_FALSE(provider_->done());
}
