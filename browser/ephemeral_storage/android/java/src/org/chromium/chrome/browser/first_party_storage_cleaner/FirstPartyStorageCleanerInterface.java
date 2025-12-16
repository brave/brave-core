/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.first_party_storage_cleaner;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public interface FirstPartyStorageCleanerInterface {
    void shredSiteData();
}
