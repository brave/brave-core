/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.DialogInterface;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoMojomHelper;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * The preference used to reset Brave Leo.
 */
public class BraveLeoResetPreference
        extends Preference implements Preference.OnPreferenceClickListener {
    private static final String TAG = "BraveLeoResetPref";

    /**
     * Constructor for BraveLeoResetPreference.
     */
    public BraveLeoResetPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOnPreferenceClickListener(this);
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        TextView titleView = (TextView) holder.findViewById(android.R.id.title);
        assert titleView != null;
        titleView.setTextAppearance(R.style.BraveLeoResetTextColor);
    }

    @Override
    public boolean onPreferenceClick(@NonNull Preference preference) {
        showBraveLeoResetDialog();
        return true;
    }

    private void showBraveLeoResetDialog() {
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.brave_leo_reset_dialog, null);

        DialogInterface.OnClickListener onClickListener =
                (dialog, button) -> {
                    if (button == AlertDialog.BUTTON_POSITIVE) {
                        Profile profile = null;
                        try {
                            BraveActivity activity = BraveActivity.getBraveActivity();
                            profile = activity.getCurrentProfile();
                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                            Log.e(TAG, "get BraveActivity exception", e);
                        }
                        if (profile == null) {
                            Log.e(TAG, "showBraveLeoResetDialog profile is null");
                            return;
                        }
                        UserPrefs.get(profile).clearPref(BravePref.LAST_ACCEPTED_DISCLAIMER);
                        BraveLeoMojomHelper.getInstance(profile)
                                .deleteConversations(TimePeriod.ALL_TIME);
                    } else {
                        dialog.dismiss();
                    }
                };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(getContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        AlertDialog alertDialog =
                alert.setTitle(R.string.leo_reset_data)
                        .setView(view)
                        .setPositiveButton(R.string.brave_leo_confirm_text, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }
}
