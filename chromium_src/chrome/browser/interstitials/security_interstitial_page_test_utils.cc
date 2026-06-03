/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/strings/string_util.h"
#include "content/public/test/browser_test_utils.h"

namespace {

// We changed the upstream text to show Private instead of Incognito. Modify
// expectation to work in the Brave's reality.
std::string ChangeIncognitoToPrivate(const std::string& str) {
  std::string result = str;
  base::ReplaceFirstSubstringAfterOffset(&result, 0, "Incognito", "Private");
  return result;
}

}  // namespace

#define EvalJs(rfh, script) EvalJs(rfh, ChangeIncognitoToPrivate(script))

#include <chrome/browser/interstitials/security_interstitial_page_test_utils.cc>

#undef EvalJs
