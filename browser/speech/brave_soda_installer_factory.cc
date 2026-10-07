/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Implements the hook that the chromium_src override of
// `chrome/browser/browser_process_impl.cc` forward declares.

#include <memory>

#include "brave/browser/speech/brave_soda_installer.h"
#include "components/soda/soda_installer.h"

namespace speech {

std::unique_ptr<SodaInstaller> CreateBraveSodaInstaller() {
  return std::make_unique<BraveSodaInstaller>();
}

}  // namespace speech
