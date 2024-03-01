/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import androidx.preference.PreferenceDialogFragmentCompat;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.profiles.ProfileManager;

public class BravePreferenceDialogFragment extends PreferenceDialogFragmentCompat {
    public static final String TAG = "BravePreferenceDialogFragment";
    private RadioGroup mRadioGroup;
    private CharSequence[] mDialogEntries;
    private BraveDialogPreference dialogPreference;
    private Preference.OnPreferenceChangeListener onPreferenceChangeListener;
    private int newValue;

    private static String currentPreference;

    public void setPreferenceDialogListener(Preference.OnPreferenceChangeListener listener) {
        this.onPreferenceChangeListener = listener;
    }

    @NonNull
    public static BravePreferenceDialogFragment newInstance(@NonNull Preference preference) {
        BravePreferenceDialogFragment fragment = new BravePreferenceDialogFragment();
        Bundle bundle = new Bundle(1);
        bundle.putString(PreferenceDialogFragmentCompat.ARG_KEY, preference.getKey());
        currentPreference = preference.getKey();
        fragment.setArguments(bundle);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dialogPreference = (BraveDialogPreference) getPreference();
    }

    @Override
    public void onDialogClosed(boolean positiveResult) {
        if (onPreferenceChangeListener != null) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            if (currentPreference.equals(BravePrivacySettings.PREF_FINGERPRINTING_PROTECTION)) {
                onPreferenceChangeListener.onPreferenceChange(dialogPreference,
                        BraveShieldsContentSettings.getShieldsValue(
                                ProfileManager.getLastUsedRegularProfile(), "",
                                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING));
            } else if (currentPreference.equals(BravePrivacySettings.PREF_BLOCK_TRACKERS_ADS)) {
                onPreferenceChangeListener.onPreferenceChange(dialogPreference,
                        BraveShieldsContentSettings.getShieldsValue(
                                ProfileManager.getLastUsedRegularProfile(), "",
                                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS));
            } else {
                onPreferenceChangeListener.onPreferenceChange(
                        dialogPreference, sharedPreferences.getInt(currentPreference, 1));
            }
        }
    }

    @Override
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        super.onPrepareDialogBuilder(builder);
        builder.setCancelable(false);
        if (mRadioGroup != null) {
            mRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup group, int checkedId) {
                    newValue = checkedId;
                    SharedPreferences.Editor sharedPreferencesEditor =
                            ContextUtils.getAppSharedPreferences().edit();
                    if (currentPreference.equals(
                                BravePrivacySettings.PREF_FINGERPRINTING_PROTECTION)) {
                        if ((int) newValue == 0) {
                            BraveShieldsContentSettings.setShieldsValue(
                                    ProfileManager.getLastUsedRegularProfile(), "",
                                    BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                                    BraveShieldsContentSettings.BLOCK_RESOURCE, false);
                        } else if ((int) newValue == 1) {
                            BraveShieldsContentSettings.setShieldsValue(
                                    ProfileManager.getLastUsedRegularProfile(), "",
                                    BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                                    BraveShieldsContentSettings.DEFAULT, false);
                        } else {
                            BraveShieldsContentSettings.setShieldsValue(
                                    ProfileManager.getLastUsedRegularProfile(), "",
                                    BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                                    BraveShieldsContentSettings.ALLOW_RESOURCE, false);
                        }
                    } else if (currentPreference.equals(
                                       BravePrivacySettings.PREF_BLOCK_TRACKERS_ADS)) {
                        switch ((int) newValue) {
                            case 0:
                                BraveShieldsContentSettings.setShieldsValue(
                                        ProfileManager.getLastUsedRegularProfile(), "",
                                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                                        BraveShieldsContentSettings.BLOCK_RESOURCE, false);
                                break;
                            case 1:
                                BraveShieldsContentSettings.setShieldsValue(
                                        ProfileManager.getLastUsedRegularProfile(), "",
                                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                                        BraveShieldsContentSettings.DEFAULT, false);
                                break;
                            default:
                                BraveShieldsContentSettings.setShieldsValue(
                                        ProfileManager.getLastUsedRegularProfile(), "",
                                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                                        BraveShieldsContentSettings.ALLOW_RESOURCE, false);
                                break;
                        }
                    } else {
                        sharedPreferencesEditor.putInt(currentPreference, (int) newValue);
                    }

                    sharedPreferencesEditor.apply();
                }
            });
        }

        builder.setPositiveButton(null, null);
        builder.setNegativeButton(getResources().getString(R.string.brave_sync_btn_done), null);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        String subtitle = dialogPreference.getDialogSubtitle();
        TextView subTitle = view.findViewById(R.id.summary);
        subTitle.setText(subtitle);
        subTitle.refreshDrawableState();
        mRadioGroup = view.findViewById(R.id.options);
        mDialogEntries = dialogPreference.getDialogEntries();

        int index = 0;

        for (CharSequence entry : mDialogEntries) {
            RadioButton radioButton = new RadioButton(getContext());
            radioButton.setLayoutParams(setParams());
            radioButton.setTextSize(15);
            radioButton.setText(entry);
            radioButton.setId(index);
            if (mRadioGroup != null) {
                mRadioGroup.addView(radioButton);
            }
            if (index == dialogPreference.getCheckedIndex()) {
                radioButton.setChecked(true);
            }
            index++;
        }
    }

    private RadioGroup.LayoutParams setParams() {
        RadioGroup.LayoutParams params = new RadioGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        float density = getResources().getDisplayMetrics().density;
        params.setMargins((int) (20 * density), (int) (10 * density), (int) (20 * density), 0);
        return params;
    }
}
