// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/services/device/geolocation/geoclue_client_object.h"
#include "services/device/public/cpp/device_features.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

class ScopedSetLocationEnabled {
 public:
  ScopedSetLocationEnabled(bool enabled) {
    base::CommandLine get_inital_location_allowed(base::FilePath("gsettings"));
    get_inital_location_allowed.AppendArg("get");
    get_inital_location_allowed.AppendArg("org.gnome.system.location");
    get_inital_location_allowed.AppendArg("enabled");

    std::string initial_value;
    CHECK(base::GetAppOutput(get_inital_location_allowed, &initial_value));

    std::string target_value = enabled ? "true" : "false";
    if (initial_value != target_value) {
      initial_value_ = initial_value;
      SetLocationEnabled(target_value);
    }
  }
  ~ScopedSetLocationEnabled() {
    if (!initial_value_) {
      return;
    }

    SetLocationEnabled(initial_value_.value());
  }

 private:
  static void SetLocationEnabled(const std::string& value) {
    base::CommandLine set_allow_location_for_test(base::FilePath("gsettings"));
    set_allow_location_for_test.AppendArg("set");
    set_allow_location_for_test.AppendArg("org.gnome.system.location");
    set_allow_location_for_test.AppendArg("enabled");
    set_allow_location_for_test.AppendArg(value);

    std::string allow_location_result;
    ASSERT_TRUE(base::GetAppOutput(set_allow_location_for_test,
                                   &allow_location_result));
  }
  std::optional<std::string> initial_value_;
};

}  // namespace

class TestGeoClueLocationProvider : public GeoClueLocationProvider {
 public:
  TestGeoClueLocationProvider() = default;
  ~TestGeoClueLocationProvider() override = default;

  bool HasPermission() { return permission_granted_; }

  bool HasClient() { return client_.get(); }

  bool Started() {
    return client_.get() &&
           client_->state_ == GeoClueClientObject::State::kStarted;
  }

  void SetPositionForTesting(mojom::GeopositionPtr position) {
    SetPosition(mojom::GeopositionResult::NewPosition(std::move(position)));
  }
};

class GeoClueLocationProviderTest : public testing::Test {
 public:
  GeoClueLocationProviderTest() = default;
  ~GeoClueLocationProviderTest() override = default;

  void InitializeProvider() {
    provider_ = std::make_unique<TestGeoClueLocationProvider>();
    provider_->SetUpdateCallback(
        base::BindLambdaForTesting([&](const LocationProvider* provider,
                                       mojom::GeopositionResultPtr position) {
          loop_->Quit();
          update_count_++;
        }));
  }

  void WaitForUpdate() {
    loop_ = std::make_unique<base::RunLoop>();
    loop_->Run();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  ScopedSetLocationEnabled enabled_location_{true};

  std::unique_ptr<base::RunLoop> loop_;
  int update_count_ = 0;

  std::unique_ptr<TestGeoClueLocationProvider> provider_;
};

TEST_F(GeoClueLocationProviderTest,
       CreateDestroy) {  // should not crash
  InitializeProvider();
  EXPECT_TRUE(provider_);
  provider_.reset();
}

TEST_F(GeoClueLocationProviderTest, OnPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->HasPermission());
  provider_->OnPermissionGranted();
  EXPECT_TRUE(provider_->HasPermission());
}

TEST_F(GeoClueLocationProviderTest, CanStart) {
  InitializeProvider();
  EXPECT_FALSE(provider_->HasClient());
  provider_->StartProvider(false);
  EXPECT_TRUE(provider_->HasClient());
}

TEST_F(GeoClueLocationProviderTest, CanStop) {
  InitializeProvider();
  EXPECT_FALSE(provider_->HasClient());

  // Shouldn't crash, even though we haven't started.
  provider_->StopProvider();

  EXPECT_FALSE(provider_->HasClient());

  provider_->StartProvider(true);
  EXPECT_TRUE(provider_->HasClient());

  provider_->StopProvider();
  EXPECT_FALSE(provider_->HasClient());

  // Shouldn't crash calling stop a second time, after having started.
  provider_->StopProvider();
  EXPECT_FALSE(provider_->HasClient());
}

TEST_F(GeoClueLocationProviderTest, CanStopPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasClient());

  provider_->OnPermissionGranted();
  provider_->StopProvider();

  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasClient());
  EXPECT_TRUE(provider_->HasPermission());
}

TEST_F(GeoClueLocationProviderTest, CanStopStartedAndPermissionGranted) {
  InitializeProvider();

  provider_->OnPermissionGranted();
  provider_->StartProvider(false);

  // Let everything initialize until we get a location
  WaitForUpdate();

  EXPECT_EQ(1, update_count_);
  EXPECT_TRUE(provider_->Started());
  EXPECT_TRUE(provider_->HasPermission());

  // After stopping, further updates should no propagate.
  provider_->StopProvider();

  // If the provider has no client, it can't get any more updates.
  EXPECT_FALSE(provider_->HasClient());
  EXPECT_FALSE(provider_->Started());
}

TEST_F(GeoClueLocationProviderTest, CanRestartProvider) {
  InitializeProvider();

  provider_->OnPermissionGranted();
  provider_->StartProvider(true);
  EXPECT_TRUE(provider_->HasClient());

  WaitForUpdate();

  EXPECT_TRUE(provider_->Started());
  EXPECT_EQ(1, update_count_);

  provider_->StopProvider();
  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasClient());

  provider_->StartProvider(true);
  EXPECT_TRUE(provider_->HasClient());

  WaitForUpdate();
  EXPECT_TRUE(provider_->Started());
  EXPECT_EQ(2, update_count_);
}

TEST_F(GeoClueLocationProviderTest, NoLocationUntilPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasClient());
  EXPECT_FALSE(provider_->HasPermission());
  EXPECT_EQ(0, update_count_);

  provider_->StartProvider(false);
  EXPECT_TRUE(provider_->HasClient());
  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasPermission());
  EXPECT_EQ(0, update_count_);

  provider_->OnPermissionGranted();
  EXPECT_TRUE(provider_->HasPermission());

  // Wait for the client to initialize.
  WaitForUpdate();
  EXPECT_EQ(1, update_count_);
  EXPECT_TRUE(provider_->Started());
}

TEST_F(GeoClueLocationProviderTest, GetsLocation) {
  InitializeProvider();
  provider_->StartProvider(false);
  provider_->OnPermissionGranted();

  WaitForUpdate();
  EXPECT_EQ(1, update_count_);

  EXPECT_LE(provider_->GetPosition()->get_position()->latitude, 90);
  EXPECT_GE(provider_->GetPosition()->get_position()->latitude, -90);
  EXPECT_LE(provider_->GetPosition()->get_position()->longitude, 180);
  EXPECT_GE(provider_->GetPosition()->get_position()->longitude, -180);
  EXPECT_GE(provider_->GetPosition()->get_position()->accuracy, 0);
  EXPECT_FALSE(provider_->GetPosition()->get_position()->timestamp.is_null());
}

TEST_F(GeoClueLocationProviderTest,
       DoesNotInitializeWithoutFeatureAndIsDisabledByDefault) {
  auto provider = MaybeCreateGeoClueLocationProvider();
  EXPECT_FALSE(provider);
}

TEST_F(GeoClueLocationProviderTest, InitializesWithFeature) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(features::kLinuxGeoClueLocationBackend);
  EXPECT_TRUE(
      base::FeatureList::IsEnabled(features::kLinuxGeoClueLocationBackend));

  auto provider = MaybeCreateGeoClueLocationProvider();
  EXPECT_TRUE(provider);
}

}  // namespace device
