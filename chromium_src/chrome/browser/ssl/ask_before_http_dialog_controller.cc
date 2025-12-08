/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/ask_before_http_dialog_controller.h"

// Prevent re-defining OpenGURL
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

namespace {

constexpr char kBraveLearnMoreLink[] =
    "https://support.brave.app/hc/en-us/articles/15513090104717";

}  // namespace

#define OpenGURL(URL, ...) OpenGURL(GURL(kBraveLearnMoreLink), __VA_ARGS__)

#include <chrome/browser/ssl/ask_before_http_dialog_controller.cc>
#undef OpenGURL
