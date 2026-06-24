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

import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_account.mojom.AccountState;
import org.chromium.brave_account.mojom.Authentication;
import org.chromium.brave_account.mojom.AuthenticationObserver;
import org.chromium.brave_account.mojom.ChangePasswordError;
import org.chromium.brave_account.mojom.ChangePasswordServerError;
import org.chromium.brave_account.mojom.ChangePasswordServerErrorCode;
import org.chromium.brave_account.mojom.ChangePasswordVerifyInitResult;
import org.chromium.brave_account.mojom.LoggedInState;
import org.chromium.brave_account.mojom.LoggedInVerificationIntent;
import org.chromium.brave_account.mojom.LoggedOutState;
import org.chromium.brave_account.mojom.LoggedOutVerificationIntent;
import org.chromium.brave_account.mojom.ResendConfirmationEmailError;
import org.chromium.brave_account.mojom.ResendConfirmationEmailResult;
import org.chromium.brave_account.mojom.ResendConfirmationEmailServerError;
import org.chromium.brave_account.mojom.ResendConfirmationEmailServerErrorCode;
import org.chromium.brave_account.mojom.VerificationIntent;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_account.BraveAccountServiceFactory;
import org.chromium.chrome.browser.customtabs.BraveAccountCustomTabActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.brave_account.BraveAccountFeatures;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Result;
import org.chromium.mojo.system.MojoException;

import java.util.Locale;
import java.util.Map;

@NullMarked
public class BraveAccountSectionController
        implements AuthenticationObserver, ConnectionErrorHandler {
    private static final Map<Integer, Integer> CHANGE_PASSWORD_CLIENT_ERROR_STRINGS = Map.of();

    private static final Map<Integer, Integer> CHANGE_PASSWORD_SERVER_ERROR_STRINGS =
            Map.of(
                    ChangePasswordServerErrorCode.TOO_MANY_VERIFICATIONS,
                    R.string.brave_account_register_too_many_verifications,
                    ChangePasswordServerErrorCode.DAILY_VERIFICATION_LIMIT_REACHED_FOR_EMAIL,
                    R.string.brave_account_daily_verification_limit_reached_for_email,
                    ChangePasswordServerErrorCode.VERIFICATION_NOT_FOUND_OR_INVALID_ID_OR_CODE,
                    R.string
                            .brave_account_password_reset_verification_not_found_or_invalid_id_or_code,
                    ChangePasswordServerErrorCode.EMAIL_ALREADY_VERIFIED,
                    R.string.brave_account_password_reset_email_already_verified,
                    ChangePasswordServerErrorCode.MAXIMUM_CODE_VERIFICATION_ATTEMPTS_EXCEEDED,
                    R.string
                            .brave_account_password_reset_maximum_code_verification_attempts_exceeded,
                    ChangePasswordServerErrorCode.INVALID_VERIFICATION_CODE,
                    R.string.brave_account_register_invalid_verification_code,
                    ChangePasswordServerErrorCode.TOKEN_HAS_EXPIRED,
                    R.string.brave_account_resend_confirmation_email_token_has_expired);

    private static final Map<Integer, Integer> RESEND_CONFIRMATION_EMAIL_CLIENT_ERROR_STRINGS =
            Map.of();

    private static final Map<Integer, Integer> RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS =
            Map.of(
                    ResendConfirmationEmailServerErrorCode.MAXIMUM_EMAIL_SEND_ATTEMPTS_EXCEEDED,
                    R.string.brave_account_resend_confirmation_email_maximum_send_attempts_exceeded,
                    ResendConfirmationEmailServerErrorCode.EMAIL_ALREADY_VERIFIED,
                    R.string.brave_account_resend_confirmation_email_already_verified,
                    ResendConfirmationEmailServerErrorCode.TOKEN_HAS_EXPIRED,
                    R.string.brave_account_resend_confirmation_email_token_has_expired);

    private static final String PREF_BRAVE_ACCOUNT_SECTION = "brave_account_section";
    private static final String PREF_USER_INFO = "user_info";
    private static final String PREF_CHANGE_PASSWORD = "change_password";
    private static final String PREF_SIGN_OUT = "sign_out";
    private static final String PREF_ALMOST_THERE = "almost_there";
    private static final String PREF_ENTER_VERIFICATION_CODE = "enter_verification_code";
    private static final String PREF_RESEND_CONFIRMATION_EMAIL = "resend_confirmation_email";
    private static final String PREF_CANCEL_VERIFICATION = "cancel_verification";
    private static final String PREF_GET_STARTED = "get_started";
    public static final String[] ALL_PREFERENCE_KEYS =
            new String[] {
                PREF_BRAVE_ACCOUNT_SECTION,
                PREF_USER_INFO,
                PREF_CHANGE_PASSWORD,
                PREF_SIGN_OUT,
                PREF_ALMOST_THERE,
                PREF_ENTER_VERIFICATION_CODE,
                PREF_RESEND_CONFIRMATION_EMAIL,
                PREF_CANCEL_VERIFICATION,
                PREF_GET_STARTED
            };

    private final ChromeBaseSettingsFragment mFragment;
    private final Profile mProfile;
    private @Nullable Authentication mBraveAccountService;

    public static @Nullable BraveAccountSectionController maybeCreate(
            ChromeBaseSettingsFragment fragment, Profile profile) {
        return BraveAccountFeatures.isBraveAccountEnabled()
                ? new BraveAccountSectionController(fragment, profile)
                : null;
    }

    private BraveAccountSectionController(ChromeBaseSettingsFragment fragment, Profile profile) {
        mFragment = fragment;
        mProfile = profile;
        initBraveAccountService();

        setupPreferenceListeners();
    }

    public void destroy() {
        cleanUpBraveAccountService();
    }

    @Override
    public void onAccountStateChanged(AccountState state) {
        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> updateBraveAccountSection(state));
    }

    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {
        cleanUpBraveAccountService();
        initBraveAccountService();
    }

    private boolean openBraveAccountDialog() {
        if (!mFragment.isAdded() || mFragment.isDetached()) {
            return false;
        }

        Activity activity = mFragment.getActivity();
        if (activity == null || activity.isFinishing()) {
            return false;
        }

        BraveAccountCustomTabActivity.show(activity);
        return true;
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

        Preference enterVerificationCodePreference =
                mFragment.findPreference(PREF_ENTER_VERIFICATION_CODE);
        if (enterVerificationCodePreference != null) {
            enterVerificationCodePreference.setOnPreferenceClickListener(
                    preference -> openBraveAccountDialog());
        }

        Preference getStartedPreference = mFragment.findPreference(PREF_GET_STARTED);
        if (getStartedPreference != null) {
            getStartedPreference.setOnPreferenceClickListener(
                    preference -> openBraveAccountDialog());
        }
    }

    private void updateBraveAccountSection(AccountState state) {
        if (!mFragment.isAdded() || mFragment.isDetached()) {
            return;
        }

        Preference userInfoPref = mFragment.findPreference(PREF_USER_INFO);
        Preference changePasswordPref = mFragment.findPreference(PREF_CHANGE_PASSWORD);
        Preference signOutPref = mFragment.findPreference(PREF_SIGN_OUT);
        Preference almostTherePref = mFragment.findPreference(PREF_ALMOST_THERE);
        Preference enterVerificationCodePref =
                mFragment.findPreference(PREF_ENTER_VERIFICATION_CODE);
        Preference resendConfirmationEmailPref =
                mFragment.findPreference(PREF_RESEND_CONFIRMATION_EMAIL);
        Preference cancelVerificationPref = mFragment.findPreference(PREF_CANCEL_VERIFICATION);
        Preference getStartedPref = mFragment.findPreference(PREF_GET_STARTED);

        // Hidden by default; each state re-shows only the rows it needs.
        setVisibility(userInfoPref, false);
        setVisibility(changePasswordPref, false);
        setVisibility(signOutPref, false);
        setVisibility(almostTherePref, false);
        setVisibility(enterVerificationCodePref, false);
        setVisibility(resendConfirmationEmailPref, false);
        setVisibility(cancelVerificationPref, false);
        setVisibility(getStartedPref, false);

        switch (state.which()) {
            case AccountState.Tag.LoggedIn:
                LoggedInState loggedIn = state.getLoggedIn();
                if (loggedIn.verification == null) {
                    userInfoPref.setTitle(loggedIn.email);
                    if (changePasswordPref != null) {
                        changePasswordPref.setOnPreferenceClickListener(
                                preference -> {
                                    preference.setEnabled(false);
                                    assert mBraveAccountService != null;
                                    mBraveAccountService.changePasswordVerifyInit(
                                            loggedIn.email,
                                            result ->
                                                    onChangePasswordVerifyInit(preference, result));
                                    return true;
                                });
                    }
                    setVisibility(userInfoPref, true);
                    setVisibility(changePasswordPref, true);
                    setVisibility(signOutPref, true);
                    break;
                }

                {
                    VerificationIntent intent = new VerificationIntent();
                    intent.setLoggedInIntent(loggedIn.verification.intent);

                    if (resendConfirmationEmailPref != null) {
                        resendConfirmationEmailPref.setOnPreferenceClickListener(
                                preference -> {
                                    preference.setEnabled(false);
                                    assert mBraveAccountService != null;
                                    mBraveAccountService.resendVerificationEmail(
                                            intent,
                                            result ->
                                                    onResendVerificationEmail(preference, result));
                                    return true;
                                });
                    }

                    if (cancelVerificationPref != null) {
                        cancelVerificationPref.setOnPreferenceClickListener(
                                preference -> {
                                    assert mBraveAccountService != null;
                                    mBraveAccountService.cancelVerification(intent);
                                    return true;
                                });
                    }
                }

                switch (loggedIn.verification.intent) {
                    case LoggedInVerificationIntent.CHANGE_PASSWORD:
                        if (loggedIn.verification.verifiedEmail.isEmpty()) {
                            setTitleAndSummary(
                                    almostTherePref,
                                    R.string.settings_brave_account_verification_row_title,
                                    R.string
                                            .settings_brave_account_change_password_row_description_1); // presubmit: ignore-long-line
                            setTitleAndSummary(
                                    enterVerificationCodePref,
                                    R.string
                                            .settings_brave_account_enter_verification_code_button_label, // presubmit: ignore-long-line
                                    R.string.settings_brave_account_verification_row_description_2);
                            setVisibility(resendConfirmationEmailPref, true);
                        } else {
                            setTitleAndSummary(
                                    almostTherePref,
                                    R.string.settings_brave_account_verification_row_title,
                                    R.string
                                            .settings_brave_account_change_password_verified_row_description); // presubmit: ignore-long-line
                            setTitleAndSummary(
                                    enterVerificationCodePref,
                                    R.string
                                            .settings_brave_account_set_new_password_button_label, // presubmit: ignore-long-line
                                    0);
                        }
                        setTitleAndSummary(
                                cancelVerificationPref,
                                R.string
                                        .settings_brave_account_cancel_change_password_button_label, // presubmit: ignore-long-line
                                0);
                        setVisibility(almostTherePref, true);
                        setVisibility(enterVerificationCodePref, true);
                        setVisibility(cancelVerificationPref, true);
                        break;

                    default:
                        assert false : "Unhandled LoggedInVerificationIntent!";
                        break;
                }
                break;

            case AccountState.Tag.LoggedOut:
                LoggedOutState loggedOut = state.getLoggedOut();
                if (loggedOut.verification == null) {
                    setVisibility(getStartedPref, true);
                    break;
                }

                {
                    VerificationIntent intent = new VerificationIntent();
                    intent.setLoggedOutIntent(loggedOut.verification.intent);

                    if (resendConfirmationEmailPref != null) {
                        resendConfirmationEmailPref.setOnPreferenceClickListener(
                                preference -> {
                                    preference.setEnabled(false);
                                    assert mBraveAccountService != null;
                                    mBraveAccountService.resendVerificationEmail(
                                            intent,
                                            result ->
                                                    onResendVerificationEmail(preference, result));
                                    return true;
                                });
                    }

                    if (cancelVerificationPref != null) {
                        cancelVerificationPref.setOnPreferenceClickListener(
                                preference -> {
                                    assert mBraveAccountService != null;
                                    mBraveAccountService.cancelVerification(intent);
                                    return true;
                                });
                    }
                }

                switch (loggedOut.verification.intent) {
                    case LoggedOutVerificationIntent.REGISTRATION:
                        setTitleAndSummary(
                                almostTherePref,
                                R.string.settings_brave_account_verification_row_title,
                                R.string.settings_brave_account_verification_row_description_1);
                        setTitleAndSummary(
                                enterVerificationCodePref,
                                R.string
                                        .settings_brave_account_enter_verification_code_button_label, // presubmit: ignore-long-line
                                R.string.settings_brave_account_verification_row_description_2);
                        setTitleAndSummary(
                                cancelVerificationPref,
                                R.string.settings_brave_account_cancel_registration_button_label,
                                0);
                        setVisibility(almostTherePref, true);
                        setVisibility(enterVerificationCodePref, true);
                        setVisibility(resendConfirmationEmailPref, true);
                        setVisibility(cancelVerificationPref, true);
                        break;

                    case LoggedOutVerificationIntent.RESET_PASSWORD:
                        if (loggedOut.verification.verifiedEmail.isEmpty()) {
                            setTitleAndSummary(
                                    almostTherePref,
                                    R.string.settings_brave_account_verification_row_title,
                                    R.string
                                            .settings_brave_account_reset_password_row_description_1); // presubmit: ignore-long-line
                            setTitleAndSummary(
                                    enterVerificationCodePref,
                                    R.string
                                            .settings_brave_account_enter_verification_code_button_label, // presubmit: ignore-long-line
                                    R.string.settings_brave_account_verification_row_description_2);
                            setVisibility(resendConfirmationEmailPref, true);
                        } else {
                            setTitleAndSummary(
                                    almostTherePref,
                                    R.string.settings_brave_account_verification_row_title,
                                    R.string
                                            .settings_brave_account_reset_password_verified_row_description); // presubmit: ignore-long-line
                            setTitleAndSummary(
                                    enterVerificationCodePref,
                                    R.string.settings_brave_account_set_new_password_button_label,
                                    0);
                        }
                        setTitleAndSummary(
                                cancelVerificationPref,
                                R.string.settings_brave_account_cancel_reset_password_button_label,
                                0);
                        setVisibility(almostTherePref, true);
                        setVisibility(enterVerificationCodePref, true);
                        setVisibility(cancelVerificationPref, true);
                        break;

                    default:
                        assert false : "Unhandled LoggedOutVerificationIntent!";
                        break;
                }
                break;
        }
        mFragment.notifyPreferencesUpdated();
    }

    private void setVisibility(@Nullable Preference preference, boolean visible) {
        if (preference != null) {
            preference.setVisible(visible);
        }
    }

    private void setTitleAndSummary(@Nullable Preference preference, int titleId, int summaryId) {
        if (preference != null) {
            preference.setTitle(titleId);
            preference.setSummary(summaryId != 0 ? mFragment.getString(summaryId) : null);
        }
    }

    private void initBraveAccountService() {
        mBraveAccountService =
                BraveAccountServiceFactory.getInstance().getBraveAccountService(mProfile, this);
        assert mBraveAccountService != null;
        mBraveAccountService.addObserver(this);
    }

    private void cleanUpBraveAccountService() {
        if (mBraveAccountService != null) {
            mBraveAccountService.close();
            mBraveAccountService = null;
        }
    }

    private String getAlertMessage(
            Map<Integer, Integer> clientErrorStrings,
            Map<Integer, Integer> serverErrorStrings,
            @Nullable Integer clientErrorCode,
            @Nullable Integer serverErrorCode,
            @Nullable Integer netErrorOrHttpStatus) {
        if (clientErrorCode != null) {
            Integer stringId = clientErrorStrings.get(clientErrorCode);
            if (stringId != null) {
                return mFragment.getString(stringId);
            }

            return mFragment
                    .getString(R.string.brave_account_client_error)
                    .replace(
                            "$1",
                            String.format(
                                    Locale.ROOT,
                                    " (%s=%d)",
                                    mFragment.getString(R.string.brave_account_error),
                                    clientErrorCode));
        }

        assert serverErrorCode != null;
        assert netErrorOrHttpStatus != null;
        Integer stringId = serverErrorStrings.get(serverErrorCode);
        if (stringId != null) {
            return mFragment.getString(stringId);
        }

        return mFragment
                .getString(R.string.brave_account_server_error)
                .replace(
                        "$1",
                        String.format(
                                Locale.ROOT,
                                "%s=%d",
                                netErrorOrHttpStatus > 0 ? "HTTP" : "NET",
                                netErrorOrHttpStatus))
                .replace(
                        "$2",
                        String.format(
                                Locale.ROOT,
                                ", %s=%d",
                                mFragment.getString(R.string.brave_account_error),
                                serverErrorCode));
    }

    private void showAlertDialog(Preference preference, String title, String message) {
        PostTask.postTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    preference.setEnabled(true);

                    if (!mFragment.isAdded() || mFragment.isDetached()) {
                        return;
                    }

                    Activity activity = mFragment.getActivity();
                    if (activity == null || activity.isFinishing()) {
                        return;
                    }

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
                });
    }

    private void onResendVerificationEmail(
            Preference preference,
            Result<ResendConfirmationEmailResult, ResendConfirmationEmailError> result) {
        ResendConfirmationEmailError error = result.isSuccess() ? null : result.getError();

        String title =
                mFragment.getString(
                        error == null
                                ? R.string.brave_account_resend_confirmation_email_success_title
                                : R.string.brave_account_resend_confirmation_email_error_title);

        String message;
        if (error == null) {
            message = mFragment.getString(R.string.brave_account_resend_confirmation_email_success);
        } else {
            boolean isClientError = error.which() == ResendConfirmationEmailError.Tag.ClientError;
            ResendConfirmationEmailServerError serverError =
                    isClientError ? null : error.getServerError();
            message =
                    getAlertMessage(
                            RESEND_CONFIRMATION_EMAIL_CLIENT_ERROR_STRINGS,
                            RESEND_CONFIRMATION_EMAIL_SERVER_ERROR_STRINGS,
                            isClientError ? error.getClientError().errorCode : null,
                            serverError == null ? null : serverError.errorCode,
                            serverError == null ? null : serverError.netErrorOrHttpStatus);
        }

        showAlertDialog(preference, title, message);
    }

    private void onChangePasswordVerifyInit(
            Preference preference,
            Result<ChangePasswordVerifyInitResult, ChangePasswordError> result) {
        if (result.isSuccess()) {
            PostTask.postTask(
                    TaskTraits.UI_DEFAULT,
                    () -> {
                        preference.setEnabled(true);
                        openBraveAccountDialog();
                    });
            return;
        }

        ChangePasswordError error = result.getError();
        boolean isClientError = error.which() == ChangePasswordError.Tag.ClientError;
        ChangePasswordServerError serverError = isClientError ? null : error.getServerError();
        String title =
                mFragment.getString(R.string.settings_brave_account_change_password_error_title);
        String message =
                getAlertMessage(
                        CHANGE_PASSWORD_CLIENT_ERROR_STRINGS,
                        CHANGE_PASSWORD_SERVER_ERROR_STRINGS,
                        isClientError ? error.getClientError().errorCode : null,
                        serverError == null ? null : serverError.errorCode,
                        serverError == null ? null : serverError.netErrorOrHttpStatus);

        showAlertDialog(preference, title, message);
    }
}
