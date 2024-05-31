/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.chrome.R;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

import java.util.Arrays;
import java.util.List;

public class AlwaysOnPagerAdapter extends PagerAdapter {
    private Context mContext;
    private List<Integer> mImageResources =
            Arrays.asList(
                    R.drawable.ic_vpn_always_on_1,
                    R.drawable.ic_vpn_always_on_2,
                    R.drawable.ic_vpn_always_on_3);
    private List<Integer> mTexts =
            Arrays.asList(
                    R.string.kill_switch_tutorial_text_1,
                    R.string.kill_switch_tutorial_text_2,
                    R.string.kill_switch_tutorial_text_3);
    private boolean mIsKillSwitch;

    public AlwaysOnPagerAdapter(Context context, boolean isKillSwitch) {
        this.mContext = context;
        this.mIsKillSwitch = isKillSwitch;
    }

    @NonNull
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        @SuppressLint("InflateParams")
        View view =
                LayoutInflater.from(mContext)
                        .inflate(R.layout.kill_switch_tutorial_item_layout, null);
        TextView killSwitchTutorialText = view.findViewById(R.id.kill_switch_tutorial_text);
        String killSwitchText = mContext.getResources().getString(mTexts.get(position));
        if (position == 2) {
            SpannableString tutorialSpannableString =
                    SpanApplier.applySpans(
                            killSwitchText,
                            new SpanInfo(
                                    "<always_on_tutorial>",
                                    "</always_on_tutorial>",
                                    null,
                                    new StyleSpan(android.graphics.Typeface.BOLD)));
            if (mIsKillSwitch) {
                killSwitchText =
                        killSwitchText
                                + "\n"
                                + mContext.getResources()
                                        .getString(R.string.kill_switch_tutorial_text_4);
                tutorialSpannableString =
                        SpanApplier.applySpans(
                                killSwitchText,
                                new SpanInfo(
                                        "<always_on_tutorial>",
                                        "</always_on_tutorial>",
                                        null,
                                        new StyleSpan(android.graphics.Typeface.BOLD)),
                                new SpanInfo(
                                        "<always_on_tutorial_2>",
                                        "</always_on_tutorial_2>",
                                        null,
                                        new StyleSpan(android.graphics.Typeface.BOLD)));
            }
            killSwitchTutorialText.setText(tutorialSpannableString);
        } else {
            killSwitchTutorialText.setText(killSwitchText);
        }

        ImageView killSwitchTutorialImage = view.findViewById(R.id.kill_switch_tutorial_image);
        int killSwitchImage = mImageResources.get(position);
        if (!mIsKillSwitch && position == 2) {
            killSwitchImage = R.drawable.ic_vpn_always_on_4;
        }
        killSwitchTutorialImage.setImageResource(killSwitchImage);

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
