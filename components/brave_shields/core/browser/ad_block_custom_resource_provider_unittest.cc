// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

base::Value CreateResource(const std::string& name,
                           const std::string& content) {
  base::Value::Dict resource;
  resource.Set("name", name);
  resource.Set("content", content);
  resource.SetByDottedPath("kind.mime", "application/javascript");
  return base::Value(std::move(resource));
}

class TestResourceProvider : public AdBlockResourceProvider {
 public:
  TestResourceProvider() = default;
  ~TestResourceProvider() override = default;

  void LoadResources(base::OnceCallback<void(const std::string& resources_json)>
                         on_load) override {
    std::move(on_load).Run(resources_json_);
  }

  void SetResources(const std::string& resources_json) {
    resources_json_ = resources_json;
  }

 private:
  std::string resources_json_;
};

}  // namespace

class AdBlockCustomResourceProviderTest : public ::testing::Test {
 public:
  AdBlockCustomResourceProviderTest() {
    feature_list_.InitAndEnableFeature(
        features::kCosmeticFilteringCustomScriptlets);
  }

  ~AdBlockCustomResourceProviderTest() override = default;

  void SetUp() override {
    auto default_resource_provider = std::make_unique<TestResourceProvider>();
    default_resource_provider_ = default_resource_provider.get();

    prefs_.registry()->RegisterBooleanPref(prefs::kAdBlockDeveloperMode, false);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    custom_resource_provider_ = std::make_unique<AdBlockCustomResourceProvider>(
        temp_dir_.GetPath(), std::move(default_resource_provider));
  }

  PrefService* prefs() { return &prefs_; }

  AdBlockCustomResourceProvider* custom_resource_provider() {
    return custom_resource_provider_.get();
  }

  TestResourceProvider* default_resource_provider() {
    return default_resource_provider_.get();
  }

  base::Value GetResources() {
    base::test::TestFuture<base::Value> value;
    custom_resource_provider()->GetCustomResources(value.GetCallback());
    return value.Take();
  }

  AdBlockCustomResourceProvider::ErrorCode AddResource(
      const base::Value& resource) {
    base::test::TestFuture<AdBlockCustomResourceProvider::ErrorCode> result;
    custom_resource_provider()->AddResource(prefs(), resource,
                                            result.GetCallback());
    return result.Take();
  }

  AdBlockCustomResourceProvider::ErrorCode RemoveResource(
      const std::string& resource) {
    base::test::TestFuture<AdBlockCustomResourceProvider::ErrorCode> result;
    custom_resource_provider()->RemoveResource(prefs(), resource,
                                               result.GetCallback());
    return result.Take();
  }

  AdBlockCustomResourceProvider::ErrorCode UpdateResource(
      const std::string& resource_name,
      const base::Value& update) {
    base::test::TestFuture<AdBlockCustomResourceProvider::ErrorCode> result;
    custom_resource_provider()->UpdateResource(prefs(), resource_name, update,
                                               result.GetCallback());
    return result.Take();
  }

  base::Value LoadResources() {
    base::test::TestFuture<const std::string&> result;
    custom_resource_provider()->LoadResources(result.GetCallback());
    if (result.Get().empty()) {
      return base::Value(base::ListValue());
    }
    return *base::JSONReader::Read(result.Take(),
                                   base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<AdBlockCustomResourceProvider> custom_resource_provider_;
  raw_ptr<TestResourceProvider> default_resource_provider_ = nullptr;
  TestingPrefServiceSimple prefs_;
};

TEST_F(AdBlockCustomResourceProviderTest, AddResource) {
  {
    // empty list by default.
    EXPECT_EQ(base::ListValue(), GetResources());
  }
  {
    // Dev mode OFF.
    auto resource = CreateResource("user-1.js", "user-1");
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kInvalid,
              AddResource(resource));

    EXPECT_EQ(base::ListValue(), GetResources());
  }

  {
    // Dev mode ON.
    prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);

    base::ListValue expect;

    {
      auto resource = CreateResource("user-1.js", "user-1");
      EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
                AddResource(resource));

      expect.Append(std::move(resource));
      EXPECT_EQ(expect, GetResources());
    }

    {
      // Try the same.
      auto resource = CreateResource("user-1.js", "user-1");
      EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kAlreadyExists,
                AddResource(resource));

      EXPECT_EQ(expect, GetResources());
    }

    {
      // One more.
      auto resource = CreateResource("user-2.js", "user-2");
      base::test::TestFuture<AdBlockCustomResourceProvider::ErrorCode> result;
      custom_resource_provider()->AddResource(prefs(), resource,
                                              result.GetCallback());
      EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk, result.Get());

      expect.Append(std::move(resource));
      EXPECT_EQ(expect, GetResources());
    }

    {
      // Invalid resource.
      EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kInvalid,
                AddResource(base::Value()));
    }
  }
}

TEST_F(AdBlockCustomResourceProviderTest, RemoveResource) {
  prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);
  AddResource(CreateResource("user-1.js", "user-1"));
  AddResource(CreateResource("user-2.js", "user-2"));
  AddResource(CreateResource("user-3.js", "user-3"));

  {
    prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, false);
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kInvalid,
              RemoveResource("user-1"));
  }

  {
    prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              RemoveResource("user-1.js"));

    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kNotFound,
              RemoveResource("user-1.js"));
  }

  {
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              RemoveResource("user-2.js"));
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              RemoveResource("user-3.js"));
    EXPECT_EQ(base::ListValue(), GetResources());
  }
}

TEST_F(AdBlockCustomResourceProviderTest, UpdateResource) {
  prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);
  AddResource(CreateResource("user-1.js", "user-1"));
  AddResource(CreateResource("user-2.js", "user-2"));
  AddResource(CreateResource("user-3.js", "user-3"));

  {
    prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, false);
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kInvalid,
              UpdateResource("user-1.js", CreateResource("user-1-update.js",
                                                         "user-1-update.js")));
  }
  {
    prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kInvalid,
              UpdateResource("user-1.js", base::Value()));

    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kNotFound,
              UpdateResource("user-4.js", CreateResource("user-4-update.js",
                                                         "user-4-update.js")));
  }
  {
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kAlreadyExists,
              UpdateResource("user-1.js",
                             CreateResource("user-2.js", "user-1-updated")));
  }
  {
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              UpdateResource("user-1.js", CreateResource("user-1-update.js",
                                                         "user-1-update.js")));
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              UpdateResource("user-2.js", CreateResource("user-2-update.js",
                                                         "user-2-update.js")));
    EXPECT_EQ(AdBlockCustomResourceProvider::ErrorCode::kOk,
              UpdateResource("user-3.js", CreateResource("user-3-update.js",
                                                         "user-3-update.js")));
    EXPECT_EQ(
        base::ListValue()
            .Append(CreateResource("user-1-update.js", "user-1-update.js"))
            .Append(CreateResource("user-2-update.js", "user-2-update.js"))
            .Append(CreateResource("user-3-update.js", "user-3-update.js")),
        GetResources());
  }
}

TEST_F(AdBlockCustomResourceProviderTest, LoadResource) {
  EXPECT_EQ(base::ListValue(), LoadResources());

  default_resource_provider()->SetResources(
      base::ListValue()
          .Append(CreateResource("default-1.js", "default-1"))
          .DebugString());

  EXPECT_EQ(
      base::ListValue().Append(CreateResource("default-1.js", "default-1")),
      LoadResources());

  prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, true);
  AddResource(CreateResource("user-1.js", "user-1"));
  EXPECT_EQ(base::ListValue()
                .Append(CreateResource("default-1.js", "default-1"))
                .Append(CreateResource("user-1.js", "user-1")),
            LoadResources());

  prefs()->SetBoolean(prefs::kAdBlockDeveloperMode, false);
  EXPECT_EQ(base::ListValue()
                .Append(CreateResource("default-1.js", "default-1"))
                .Append(CreateResource("user-1.js", "user-1")),
            LoadResources());
}

}  // namespace brave_shields
