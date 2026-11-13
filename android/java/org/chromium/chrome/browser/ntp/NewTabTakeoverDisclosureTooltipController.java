/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.view.View;
import android.view.ViewTreeObserver;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.prefs.PrefService;

import java.util.function.BooleanSupplier;

/**
 * Shows the New Tab Takeover disclosure tooltip above the "Sponsored" label, and auto-displays it
 * while displays remain. The display count mirrors new_tab_takeover_disclosure_tooltip_util.cc.
 */
@NullMarked
class NewTabTakeoverDisclosureTooltipController {
    static final long AUTO_DISPLAY_DELAY_MS = 200;

    interface TooltipFactory {
        NewTabTakeoverDisclosureTooltip create(
                Context context,
                View disclosureLabel,
                NewTabTakeoverDisclosureTooltip.Delegate delegate);
    }

    private final View mRootView;
    private final PrefService mPrefService;
    private final BooleanSupplier mIsRewardsEnabled;
    private final TooltipFactory mTooltipFactory;

    private final NewTabTakeoverDisclosureTooltip.Delegate mTooltipDelegate =
            new NewTabTakeoverDisclosureTooltip.Delegate() {
                @Override
                public void onPopupWindowTooltipDismissed(NewTabTakeoverDisclosureTooltip tooltip) {
                    if (tooltip != mTooltip) {
                        return;
                    }
                    dismiss();
                }

                @Override
                public void onLearnMoreClicked(NewTabTakeoverDisclosureTooltip tooltip) {
                    dismiss();
                    suppressAutoDisplay();
                    TabUtils.openUrlInSameTab(NewTabTakeoverDisclosureTooltip.LEARN_MORE_URL);
                }
            };

    private @Nullable NewTabTakeoverDisclosureTooltip mTooltip;
    private ViewTreeObserver.@Nullable OnGlobalLayoutListener mAutoDisplayGlobalLayoutListener;
    private @Nullable Runnable mDelayedAutoDisplayRunnable;

    NewTabTakeoverDisclosureTooltipController(
            View rootView,
            PrefService prefService,
            BooleanSupplier isRewardsEnabled,
            TooltipFactory tooltipFactory) {
        mRootView = rootView;
        mPrefService = prefService;
        mIsRewardsEnabled = isRewardsEnabled;
        mTooltipFactory = tooltipFactory;

        if (canAutoDisplay()) {
            ViewTreeObserver.OnGlobalLayoutListener listener =
                    () -> {
                        if (findDisclosureLabelIfVisible() == null) {
                            return;
                        }

                        removeAutoDisplayGlobalLayoutListener();
                        Runnable delayedAutoDisplayRunnable = this::autoDisplay;
                        mDelayedAutoDisplayRunnable = delayedAutoDisplayRunnable;
                        mRootView.postDelayed(delayedAutoDisplayRunnable, AUTO_DISPLAY_DELAY_MS);
                    };
            mAutoDisplayGlobalLayoutListener = listener;
            mRootView.getViewTreeObserver().addOnGlobalLayoutListener(listener);
        }
    }

    void onDisclosureLabelClicked(View disclosureLabel) {
        if (isTooltipShown()) {
            return;
        }

        openTooltip(disclosureLabel);
    }

    void dismiss() {
        removeAutoDisplayGlobalLayoutListener();
        removeDelayedAutoDisplayRunnable();

        NewTabTakeoverDisclosureTooltip tooltip = mTooltip;
        if (tooltip == null) {
            return;
        }

        mTooltip = null;
        tooltip.dismiss();
    }

    private boolean isTooltipShown() {
        return mTooltip != null;
    }

    private @Nullable View findDisclosureLabelIfVisible() {
        View disclosureLabel = mRootView.findViewById(R.id.sponsored_label);
        if (disclosureLabel == null
                || !disclosureLabel.isShown()
                || !disclosureLabel.isLaidOut()
                || disclosureLabel.getAlpha() <= 0f) {
            return null;
        }
        return disclosureLabel;
    }

    private void openTooltip(View disclosureLabel) {
        NewTabTakeoverDisclosureTooltip tooltip =
                mTooltipFactory.create(
                        disclosureLabel.getContext(), disclosureLabel, mTooltipDelegate);
        mTooltip = tooltip;
        tooltip.show();
    }

    private void autoDisplay() {
        removeDelayedAutoDisplayRunnable();

        View disclosureLabel = findDisclosureLabelIfVisible();
        if (disclosureLabel == null || !canAutoDisplay()) {
            return;
        }

        recordAutoDisplay();
        if (!isTooltipShown()) {
            openTooltip(disclosureLabel);
        }
    }

    private void removeAutoDisplayGlobalLayoutListener() {
        ViewTreeObserver.OnGlobalLayoutListener listener = mAutoDisplayGlobalLayoutListener;
        if (listener == null) {
            return;
        }
        mAutoDisplayGlobalLayoutListener = null;
        mRootView.getViewTreeObserver().removeOnGlobalLayoutListener(listener);
    }

    private void removeDelayedAutoDisplayRunnable() {
        Runnable delayedAutoDisplayRunnable = mDelayedAutoDisplayRunnable;
        if (delayedAutoDisplayRunnable == null) {
            return;
        }
        mDelayedAutoDisplayRunnable = null;
        mRootView.removeCallbacks(delayedAutoDisplayRunnable);
    }

    // TODO(https://github.com/brave/brave-browser/issues/58801): Switch to
    // `kNewTabTakeoverDisclosureTooltipRemainingDisplayCount` once the New Tab
    // Takeover infobar is removed on all platforms.
    private void recordAutoDisplay() {
        mPrefService.setInteger(
                BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT,
                getRemainingDisplayCount() - 1);
    }

    private void suppressAutoDisplay() {
        mPrefService.setInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT, 0);
    }

    private boolean canAutoDisplay() {
        return !mIsRewardsEnabled.getAsBoolean() && getRemainingDisplayCount() > 0;
    }

    private int getRemainingDisplayCount() {
        return mPrefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT);
    }
}
