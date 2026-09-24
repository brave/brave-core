// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/on_device_head_provider.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/on_device_model_update_listener.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::NiceMock;
using testing::Return;

class OnDeviceHeadProviderTest : public testing::Test,
                                 public AutocompleteProviderListener {
 protected:
  void SetUp() override {
    client_ = std::make_unique<FakeAutocompleteProviderClient>();
    SetupTestOnDeviceHeadModel();
    provider_ = OnDeviceHeadProvider::Create(client_.get(), this);
    task_environment_.RunUntilIdle();
  }

  void TearDown() override {
    provider_ = nullptr;
    client_.reset();
    task_environment_.RunUntilIdle();
  }

  // AutocompleteProviderListener:
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {
    // No action required.
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

  bool IsOnDeviceHeadProviderAllowed(const AutocompleteInput& input) {
    return provider_->IsOnDeviceHeadProviderAllowed(input);
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;
  scoped_refptr<OnDeviceHeadProvider> provider_;
};

TEST_F(OnDeviceHeadProviderTest, CheckEnabledIfSuggestionsAreDisabled) {
  AutocompleteInput input(u"M", metrics::OmniboxEventProto::OTHER,
                          TestSchemeClassifier());
  input.set_omit_asynchronous_matches(false);

  EXPECT_CALL(*client_.get(), IsOffTheRecord()).WillRepeatedly(Return(false));
  EXPECT_CALL(*client_.get(), SearchSuggestEnabled())
      .WillRepeatedly(Return(false));

  ASSERT_TRUE(IsOnDeviceHeadProviderAllowed(input));

  provider_->Start(input, false);
  task_environment_.RunUntilIdle();

  EXPECT_TRUE(provider_->done());
  ASSERT_EQ(3U, provider_->matches().size());
  EXPECT_EQ(u"maps", provider_->matches()[0].contents);
  EXPECT_EQ(u"mail", provider_->matches()[1].contents);
  EXPECT_EQ(u"map", provider_->matches()[2].contents);
}
