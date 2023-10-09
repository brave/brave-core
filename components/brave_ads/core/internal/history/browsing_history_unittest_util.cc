/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/browsing_history_unittest_util.h"

namespace brave_ads::test {

BrowsingHistoryList BuildBrowsingHistory() {
  return {GURL("https://www.foobar.com"), GURL("https://www.foo.com"),
          GURL("https://www.bar.com"),    GURL("https://www.baz.com"),
          GURL("https://www.qux.com"),    GURL("https://www.quux.com"),
          GURL("https://www.corge.com"),  GURL("https://www.grault.com"),
          GURL("https://www.garply.com"), GURL("https://www.waldo.com"),
          GURL("https://www.fred.com"),   GURL("https://www.plugh.com"),
          GURL("https://www.xyzzy.com"),  GURL("https://www.thud.com")};
}

}  // namespace brave_ads::test
