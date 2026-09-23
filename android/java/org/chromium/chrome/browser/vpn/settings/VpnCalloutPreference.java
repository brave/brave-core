/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.settings;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.cardview.widget.CardView;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.widget.Toast;

public class VpnCalloutPreference extends Preference {
    private @Nullable Runnable mOnDismissedCallback;

    public VpnCalloutPreference(Context context) {
        super(context);
        setLayoutResource(R.layout.vpn_settings_callout_modal_layout);
    }

    /**
     * Sets a callback invoked after the callout removes itself from the preference screen. Lets the
     * host fragment refresh dependent UI (e.g. containment styling of the remaining preferences).
     */
    public void setOnDismissedCallback(@Nullable Runnable callback) {
        mOnDismissedCallback = callback;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        View mainView = holder.findViewById(R.id.callout_cardview);

        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext());

        int width;
        if (ChromeFeatureList.sAndroidSettingsContainment.isEnabled()) {
            // With containment the card lives inside a padded rounded container — let it
            // fill the available width rather than forcing the raw screen pixel width.
            width = CardView.LayoutParams.MATCH_PARENT;
        } else {
            width = (int) (getContext().getResources().getDisplayMetrics().widthPixels * 1.00);
            if (isTablet || ConfigurationUtils.isLandscape(getContext())) {
                width = (int) dpToPx(getContext(), 390);
            }
        }
        int height = CardView.LayoutParams.WRAP_CONTENT;
        ViewGroup.LayoutParams params = mainView.getLayoutParams();
        params.width = width;
        params.height = height;
        mainView.setLayoutParams(params);

        AppCompatImageView btnClose = (AppCompatImageView) holder.findViewById(R.id.modal_close);
        btnClose.setOnClickListener(
                v -> {
                    getPreferenceManager().getPreferenceScreen().removePreference(this);
                    BraveVpnPrefUtils.setCallout(false);
                    if (mOnDismissedCallback != null) {
                        mOnDismissedCallback.run();
                    }
                });
        Button btnLearnMore = (Button) holder.findViewById(R.id.btn_learn_more);
        btnLearnMore.setOnClickListener(
                v -> {
                    if (!InternetConnection.isNetworkAvailable(getContext())) {
                        Toast.makeText(getContext(), R.string.no_internet, Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity((Activity) getContext());
                        BraveVpnPrefUtils.setCallout(false);
                    }
                });
    }
}
