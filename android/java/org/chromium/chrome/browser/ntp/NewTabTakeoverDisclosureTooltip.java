/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.content.Context;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltipUtils;

/** The New Tab Takeover disclosure tooltip, anchored above the "Sponsored" label. */
@NullMarked
class NewTabTakeoverDisclosureTooltip {
    static final String LEARN_MORE_URL =
            "https://support.brave.app/hc/en-us/articles/35182999599501";

    private static final int WIDTH_DP = 310;
    private static final int SCREEN_MARGIN_DP = 16;
    private static final int PADDING_DP = 16;
    private static final int ARROW_WIDTH_DP = 12;
    private static final int ARROW_HEIGHT_DP = 6;
    private static final int SHADOW_SPACE_DP = 20;
    private static final int LABEL_OVERLAP_DP = 9;
    private static final int FADE_IN_DURATION_MS = 500;

    interface Delegate {
        void onContentVisible(NewTabTakeoverDisclosureTooltip tooltip);

        void onWindowDismissed(NewTabTakeoverDisclosureTooltip tooltip);

        void onLearnMoreClicked(NewTabTakeoverDisclosureTooltip tooltip);
    }

    private final PopupWindowTooltip mTooltip;

    NewTabTakeoverDisclosureTooltip(Context context, View anchor, Delegate delegate) {
        int width =
                Math.min(
                        dpToPx(context, WIDTH_DP),
                        context.getResources().getDisplayMetrics().widthPixels
                                - 2 * dpToPx(context, SCREEN_MARGIN_DP));
        mTooltip =
                new PopupWindowTooltip.Builder(context)
                        .anchorView(anchor)
                        .contentView(R.layout.new_tab_takeover_disclosure_tooltip)
                        .gravity(Gravity.TOP)
                        .arrowColor(ContextCompat.getColor(context, R.color.container_background))
                        .dismissOnInsideTouch(false)
                        .dismissOnOutsideTouch(true)
                        .backgroundDimDisabled(true)
                        .padding((float) dpToPx(context, PADDING_DP))
                        .arrowWidth((float) dpToPx(context, ARROW_WIDTH_DP))
                        .arrowHeight((float) dpToPx(context, ARROW_HEIGHT_DP))
                        .parentPaddingHorizontal(dpToPx(context, SHADOW_SPACE_DP))
                        .parentPaddingVertical(dpToPx(context, SHADOW_SPACE_DP))
                        .margin((float) -dpToPx(context, LABEL_OVERLAP_DP + SHADOW_SPACE_DP))
                        .onShowListener(
                                tooltip -> {
                                    fadeInOnceContentIsVisible();
                                    delegate.onContentVisible(this);
                                })
                        .onDismissListener(tooltip -> delegate.onWindowDismissed(this))
                        .build();

        TextView textView = mTooltip.findViewById(R.id.new_tab_takeover_disclosure_tooltip_text);
        PopupWindowTooltipUtils.setWidth(textView, width);
        textView.setText(buildText(context, () -> delegate.onLearnMoreClicked(this)));
        textView.setMovementMethod(LinkMovementMethod.getInstance());
    }

    void show() {
        mTooltip.show();
    }

    void dismiss() {
        mTooltip.dismiss();
    }

    boolean isWindowShowing() {
        return mTooltip.isShowing();
    }

    private void fadeInOnceContentIsVisible() {
        ViewGroup contentLayout = mTooltip.getContentLayout();
        for (int i = 0; i < contentLayout.getChildCount(); ++i) {
            contentLayout.getChildAt(i).setAlpha(0f);
        }
        contentLayout.post(
                () -> {
                    for (int i = 0; i < contentLayout.getChildCount(); ++i) {
                        contentLayout
                                .getChildAt(i)
                                .animate()
                                .alpha(1f)
                                .setDuration(FADE_IN_DURATION_MS);
                    }
                });
    }

    private static CharSequence buildText(Context context, Runnable onLearnMoreClicked) {
        String learnMore =
                context.getString(R.string.new_tab_takeover_disclosure_tooltip_learn_more);
        String text =
                context.getString(R.string.new_tab_takeover_disclosure_tooltip_text, learnMore);
        SpannableString spannable = new SpannableString(text);
        int start = text.indexOf(learnMore);
        if (start >= 0) {
            spannable.setSpan(
                    new ClickableSpan() {
                        @Override
                        public void onClick(View widget) {
                            onLearnMoreClicked.run();
                        }
                    },
                    start,
                    start + learnMore.length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return spannable;
    }
}
