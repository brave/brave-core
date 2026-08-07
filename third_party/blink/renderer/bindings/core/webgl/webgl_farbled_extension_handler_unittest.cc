/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_farbled_extension_handler.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"

namespace blink {

TEST(WebGLExtensionHandlerTest, SeedDeterminesInjectedExtension) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kWebGLBalancedFingerprintingProtection);

  auto fake = GetFakeSupportedExtensionsForTesting();
  auto handler0 = WebGLFarbledExtensionHandler::CreateHandler(
      /*seed=*/0);
  EXPECT_EQ(fake[0].name, handler0->GetExtensionName());
  EXPECT_EQ(fake[0].script_object_name, handler0->GetExtensionObjectName());

  auto handler1 = WebGLFarbledExtensionHandler::CreateHandler(
      /*seed=*/1);
  EXPECT_EQ(fake[1].name, handler1->GetExtensionName());
  EXPECT_EQ(fake[1].script_object_name, handler1->GetExtensionObjectName());

  auto handler20 = WebGLFarbledExtensionHandler::CreateHandler(
      /*seed=*/20);
  EXPECT_EQ(fake[20].name, handler20->GetExtensionName());
  EXPECT_EQ(fake[20].script_object_name, handler20->GetExtensionObjectName());
}

TEST(WebGLExtensionHandlerTest, SeedModuloWrapsAround) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      blink::features::kWebGLBalancedFingerprintingProtection);

  auto fake = GetFakeSupportedExtensionsForTesting();

  // seed=0 and seed=kFakeListSize both map to index 0.
  auto handler0 = WebGLFarbledExtensionHandler::CreateHandler(
      /*seed=*/0);
  auto handler_last = WebGLFarbledExtensionHandler::CreateHandler(
      /*seed=*/fake.size());

  EXPECT_EQ(handler0->GetExtensionName(), handler_last->GetExtensionName());
}

}  // namespace blink
