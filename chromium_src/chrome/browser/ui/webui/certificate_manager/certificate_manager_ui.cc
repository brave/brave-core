/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/certificate_manager/certificate_manager_ui.h"

#include "content/public/browser/web_ui_data_source.h"

namespace {

const char kBraveCRSLearnMoreLink[] =
    "https://github.com/brave/brave-browser/wiki/TLS-Policy#root-store";

}  // namespace

// Decay arrays to pointers for == comparison.
#define AddString(NAME, VALUE) \
  AddString(NAME, +VALUE == +kCRSLearnMoreLink ? kBraveCRSLearnMoreLink : VALUE)

#include "src/chrome/browser/ui/webui/certificate_manager/certificate_manager_ui.cc"
#undef AddString
