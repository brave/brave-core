/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import androidx.annotation.DrawableRes;

import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.R;

public class BraveClearBrowsingDataPreferencesAdvanced extends ClearBrowsingDataFragmentAdvanced {
    private static final int CLEAR_DOWNLOADS = DialogOption.NUM_ENTRIES;

    // TODO(samartnik): disabling option as it requires futher research
    // At the moment causes issue https://github.com/brave/brave-browser/issues/8737
    // @Override
    // protected List<Integer> getDialogOptions() {
    //     return Arrays.asList(DialogOption.CLEAR_HISTORY, DialogOption.CLEAR_COOKIES_AND_SITE_DATA,
    //             DialogOption.CLEAR_CACHE, DialogOption.CLEAR_PASSWORDS,
    //             DialogOption.CLEAR_FORM_DATA, DialogOption.CLEAR_SITE_SETTINGS, CLEAR_DOWNLOADS);
    // }

    // TODO(samartnik): disabling option as it requires futher research
    // At the moment causes issue https://github.com/brave/brave-browser/issues/8737
    // @Override
    // protected Set<Integer> getAllOptions() {
    //     Set<Integer> all = super.getAllOptions();
    //     all.add(CLEAR_DOWNLOADS);
    //     return all;
    // }

    static boolean isValidDataType(int type) {
        // TODO(samartnik): disabling option as it requires futher research
        // At the moment causes issue https://github.com/brave/brave-browser/issues/8737
        // return type == CLEAR_DOWNLOADS;
        return false;
    }

    static @BrowsingDataType int getDataType(int type) {
        switch (type) {
            case CLEAR_DOWNLOADS:
                return BrowsingDataType.DOWNLOADS;
            default:
                throw new IllegalArgumentException();
        }
    }

    static String getPreferenceKey(int type) {
        switch (type) {
            case CLEAR_DOWNLOADS:
                return "clear_downloads_checkbox";
            default:
                throw new IllegalArgumentException();
        }
    }

    static @DrawableRes int getIcon(int type) {
        switch (type) {
            case CLEAR_DOWNLOADS:
                return R.drawable.ic_file_download_24dp;
            default:
                throw new IllegalArgumentException();
        }
    }
}
