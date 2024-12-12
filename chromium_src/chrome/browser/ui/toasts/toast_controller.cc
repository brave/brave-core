/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/toasts/toast_controller.h"

#define MaybeShowToast MaybeShowToast_ChromiumImpl
#include "src/chrome/browser/ui/toasts/toast_controller.cc"
#undef MaybeShowToast

bool ToastController::MaybeShowToast(ToastParams params) {
  if (params.toast_id == ToastId::kLinkCopied ||
      params.toast_id == ToastId::kImageCopied) {
    return false;
  }
  return MaybeShowToast_ChromiumImpl(std::move(params));
}
