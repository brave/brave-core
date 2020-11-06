/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/net/net_error_tab_helper.h"

#define ProbesAllowed ProbesAllowed_ChromiumImpl
#include "../../../../../chrome/browser/net/net_error_tab_helper.cc"
#undef ProbesAllowed

namespace chrome_browser_net {

bool NetErrorTabHelper::ProbesAllowed() const {
  return false;
}

}  // namespace chrome_browser_net