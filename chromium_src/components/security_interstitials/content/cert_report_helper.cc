/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/content/cert_report_helper.h"

#define BRAVE_CERT_REPORT_HELPER_SHOULD_SHOW_ENHANCED_PROTECTION_MESSAGE \
  return false;
#include "../../../../../components/security_interstitials/content/cert_report_helper.cc"
#undef BRAVE_CERT_REPORT_HELPER_SHOULD_SHOW_ENHANCED_PROTECTION_MESSAGE
