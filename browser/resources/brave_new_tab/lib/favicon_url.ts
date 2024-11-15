/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function faviconURL(url: string) {
  return 'chrome://favicon2/?size=64&pageUrl=' + encodeURIComponent(url)
}
