// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/suggestions_controller.h"

#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/browser/unsupported_publisher_migrator.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {
class SuggestionsControllerTest : public testing::Test {
 public:
  SuggestionsControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        direct_feed_controller_(profile_.GetPrefs(), nullptr),
        unsupported_publisher_migrator_(profile_.GetPrefs(),
                                        &direct_feed_controller_,
                                        &api_request_helper_),
        publishers_controller_(profile_.GetPrefs(),
                               &direct_feed_controller_,
                               &unsupported_publisher_migrator_,
                               &api_request_helper_),
        suggestions_controller_(profile_.GetPrefs(),
                                &publishers_controller_,
                                &api_request_helper_,
                                nullptr) {}
  ~SuggestionsControllerTest() = default;

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  TestingProfile profile_;

  DirectFeedController direct_feed_controller_;
  UnsupportedPublisherMigrator unsupported_publisher_migrator_;
  PublishersController publishers_controller_;
  SuggestionsController suggestions_controller_;
};

TEST_F(SuggestionsControllerTest, VisitedSourcesAreSuggested) {

}

}  // namespace brave_news
