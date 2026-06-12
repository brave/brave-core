// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/farbling_args.h"

#include "base/test/scoped_feature_list.h"
#include "base/token.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_shields {

namespace {

// A pair of arbitrary but stable tokens to seed the generator with. Distinct
// tokens stand in for distinct origins (whose persistent farbling tokens
// differ).
constexpr base::Token kTokenA(1234567890ULL, 9876543210ULL);
constexpr base::Token kTokenB(1111111111ULL, 2222222222ULL);

}  // namespace

using FarblingArgsTest = PlatformTest;

TEST_F(FarblingArgsTest, SameTokenProducesSameArgs) {
  EXPECT_EQ(MakeFarblingArgs(kTokenA), MakeFarblingArgs(kTokenA));
}

TEST_F(FarblingArgsTest, DifferentTokensProduceDifferentArgs) {
  EXPECT_NE(MakeFarblingArgs(kTokenA), MakeFarblingArgs(kTokenB));
}

// The generated arguments must contain every key expected by farbling.ts's
// `FarblingArgs` interface, with the documented value types.
TEST_F(FarblingArgsTest, ContainsExpectedKeys) {
  base::DictValue args = MakeFarblingArgs(kTokenA);

  std::optional<double> fudge_factor = args.FindDouble("fudgeFactor");
  ASSERT_TRUE(fudge_factor.has_value());
  EXPECT_GE(*fudge_factor, 0.99);
  EXPECT_LE(*fudge_factor, 1.0);

  EXPECT_NE(args.FindString("fakeVoiceName"), nullptr);
  EXPECT_NE(args.FindList("fakePluginData"), nullptr);

  std::optional<double> voice_scale = args.FindDouble("randomVoiceIndexScale");
  ASSERT_TRUE(voice_scale.has_value());
  EXPECT_GE(*voice_scale, 0.0);
  EXPECT_LT(*voice_scale, 1.0);

  std::optional<double> hardware_scale =
      args.FindDouble("randomHardwareIndexScale");
  ASSERT_TRUE(hardware_scale.has_value());
  EXPECT_GE(*hardware_scale, 0.0);
  EXPECT_LT(*hardware_scale, 1.0);
}

// When the plugin farbling feature is disabled, no fake plugins are generated.
TEST_F(FarblingArgsTest, NoFakePluginsWhenFeatureDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      features::kBraveIOSEnableFarblingPlugins);

  base::DictValue args = MakeFarblingArgs(kTokenA);
  const base::ListValue* plugins = args.FindList("fakePluginData");
  ASSERT_NE(plugins, nullptr);
  EXPECT_TRUE(plugins->empty());
}

// When the plugin farbling feature is enabled, 1 to 3 fake plugins are
// generated, each with 1 to 3 fake mime types.
TEST_F(FarblingArgsTest, GeneratesFakePluginsWhenFeatureEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      features::kBraveIOSEnableFarblingPlugins);

  base::DictValue args = MakeFarblingArgs(kTokenA);
  const base::ListValue* plugins = args.FindList("fakePluginData");
  ASSERT_NE(plugins, nullptr);
  EXPECT_GE(plugins->size(), 1u);
  EXPECT_LE(plugins->size(), 3u);

  for (const base::Value& plugin_value : *plugins) {
    const base::DictValue* plugin = plugin_value.GetIfDict();
    ASSERT_NE(plugin, nullptr);
    EXPECT_NE(plugin->FindString("name"), nullptr);
    EXPECT_NE(plugin->FindString("filename"), nullptr);
    EXPECT_NE(plugin->FindString("description"), nullptr);

    const base::ListValue* mime_types = plugin->FindList("mimeTypes");
    ASSERT_NE(mime_types, nullptr);
    EXPECT_GE(mime_types->size(), 1u);
    EXPECT_LE(mime_types->size(), 3u);

    for (const base::Value& mime_type_value : *mime_types) {
      const base::DictValue* mime_type = mime_type_value.GetIfDict();
      ASSERT_NE(mime_type, nullptr);
      const std::string* suffixes = mime_type->FindString("suffixes");
      ASSERT_NE(suffixes, nullptr);
      EXPECT_EQ(*suffixes, "pdf");
      const std::string* type = mime_type->FindString("type");
      ASSERT_NE(type, nullptr);
      EXPECT_EQ(*type, "application/pdf");
      EXPECT_NE(mime_type->FindString("description"), nullptr);
    }
  }
}

}  // namespace brave_shields
