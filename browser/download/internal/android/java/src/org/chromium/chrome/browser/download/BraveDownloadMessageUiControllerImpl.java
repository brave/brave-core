/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download;

import android.text.TextUtils;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.components.offline_items_collection.LegacyHelpers;
import org.chromium.components.offline_items_collection.OfflineItem;

public class BraveDownloadMessageUiControllerImpl {
    public boolean isVisibleToUser(OfflineItem offlineItem) {
        if (offlineItem.isTransient || offlineItem.isSuggested || offlineItem.isDangerous) {
            return false;
        }

        if (LegacyHelpers.isLegacyDownload(offlineItem.id)
                && TextUtils.isEmpty(offlineItem.filePath)) {
            return false;
        }

        return ContextUtils.getAppSharedPreferences().getBoolean(
                BravePreferenceKeys.BRAVE_DOWNLOADS_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE, false);
    }
}
