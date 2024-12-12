/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/toasts/api/toast_id.h"

// All of the upstream unit tests use kLinkCopied or kImageCopied when creating
// test toasts, but those are the two notifications we want to hide so redefine
// their identifiers here so we can continue to run the upstream tests.
#define kLinkCopied kLinkToHighlightCopied
#define kImageCopied kClearBrowsingData
#include "src/chrome/browser/ui/toasts/toast_controller_unittest.cc"
#undef kImageCopied
#undef kLinkCopied

TEST_F(ToastControllerUnitTest, NeverShowToastForLinkCopied) {
  ToastRegistry* const registry = toast_registry();
  registry->RegisterToast(
      ToastId::kLinkCopied,
      ToastSpecification::Builder(vector_icons::kEmailIcon, 0).Build());

  auto controller = std::make_unique<TestToastController>(registry);

  EXPECT_FALSE(controller->IsShowingToast());
  EXPECT_TRUE(controller->CanShowToast(ToastId::kLinkCopied));
  EXPECT_FALSE(controller->MaybeShowToast(ToastParams(ToastId::kLinkCopied)));
  EXPECT_FALSE(controller->IsShowingToast());
}

TEST_F(ToastControllerUnitTest, NeverShowToastForImageCopied) {
  ToastRegistry* const registry = toast_registry();
  registry->RegisterToast(
      ToastId::kImageCopied,
      ToastSpecification::Builder(vector_icons::kEmailIcon, 0).Build());

  auto controller = std::make_unique<TestToastController>(registry);

  EXPECT_FALSE(controller->IsShowingToast());
  EXPECT_TRUE(controller->CanShowToast(ToastId::kImageCopied));
  EXPECT_FALSE(controller->MaybeShowToast(ToastParams(ToastId::kImageCopied)));
  EXPECT_FALSE(controller->IsShowingToast());
}

TEST_F(ToastControllerUnitTest, NeverShowToastForAddedToReadingList) {
  ToastRegistry* const registry = toast_registry();
  registry->RegisterToast(
      ToastId::kAddedToReadingList,
      ToastSpecification::Builder(vector_icons::kEmailIcon, 0).Build());

  auto controller = std::make_unique<TestToastController>(registry);

  EXPECT_FALSE(controller->IsShowingToast());
  EXPECT_TRUE(controller->CanShowToast(ToastId::kAddedToReadingList));
  EXPECT_FALSE(
      controller->MaybeShowToast(ToastParams(ToastId::kAddedToReadingList)));
  EXPECT_FALSE(controller->IsShowingToast());
}
