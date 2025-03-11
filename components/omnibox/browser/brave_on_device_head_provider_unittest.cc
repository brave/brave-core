// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_on_device_head_provider.h"

#include <memory>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
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
  prefs()->SetBoolean(omnibox::kTopSuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("Hello"), false);
  EXPECT_TRUE(provider_->done());
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(BraveOnDeviceHeadProviderTest, SuggestionsEnabledRunsProvider) {
  prefs()->SetBoolean(omnibox::kTopSuggestionsEnabled, true);
  provider_->Start(CreateAutocompleteInput("Hello"), false);
  EXPECT_FALSE(provider_->done());
}
