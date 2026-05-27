/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_extension_handler.h"

#include <algorithm>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

bool IsKnownFakeExtension(const String& name) {
  const auto fake_extensions = GetFakeSupportedExtensionsForTesting();
  return std::ranges::find(fake_extensions, name) != fake_extensions.end();
}

}  // namespace

// CreateOffHandler

TEST(WebGLExtensionHandlerTest, OffHandler_ReturnsSameExtensions) {
  auto real = ExtensionVector({"OES_texture_float", "WEBGL_lose_context"});
  auto handler = CreateOffHandler(real);
  EXPECT_EQ(handler->GetSupportedExtensions(), real);
}

// CreateFarblingHandler (Maximum)

TEST(WebGLExtensionHandlerTest,
     MaximumHandler_WithDebugRendererInfo_ReturnsOnlyDebugInfo) {
  auto real = ExtensionVector(
      {"OES_texture_float", "WEBGL_debug_renderer_info", "WEBGL_lose_context"});
  auto handler = CreateMaximumHandler(real);
  const auto extensions = handler->GetSupportedExtensions();
  ASSERT_EQ(extensions.size(), 1u);
  EXPECT_EQ(extensions[0], String("WEBGL_debug_renderer_info"));
}

TEST(WebGLExtensionHandlerTest,
     MaximumHandler_WithoutDebugRendererInfo_ReturnsEmptyList) {
  auto real = ExtensionVector({"OES_texture_float", "WEBGL_lose_context"});
  auto handler = CreateMaximumHandler(real);
  EXPECT_TRUE(handler->GetSupportedExtensions().empty());
}

TEST(WebGLExtensionHandlerTest,
     MaximumHandler_EmptyRealExtensions_ReturnsEmpty) {
  auto handler = CreateMaximumHandler({});
  EXPECT_TRUE(handler->GetSupportedExtensions().empty());
}

// CreateBalancedHandler with feature disabled.

TEST(WebGLExtensionHandlerTest,
     BalancedHandler_FeatureDisabled_ReturnsSameExtensions) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      features::kWebGLBalancedFingerprintingProtection);

  auto real = ExtensionVector({"OES_texture_float", "WEBGL_lose_context"});
  auto handler = CreateBalancedHandler(real, /*seed=*/0);
  EXPECT_EQ(handler->GetSupportedExtensions(), real);
}

// CreateBalancedHandler with feature enabled.

TEST(WebGLExtensionHandlerTest, BalancedHandler_FeatureEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      features::kWebGLBalancedFingerprintingProtection);

  auto real = ExtensionVector({"OES_texture_float", "WEBGL_lose_context"});
  auto handler = CreateBalancedHandler(real, /*seed=*/0);
  const auto extensions = handler->GetSupportedExtensions();
  ASSERT_FALSE(extensions.empty());

  EXPECT_EQ(extensions.size(), real.size() + 1u);
  EXPECT_EQ(extensions[0], String("OES_texture_float"));
  EXPECT_EQ(extensions[1], String("WEBGL_lose_context"));
  EXPECT_TRUE(IsKnownFakeExtension(extensions.back()))
      << "Injected extension not from known fake set: " << extensions.back();
}

TEST(WebGLExtensionHandlerTest,
     BalancedHandler_FeatureEnabled_SeedDeterminesInjectedExtension) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      features::kWebGLBalancedFingerprintingProtection);

  auto real = ExtensionVector({"OES_texture_float"});
  auto fake = GetFakeSupportedExtensionsForTesting();

  // seed=0  → index 0  → "EXT_texture_sampler"
  auto handler0 = CreateBalancedHandler(real,
                                        /*seed=*/0);
  const auto ext0 = handler0->GetSupportedExtensions();
  ASSERT_EQ(ext0.size(), 2u);
  EXPECT_EQ(ext0.back(), fake[0]);

  // seed=1  → index 1  → "EXT_texture_compressor"
  auto handler1 = CreateBalancedHandler(real,
                                        /*seed=*/1);
  const auto ext1 = handler1->GetSupportedExtensions();
  ASSERT_EQ(ext1.size(), 2u);
  EXPECT_EQ(ext1.back(), fake[1]);

  // seed=20 → index 20 → "EXT_draw_blender"
  auto handler20 = CreateBalancedHandler(real,
                                         /*seed=*/20);
  const auto ext20 = handler20->GetSupportedExtensions();
  ASSERT_EQ(ext20.size(), 2u);
  EXPECT_EQ(ext20.back(), fake[20]);
}

TEST(WebGLExtensionHandlerTest,
     BalancedHandler_FeatureEnabled_SeedModuloWrapsAround) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      features::kWebGLBalancedFingerprintingProtection);

  auto real = ExtensionVector({"OES_texture_float"});

  // seed=0 and seed=kFakeListSize both map to index 0.
  auto handler0 = CreateBalancedHandler(real,
                                        /*seed=*/0);
  auto handler21 = CreateBalancedHandler(real,
                                         /*seed=*/kFakeExtensionsSize);
  EXPECT_EQ(handler0->GetSupportedExtensions().back(),
            handler21->GetSupportedExtensions().back());
}

TEST(WebGLExtensionHandlerTest,
     BalancedHandler_FeatureEnabled_EmptyRealExtensions_OneEntryAdded) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      features::kWebGLBalancedFingerprintingProtection);

  auto handler = CreateBalancedHandler(ExtensionVector{},
                                       /*seed=*/3);
  const auto extensions = handler->GetSupportedExtensions();
  ASSERT_EQ(extensions.size(), 1u);
  EXPECT_TRUE(IsKnownFakeExtension(extensions[0]));
}

}  // namespace blink
