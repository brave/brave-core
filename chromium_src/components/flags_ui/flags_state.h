/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_

// Inject an alias for this method so it can be called by this name from
// chromium_src/chrome/browser/about_flags.cc where a function with the same
// name is overridden, but internally calls the method in this class.
#define SetFeatureEntryEnabled                                          \
  SetFeatureEntryEnabled_ChromiumImpl(FlagsStorage* flags_storage,      \
                                      const std::string& internal_name, \
                                      bool enable) {                    \
    SetFeatureEntryEnabled(flags_storage, internal_name, enable);       \
  }                                                                     \
  void SetFeatureEntryEnabled

#include "../../../../components/flags_ui/flags_state.h"
#undef SetFeatureEntryEnabled

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_FLAGS_UI_FLAGS_STATE_H_
