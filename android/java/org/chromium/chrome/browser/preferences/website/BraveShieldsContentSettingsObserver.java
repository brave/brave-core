/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

/** Allows monitoring of blocked resources via brave shields. */
public interface BraveShieldsContentSettingsObserver {
    public void blockEvent(int tabId, String blockType, String subresource);

    public void savedBandwidth(long savings);
}
