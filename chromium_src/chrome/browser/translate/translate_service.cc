/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/translate/buildflags/buildflags.h"

namespace {
bool IsBraveTranslateEnabled() {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return true;
#endif
  return false;
}
}  // namespace

#include "../../../../../../chrome/browser/translate/translate_service.cc" // NOLINT
