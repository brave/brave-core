/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.util;

import java.util.List;

import org.chromium.chrome.browser.ntp_background_images.model.TopSite;

public interface NewTabPageListener {
	void updateInteractableFlag(boolean isFromBottomSheet);
	void updateNTPImage();
	void updateTopSites(List<TopSite> topSites);
	void selectedPage(int index);
}