/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.text.Layout;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

import java.util.Arrays;
import java.util.List;

@NullMarked
public class AlwaysOnPagerAdapter extends PagerAdapter {
    private final Context mContext;

    private static final String AUTO_RECONNECT_VPN_LINK =
            "https://support.brave.com/hc/en-us/articles/29918727663373-How-do-I-connect-to-Brave-VPN-automatically#tab-2";

    private final List<Integer> mImageResources =
            Arrays.asList(
                    R.drawable.ic_vpn_always_on_1,
                    R.drawable.ic_vpn_always_on_2,
                    R.drawable.ic_vpn_always_on_3);
    private final List<Integer> mTexts =
            Arrays.asList(
                    R.string.auto_reconnect_vpn_tutorial_text_1,
                    R.string.auto_reconnect_vpn_tutorial_text_2,
                    R.string.auto_reconnect_vpn_tutorial_text_3,
                    R.string.auto_reconnect_vpn_tutorial_text_4);

    public AlwaysOnPagerAdapter(Context context) {
        this.mContext = context;
    }

    @NonNull
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        @SuppressLint("InflateParams")
        View view =
                LayoutInflater.from(mContext)
                        .inflate(R.layout.auto_reconnect_vpn_tutorial_item_layout, null);
        TextView autoReconnectVpnTutorialText =
                view.findViewById(R.id.auto_reconnect_vpn_tutorial_text);
        String autoReconnectVpnText = mContext.getResources().getString(mTexts.get(position));
        TextView autoReconnectVpnTutorialWarningText =
                view.findViewById(R.id.auto_reconnect_vpn_tutorial_warning_text);
        if (position == 2) {
            SpannableString tutorialSpannableString =
                    SpanApplier.applySpans(
                            autoReconnectVpnText,
                            new SpanInfo(
                                    "<auto_reconnect_vpn_tutorial>",
                                    "</auto_reconnect_vpn_tutorial>",
                                    new StyleSpan(android.graphics.Typeface.BOLD)));
            autoReconnectVpnTutorialText.setText(tutorialSpannableString);

            String autoReconnectVpnWarningText =
                    mContext.getResources().getString(mTexts.get(position + 1));
            ChromeClickableSpan learnMoreClickableSpan =
                    new ChromeClickableSpan(
                            mContext.getColor(R.color.brave_blue_tint_color),
                            (textView) -> {
                                CustomTabActivity.showInfoPage(mContext, AUTO_RECONNECT_VPN_LINK);
                            });
            SpannableString learnMoreSpannableString =
                    SpanApplier.applySpans(
                            autoReconnectVpnWarningText,
                            new SpanInfo("<learn_more>", "</learn_more>", learnMoreClickableSpan));

            autoReconnectVpnTutorialWarningText.setMovementMethod(LinkMovementMethod.getInstance());
            autoReconnectVpnTutorialWarningText.setText(learnMoreSpannableString);
            autoReconnectVpnTutorialWarningText.post(
                    () -> {
                        Layout layout = autoReconnectVpnTutorialWarningText.getLayout();
                        if (layout != null) {
                            int desiredHeight =
                                    layout.getLineTop(
                                            autoReconnectVpnTutorialWarningText.getLineCount());
                            ViewGroup.LayoutParams params =
                                    autoReconnectVpnTutorialWarningText.getLayoutParams();
                            params.height = desiredHeight;
                            autoReconnectVpnTutorialWarningText.setLayoutParams(params);
                            autoReconnectVpnTutorialWarningText.setVisibility(View.VISIBLE);
                        }
                    });
        } else {
            autoReconnectVpnTutorialText.setText(autoReconnectVpnText);
        }

        ImageView autoReconnectVpnTutorialImage =
                view.findViewById(R.id.auto_reconnect_vpn_tutorial_image);
        int autoReconnectVpnImage = mImageResources.get(position);
        autoReconnectVpnTutorialImage.setImageResource(autoReconnectVpnImage);

        collection.addView(view);
        return view;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, @NonNull Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return 3;
    }

    @Override
    public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
        return view == object;
    }
}
