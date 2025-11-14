/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_account.mojom.Authentication;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.brave_account.BraveAccountServiceFactory;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.PrefServiceUtil;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.prefs.PrefChangeRegistrar;
import org.chromium.components.prefs.PrefChangeRegistrar.PrefObserver;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

@NullMarked
public class BraveAccountSectionController implements PrefObserver, ConnectionErrorHandler {
    private static final String PREF_BRAVE_ACCOUNT_SECTION = "brave_account_section";
    private static final String PREF_USER_INFO = "user_info";
    private static final String PREF_SIGN_OUT = "sign_out";
    private static final String PREF_ALMOST_THERE = "almost_there";
    private static final String PREF_RESEND_CONFIRMATION_EMAIL = "resend_confirmation_email";
    private static final String PREF_CANCEL_REGISTRATION = "cancel_registration";
    private static final String PREF_GET_STARTED = "get_started";
    public static final String[] ALL_PREFERENCE_KEYS =
            new String[] {
                PREF_BRAVE_ACCOUNT_SECTION,
                PREF_USER_INFO,
                PREF_SIGN_OUT,
                PREF_ALMOST_THERE,
                PREF_RESEND_CONFIRMATION_EMAIL,
                PREF_CANCEL_REGISTRATION,
                PREF_GET_STARTED
            };

    private final PreferenceFragmentCompat mFragment;
    private final Profile mProfile;
    private @Nullable Authentication mBraveAccountService;
    private @Nullable PrefChangeRegistrar mPrefChangeRegistrar;

    public static @Nullable BraveAccountSectionController maybeCreate(
            PreferenceFragmentCompat fragment, Profile profile) {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ACCOUNT)
                ? new BraveAccountSectionController(fragment, profile)
                : null;
    }

    private BraveAccountSectionController(PreferenceFragmentCompat fragment, Profile profile) {
        mFragment = fragment;
        mProfile = profile;
        initBraveAccountService();

        setupPreferenceListeners();
        setupPrefChangeRegistrar();
    }

    public void destroy() {
        if (mPrefChangeRegistrar != null) {
            mPrefChangeRegistrar.destroy();
            mPrefChangeRegistrar = null;
        }

        cleanUpBraveAccountService();
    }

    public void updateUI() {
        PostTask.postTask(TaskTraits.UI_DEFAULT, this::updateBraveAccountSection);
    }

    @Override
    public void onPreferenceChange() {
        updateUI();
    }

    @Override
    public void onConnectionError(MojoException e) {
        cleanUpBraveAccountService();
        initBraveAccountService();
    }

    private void setupPreferenceListeners() {
        Preference signOutPreference = mFragment.findPreference(PREF_SIGN_OUT);
        if (signOutPreference != null) {
            signOutPreference.setOnPreferenceClickListener(
                    preference -> {
                        assert mBraveAccountService != null;
                        mBraveAccountService.logOut();
                        return true;
                    });
        }

        Preference resendConfirmationEmailPreference =
                mFragment.findPreference(PREF_RESEND_CONFIRMATION_EMAIL);
        if (resendConfirmationEmailPreference != null) {
            resendConfirmationEmailPreference.setOnPreferenceClickListener(
                    preference -> {
                        assert mBraveAccountService != null;
                        mBraveAccountService.resendConfirmationEmail();
                        return true;
                    });
        }

        Preference cancelRegistrationPreference =
                mFragment.findPreference(PREF_CANCEL_REGISTRATION);
        if (cancelRegistrationPreference != null) {
            cancelRegistrationPreference.setOnPreferenceClickListener(
                    preference -> {
                        assert mBraveAccountService != null;
                        mBraveAccountService.cancelRegistration();
                        return true;
                    });
        }

        Preference getStartedPreference = mFragment.findPreference(PREF_GET_STARTED);
        if (getStartedPreference != null) {
            getStartedPreference.setOnPreferenceClickListener(
                    preference -> {
                        if (!mFragment.isAdded() || mFragment.isDetached()) {
                            return false;
                        }

                        Activity activity = mFragment.getActivity();
                        if (activity == null || activity.isFinishing()) {
                            return false;
                        }

                        BraveAccountCustomTabActivity.show(activity);
                        return true;
                    });
        }
    }

    private void setupPrefChangeRegistrar() {
        mPrefChangeRegistrar = PrefServiceUtil.createFor(mProfile);
        mPrefChangeRegistrar.addObserver(BravePref.BRAVE_ACCOUNT_AUTHENTICATION_TOKEN, this);
        mPrefChangeRegistrar.addObserver(BravePref.BRAVE_ACCOUNT_VERIFICATION_TOKEN, this);
    }

    private boolean hasPrefValue(String prefKey) {
        String value = UserPrefs.get(mProfile).getString(prefKey);
        return value != null && !value.isEmpty();
    }

    private void updateBraveAccountSection() {
        if (!mFragment.isAdded() || mFragment.isDetached()) {
            return;
        }

        Preference userInfoPref = mFragment.findPreference(PREF_USER_INFO);
        Preference signOutPref = mFragment.findPreference(PREF_SIGN_OUT);
        Preference almostTherePref = mFragment.findPreference(PREF_ALMOST_THERE);
        Preference resendConfirmationEmailPref =
                mFragment.findPreference(PREF_RESEND_CONFIRMATION_EMAIL);
        Preference cancelRegistrationPref = mFragment.findPreference(PREF_CANCEL_REGISTRATION);
        Preference getStartedPref = mFragment.findPreference(PREF_GET_STARTED);

        if (hasPrefValue(BravePref.BRAVE_ACCOUNT_AUTHENTICATION_TOKEN)) { // logged in
            setVisibility(userInfoPref, true);
            setVisibility(signOutPref, true);
            setVisibility(almostTherePref, false);
            setVisibility(resendConfirmationEmailPref, false);
            setVisibility(cancelRegistrationPref, false);
            setVisibility(getStartedPref, false);
        } else if (hasPrefValue(BravePref.BRAVE_ACCOUNT_VERIFICATION_TOKEN)) { // verification
            setVisibility(userInfoPref, false);
            setVisibility(signOutPref, false);
            setVisibility(almostTherePref, true);
            setVisibility(resendConfirmationEmailPref, true);
            setVisibility(cancelRegistrationPref, true);
            setVisibility(getStartedPref, false);
        } else { // logged out
            setVisibility(userInfoPref, false);
            setVisibility(signOutPref, false);
            setVisibility(almostTherePref, false);
            setVisibility(resendConfirmationEmailPref, false);
            setVisibility(cancelRegistrationPref, false);
            setVisibility(getStartedPref, true);
        }
    }

    private void setVisibility(@Nullable Preference preference, boolean visible) {
        if (preference != null) {
            preference.setVisible(visible);
        }
    }

    private void initBraveAccountService() {
        mBraveAccountService =
                BraveAccountServiceFactory.getInstance().getBraveAccountService(mProfile, this);
        assert mBraveAccountService != null;
    }

    private void cleanUpBraveAccountService() {
        if (mBraveAccountService != null) {
            mBraveAccountService.close();
            mBraveAccountService = null;
        }
    }
}
