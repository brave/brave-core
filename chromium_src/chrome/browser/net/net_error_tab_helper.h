/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_NET_ERROR_TAB_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_NET_ERROR_TAB_HELPER_H_

#define ProbesAllowed ProbesAllowed_ChromiumImpl() const; \
  virtual bool ProbesAllowed
#include "../../../../../chrome/browser/net/net_error_tab_helper.h"
#undef ProbesAllowed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_NET_NET_ERROR_TAB_HELPER_H_