/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/network_time/network_time_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/task_environment.h"
#include "components/client_update_protocol/ecdsa.h"
#include "components/network_time/network_time_pref_names.h"
#include "components/network_time/network_time_test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network_time {

class NetworkTimeTrackerTest : public ::testing::Test {
 public:
  ~NetworkTimeTrackerTest() override {}

  NetworkTimeTrackerTest()
      : task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        clock_(new base::SimpleTestClock),
        tick_clock_(new base::SimpleTestTickClock),
        test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {
    NetworkTimeTracker::RegisterPrefs(pref_service_.registry());
    tracker_.reset(new NetworkTimeTracker(
        std::unique_ptr<base::Clock>(clock_),
        std::unique_ptr<const base::TickClock>(tick_clock_), &pref_service_,
        shared_url_loader_factory()));
  }

 protected:
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
      return test_shared_loader_factory_;
  }
  network::TestURLLoaderFactory* test_url_loader_factory() {
      return &test_url_loader_factory_;
  }

  base::test::ScopedTaskEnvironment task_environment_;
  raw_ptr<base::SimpleTestClock> clock_ = nullptr;
  raw_ptr<base::SimpleTestTickClock> tick_clock_ = nullptr;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NetworkTimeTracker> tracker_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
};

TEST_F(NetworkTimeTrackerTest, Disabled) {
  EXPECT_FALSE(tracker_->AreTimeFetchesEnabled());
}

TEST_F(NetworkTimeTrackerTest, NoFetch) {
  bool network_access_occurred = false;
  test_url_loader_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                     network_access_occurred = true;
                                 }));
  tracker_->QueryTimeServiceForTesting();
  EXPECT_FALSE(network_access_occurred);
}

}  // namespace network_time
