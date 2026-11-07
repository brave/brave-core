/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.prefs.PrefService;

import java.util.function.BooleanSupplier;

/**
 * Decides when the New Tab Takeover disclosure tooltip auto-displays. Mirrors
 * new_tab_takeover_disclosure_tooltip_util.cc, which shares the infobar's remaining display count.
 */
@NullMarked
class NewTabTakeoverDisclosureTooltipController {
    private final PrefService mPrefService;
    private final BooleanSupplier mIsRewardsEnabled;

    private boolean mHasAutoDisplayed;

    NewTabTakeoverDisclosureTooltipController(
            PrefService prefService, BooleanSupplier isRewardsEnabled) {
        mPrefService = prefService;
        mIsRewardsEnabled = isRewardsEnabled;
    }

    boolean shouldAutoDisplay() {
        return !mHasAutoDisplayed && canAutoDisplay();
    }

    void onAutoDisplayed() {
        mHasAutoDisplayed = true;
        if (!canAutoDisplay()) {
            return;
        }
        decrementRemainingDisplayCount();
    }

    void onLearnMoreClicked() {
        mPrefService.setInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT, 0);
    }

    private boolean canAutoDisplay() {
        return !mIsRewardsEnabled.getAsBoolean() && getRemainingDisplayCount() > 0;
    }

    private void decrementRemainingDisplayCount() {
        mPrefService.setInteger(
                BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT,
                getRemainingDisplayCount() - 1);
    }

    private int getRemainingDisplayCount() {
        return mPrefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT);
    }
}
