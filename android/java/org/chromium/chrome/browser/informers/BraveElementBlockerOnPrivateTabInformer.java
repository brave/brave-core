/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.informers;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.cosmetic_filters.BraveCosmeticFiltersUtils;
import org.chromium.chrome.browser.infobar.BraveInfoBarIdentifier;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.infobar.BraveSimpleConfirmInfoBarBuilder;
import org.chromium.chrome.browser.ui.messages.infobar.SimpleConfirmInfoBarBuilder;

@NullMarked
public class BraveElementBlockerOnPrivateTabInformer {
    private static final String TAG = "ElBlockerPrivTabInfo";

    public static void show(Tab currentActiveTab) {
        try {
            if (currentActiveTab == null) {
                Log.e(TAG, "currentActiveTab is null");
                return;
            }

            BraveActivity activity = BraveActivity.getBraveActivity();
            BraveSimpleConfirmInfoBarBuilder.createInfobarWithDrawable(
                    currentActiveTab.getWebContents(),
                    new SimpleConfirmInfoBarBuilder.Listener() {
                        @Override
                        public void onInfoBarDismissed() {}

                        @Override
                        public boolean onInfoBarButtonClicked(boolean isPrimary) {
                            assert isPrimary : "We don't have secondary button";
                            showElementBlocker(currentActiveTab);
                            return false;
                        }

                        @Override
                        public boolean onInfoBarLinkClicked() {
                            return false;
                        }
                    },
                    BraveInfoBarIdentifier.BRAVE_ELEMENT_BLOCKER_ON_PRIVATE_TAB_INFOBAR,
                    activity,
                    R.drawable.ic_warning_circle,
                    activity.getString(R.string.brave_allow_element_blocker_in_private_summary, ""),
                    activity.getString(R.string.ok),
                    "",
                    "",
                    false);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "show " + e);
        }
    }

    private static void showElementBlocker(Tab currentActiveTab) {
        boolean success =
                BraveCosmeticFiltersUtils.launchContentPickerForWebContent(currentActiveTab);
        if (!success) {
            Log.w(TAG, "Failed to launch content picker for web content");
        }
    }
}
