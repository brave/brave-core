/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/find_bar_host.cc"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
views::Widget* FindBarHost::GetHostWidget() {
  return host();
}
#endif
