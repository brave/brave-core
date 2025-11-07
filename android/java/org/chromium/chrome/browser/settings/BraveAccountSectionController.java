/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_account.mojom.Authentication;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.brave_account.BraveAccountServiceFactory;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.chrome.browser.preferences.PrefServiceUtil;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.prefs.PrefChangeRegistrar;
import org.chromium.components.prefs.PrefChangeRegistrar.PrefObserver;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Controller for managing the Brave Account section in settings. Handles preference visibility,
 * click listeners, and state changes based on authentication status.
 */
@NullMarked
public class BraveAccountSectionController implements PrefObserver {
    // Preference keys
    private static final String PREF_GET_STARTED = "get_started";
    private static final String PREF_ALMOST_THERE = "almost_there";
    private static final String PREF_RESEND_CONFIRMATION_EMAIL = "resend_confirmation_email";
    private static final String PREF_CANCEL_REGISTRATION = "cancel_registration";
    private static final String PREF_BRAVE_ACCOUNT_USER = "brave_account_user";
    private static final String PREF_SIGN_OUT = "sign_out";

    // Brave Account pref names
    private static final String BRAVE_ACCOUNT_AUTHENTICATION_TOKEN_PREF =
            "brave.account.authentication_token";
    private static final String BRAVE_ACCOUNT_VERIFICATION_TOKEN_PREF =
            "brave.account.verification_token";

    private final PreferenceFragmentCompat mFragment;
    private final Profile mProfile;
    private @Nullable PrefChangeRegistrar mPrefChangeRegistrar;
    private @Nullable Authentication mAuthentication;

    /**
     * Creates a new controller for the Brave Account section.
     *
     * @param fragment The preference fragment containing the Brave Account preferences
     * @param profile The user profile
     */
    public BraveAccountSectionController(PreferenceFragmentCompat fragment, Profile profile) {
        mFragment = fragment;
        mProfile = profile;
    }

    /**
     * Initializes the controller by setting up preference listeners and observers. Should be called
     * during fragment onCreate().
     */
    public void init() {
        setupPreferenceListeners();
        setupPrefChangeObservers();
    }

    /** Cleans up resources. Should be called during fragment onDestroy(). */
    public void destroy() {
        if (mPrefChangeRegistrar != null) {
            mPrefChangeRegistrar.destroy();
            mPrefChangeRegistrar = null;
        }
        if (mAuthentication != null) {
            mAuthentication.close();
            mAuthentication = null;
        }
    }

    /**
     * Updates the UI based on the current authentication state. Should be called during fragment
     * onResume() or when state changes.
     */
    public void updateUI() {
        PostTask.postTask(TaskTraits.UI_DEFAULT, this::updateBraveAccountSection);
    }

    @Override
    public void onPreferenceChange() {
        // React to Brave Account pref changes
        updateUI();
    }

    private void setupPrefChangeObservers() {
        mPrefChangeRegistrar = PrefServiceUtil.createFor(mProfile);
        mPrefChangeRegistrar.addObserver(BRAVE_ACCOUNT_AUTHENTICATION_TOKEN_PREF, this);
        mPrefChangeRegistrar.addObserver(BRAVE_ACCOUNT_VERIFICATION_TOKEN_PREF, this);
    }

    private void setupPreferenceListeners() {
        Preference getStartedPreference = findPreference(PREF_GET_STARTED);
        if (getStartedPreference != null) {
            getStartedPreference.setOnPreferenceClickListener(
                    preference -> {
                        BraveAccountCustomTabActivity.show(mFragment.getActivity());
                        return true;
                    });
        }

        Preference resendEmailPreference = findPreference(PREF_RESEND_CONFIRMATION_EMAIL);
        if (resendEmailPreference != null) {
            resendEmailPreference.setOnPreferenceClickListener(
                    preference -> {
                        Authentication authentication = getAuthentication();
                        if (authentication != null) {
                            authentication.resendConfirmationEmail();
                        }
                        return true;
                    });
        }

        Preference cancelRegistrationPreference = findPreference(PREF_CANCEL_REGISTRATION);
        if (cancelRegistrationPreference != null) {
            cancelRegistrationPreference.setOnPreferenceClickListener(
                    preference -> {
                        Authentication authentication = getAuthentication();
                        if (authentication != null) {
                            authentication.cancelRegistration();
                        }
                        return true;
                    });
        }

        Preference signOutPreference = findPreference(PREF_SIGN_OUT);
        if (signOutPreference != null) {
            signOutPreference.setOnPreferenceClickListener(
                    preference -> {
                        Authentication authentication = getAuthentication();
                        if (authentication != null) {
                            authentication.logOut();
                        }
                        return true;
                    });
        }
    }

    private @Nullable Authentication getAuthentication() {
        if (mAuthentication == null) {
            mAuthentication =
                    BraveAccountServiceFactory.getInstance().getBraveAccountService(mProfile, null);
        }
        return mAuthentication;
    }

    private boolean hasAuthenticationToken() {
        String value = UserPrefs.get(mProfile).getString(BRAVE_ACCOUNT_AUTHENTICATION_TOKEN_PREF);
        return value != null && !value.isEmpty();
    }

    private boolean hasVerificationToken() {
        String value = UserPrefs.get(mProfile).getString(BRAVE_ACCOUNT_VERIFICATION_TOKEN_PREF);
        return value != null && !value.isEmpty();
    }

    private void updateBraveAccountSection() {
        boolean hasAuthToken = hasAuthenticationToken();
        boolean hasVerificationToken = hasVerificationToken();

        Preference getStartedPref = findPreference(PREF_GET_STARTED);
        Preference almostTherePref = findPreference(PREF_ALMOST_THERE);
        Preference resendEmailPref = findPreference(PREF_RESEND_CONFIRMATION_EMAIL);
        Preference cancelRegistrationPref = findPreference(PREF_CANCEL_REGISTRATION);
        Preference accountUserPref = findPreference(PREF_BRAVE_ACCOUNT_USER);
        Preference signOutPref = findPreference(PREF_SIGN_OUT);

        if (hasAuthToken) {
            // Show user account info
            setVisibility(getStartedPref, false);
            setVisibility(almostTherePref, false);
            setVisibility(resendEmailPref, false);
            setVisibility(cancelRegistrationPref, false);
            setVisibility(accountUserPref, true);
            setVisibility(signOutPref, true);
        } else if (hasVerificationToken) {
            // Show verification state
            setVisibility(getStartedPref, false);
            setVisibility(almostTherePref, true);
            setVisibility(resendEmailPref, true);
            setVisibility(cancelRegistrationPref, true);
            setVisibility(accountUserPref, false);
            setVisibility(signOutPref, false);
        } else {
            // Show get started
            setVisibility(getStartedPref, true);
            setVisibility(almostTherePref, false);
            setVisibility(resendEmailPref, false);
            setVisibility(cancelRegistrationPref, false);
            setVisibility(accountUserPref, false);
            setVisibility(signOutPref, false);
        }
    }

    private void setVisibility(@Nullable Preference preference, boolean visible) {
        if (preference != null) {
            preference.setVisible(visible);
        }
    }

    private @Nullable Preference findPreference(String key) {
        return mFragment.findPreference(key);
    }
}
