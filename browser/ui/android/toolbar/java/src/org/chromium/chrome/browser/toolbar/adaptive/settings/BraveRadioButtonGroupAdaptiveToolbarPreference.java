/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RadioGroup;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.ui.base.DeviceFormFactor;

/** Brave's fragment that allows the user to configure toolbar shortcut preferences. */
@NullMarked
public class BraveRadioButtonGroupAdaptiveToolbarPreference
        extends RadioButtonGroupAdaptiveToolbarPreference {
    private final Context mContext;
    private @Nullable RadioButtonWithDescription mAutoButton;
    private @Nullable RadioButtonWithDescription mNewTabButton;
    private @Nullable RadioButtonWithDescription mShareButton;

    public BraveRadioButtonGroupAdaptiveToolbarPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mAutoButton =
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.adaptive_option_based_on_usage);
        // We don't have Auto option in Brave, so we hide it.
        mAutoButton.setVisibility(View.GONE);

        mNewTabButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_new_tab);
        mShareButton = (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_share);
    }

    @Override
    public void onCheckedChanged(@Nullable RadioGroup group, int checkedId) {
        RadioButtonWithDescription defaultButton =
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext)
                        ? mShareButton
                        : mNewTabButton;
        if (mAutoButton != null && defaultButton != null && checkedId == mAutoButton.getId()) {
            // Redirect the state of the Auto button to the default button.
            checkedId = defaultButton.getId();
            defaultButton.setChecked(mAutoButton.isChecked());
        }

        super.onCheckedChanged(group, checkedId);
    }
}
