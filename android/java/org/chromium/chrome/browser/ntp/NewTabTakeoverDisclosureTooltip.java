/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.content.res.Resources;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.view.Gravity;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltipUtils;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

/** The New Tab Takeover disclosure tooltip, anchored above the "Sponsored" label. */
@NullMarked
class NewTabTakeoverDisclosureTooltip {
    static final String LEARN_MORE_URL =
            "https://support.brave.app/hc/en-us/articles/35182999599501";

    interface Delegate {
        void onPopupWindowTooltipDismissed(NewTabTakeoverDisclosureTooltip tooltip);

        void onLearnMoreClicked(NewTabTakeoverDisclosureTooltip tooltip);
    }

    private final PopupWindowTooltip mTooltip;
    private final View mDisclosureLabel;
    private final ViewTreeObserver.OnPreDrawListener mFollowDisclosureLabelListener;
    private @Nullable ViewTreeObserver mDisclosureLabelViewTreeObserver;

    NewTabTakeoverDisclosureTooltip(Context context, View disclosureLabel, Delegate delegate) {
        mDisclosureLabel = disclosureLabel;
        Resources resources = context.getResources();
        int maxWidth =
                resources.getDimensionPixelSize(R.dimen.new_tab_takeover_disclosure_tooltip_width);
        int screenMargin =
                resources.getDimensionPixelSize(
                        R.dimen.new_tab_takeover_disclosure_tooltip_screen_margin);
        int width =
                Math.min(maxWidth, resources.getDisplayMetrics().widthPixels - 2 * screenMargin);
        int padding =
                resources.getDimensionPixelSize(
                        R.dimen.new_tab_takeover_disclosure_tooltip_padding);
        int arrowWidth =
                resources.getDimensionPixelSize(
                        R.dimen.new_tab_takeover_disclosure_tooltip_arrow_width);
        int arrowHeight =
                resources.getDimensionPixelSize(
                        R.dimen.new_tab_takeover_disclosure_tooltip_arrow_height);
        mTooltip =
                new PopupWindowTooltip.Builder(context)
                        .anchorView(disclosureLabel)
                        .contentView(R.layout.new_tab_takeover_disclosure_tooltip)
                        .gravity(Gravity.TOP)
                        .arrowColor(ContextCompat.getColor(context, R.color.container_background))
                        .dismissOnInsideTouch(false)
                        .dismissOnOutsideTouch(true)
                        .backgroundDimDisabled(true)
                        .padding((float) padding)
                        .arrowWidth((float) arrowWidth)
                        .arrowHeight((float) arrowHeight)
                        .margin(0f)
                        .onDismissListener(
                                tooltip -> {
                                    stopFollowingDisclosureLabel();
                                    delegate.onPopupWindowTooltipDismissed(this);
                                })
                        .build();
        mFollowDisclosureLabelListener =
                () -> {
                    mTooltip.updateLocation();
                    return true;
                };

        TextView textView = mTooltip.findViewById(R.id.new_tab_takeover_disclosure_tooltip_text);
        PopupWindowTooltipUtils.setWidth(textView, width);
        textView.setText(buildText(context, () -> delegate.onLearnMoreClicked(this)));
        textView.setMovementMethod(LinkMovementMethod.getInstance());
    }

    void show() {
        ViewTreeObserver observer = mDisclosureLabel.getViewTreeObserver();
        observer.addOnPreDrawListener(mFollowDisclosureLabelListener);
        mDisclosureLabelViewTreeObserver = observer;
        mTooltip.show();
    }

    void dismiss() {
        stopFollowingDisclosureLabel();
        mTooltip.dismiss();
    }

    private void stopFollowingDisclosureLabel() {
        ViewTreeObserver observer = mDisclosureLabelViewTreeObserver;
        mDisclosureLabelViewTreeObserver = null;
        if (observer != null && observer.isAlive()) {
            observer.removeOnPreDrawListener(mFollowDisclosureLabelListener);
        }
    }

    private static CharSequence buildText(Context context, Runnable onLearnMoreClicked) {
        return SpanApplier.applySpans(
                context.getString(R.string.new_tab_takeover_disclosure_tooltip_text),
                new SpanInfo(
                        "<link>",
                        "</link>",
                        new ClickableSpan() {
                            @Override
                            public void onClick(View widget) {
                                onLearnMoreClicked.run();
                            }
                        }));
    }
}
