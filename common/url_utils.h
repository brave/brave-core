/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

namespace brave {
  // Given a chrome:// url string this method will return a brave:// url string
  // This method wouldn't support nested schemes such as rss:chrome://
  base::string16 ReplaceChromeSchemeWithBrave(const base::string16 url);
}
