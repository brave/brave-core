/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_account.mojom.Authentication;
import org.chromium.brave_account.mojom.ResendConfirmationEmailError;
import org.chromium.brave_account.mojom.ResendConfirmationEmailErrorCode;
import org.chromium.brave_account.mojom.ResendConfirmationEmailResult;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
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
import org.chromium.mojo.bindings.Result;
import org.chromium.mojo.system.MojoException;

import java.util.Locale;
import java.util.Map;

@NullMarked
public class BraveAccountSectionController implements PrefObserver, ConnectionErrorHandler {
    private static final Map<Integer, Integer> ERROR_STRINGS =
            Map.of(
                    ResendConfirmationEmailErrorCode.MAXIMUM_EMAIL_SEND_ATTEMPTS_EXCEEDED,
                    R.string.brave_account_resend_confirmation_email_maximum_send_attempts_exceeded,
                    ResendConfirmationEmailErrorCode.EMAIL_ALREADY_VERIFIED,
                    R.string.brave_account_resend_confirmation_email_already_verified);

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
                        preference.setEnabled(false);
                        assert mBraveAccountService != null;
                        mBraveAccountService.resendConfirmationEmail(
                                result -> showAlertDialog(preference, result));
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
        mPrefChangeRegistrar.addObserver(BravePref.BRAVE_ACCOUNT_EMAIL_ADDRESS, this);
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
            userInfoPref.setSummary(
                    UserPrefs.get(mProfile).getString(BravePref.BRAVE_ACCOUNT_EMAIL_ADDRESS));
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

    private String getAlertTitle(@Nullable ResendConfirmationEmailError error) {
        return mFragment.getString(
                error == null
                        ? R.string.brave_account_resend_confirmation_email_success_title
                        : R.string.brave_account_resend_confirmation_email_error_title);
    }

    private String getAlertMessage(@Nullable ResendConfirmationEmailError error) {
        if (error == null) {
            return mFragment.getString(R.string.brave_account_resend_confirmation_email_success);
        }

        if (error.netErrorOrHttpStatus == null) {
            // client-side error
            return mFragment
                    .getString(R.string.brave_account_client_error)
                    .replace(
                            "$1",
                            error.errorCode != null
                                    ? String.format(
                                            Locale.ROOT,
                                            " (%s=%d)",
                                            mFragment.getString(R.string.brave_account_error),
                                            error.errorCode)
                                    : "");
        }

        // server-side error
        if (error.errorCode != null) {
            Integer stringId = ERROR_STRINGS.get(error.errorCode);
            if (stringId != null) {
                return mFragment.getString(stringId);
            }
        }

        return mFragment
                .getString(R.string.brave_account_server_error)
                .replace("$1", String.valueOf(error.netErrorOrHttpStatus))
                .replace(
                        "$2",
                        error.errorCode != null
                                ? String.format(
                                        Locale.ROOT,
                                        ", %s=%d",
                                        mFragment.getString(R.string.brave_account_error),
                                        error.errorCode)
                                : "");
    }

    private void showAlertDialog(
            Preference preference,
            Result<ResendConfirmationEmailResult, ResendConfirmationEmailError> result) {
        PostTask.postTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (!mFragment.isAdded() || mFragment.isDetached()) {
                        return;
                    }

                    Activity activity = mFragment.getActivity();
                    if (activity == null || activity.isFinishing()) {
                        return;
                    }

                    ResendConfirmationEmailError error =
                            result.isSuccess() ? null : result.getError();
                    String title = getAlertTitle(error);
                    String message = getAlertMessage(error);

                    LayoutInflater inflater =
                            (LayoutInflater)
                                    activity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                    View view = inflater.inflate(R.layout.brave_account_alert_dialog, null);
                    TextView messageView = view.findViewById(R.id.brave_account_alert_message);
                    messageView.setText(message);

                    new AlertDialog.Builder(activity, R.style.ThemeOverlay_BrowserUI_AlertDialog)
                            .setTitle(title)
                            .setView(view)
                            .setPositiveButton(R.string.ok, null)
                            .show();

                    preference.setEnabled(true);
                });
    }
}
