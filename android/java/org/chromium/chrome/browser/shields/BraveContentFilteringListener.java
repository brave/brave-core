/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

public interface BraveContentFilteringListener {
    public void onAddSubscriptionFilter();

    public void onSubscriptionFilterToggle(int position, boolean isEnable);

    public void onSubscriptionFilterRefresh(int position);

    public void onSubscriptionFilterDelete(int position);

    public void onFilterToggle(String uuid, boolean isEnable);

    public void onCustomFilters();
}
