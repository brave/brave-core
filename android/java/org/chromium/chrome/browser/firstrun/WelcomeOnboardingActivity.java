/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.firstrun;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.LayoutTransition;
import android.content.Intent;
import android.os.Build;
import android.os.RemoteException;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.Transformation;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.android.installreferrer.api.InstallReferrerClient;
import com.android.installreferrer.api.InstallReferrerClient.InstallReferrerResponse;
import com.android.installreferrer.api.InstallReferrerStateListener;
import com.android.installreferrer.api.ReferrerDetails;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.day_zero.DayZeroHelper;
import org.chromium.chrome.browser.metrics.ChangeMetricsReportingStateCalledFrom;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

import java.util.Locale;

/**
 * Activity that handles the first run onboarding experience for new Brave browser installations.
 * Extends FirstRunActivityBase to provide onboarding flows for: - Setting Brave as default browser
 * - Configuring privacy and analytics preferences (P3A and crash reporting) - Accepting terms of
 * service The activity guides users through a series of steps using animations and clear UI
 * elements to explain Brave's key features and privacy-focused approach.
 */
public class WelcomeOnboardingActivity extends FirstRunActivityBase {
    private static final String P3A_URL =
            "https://support.brave.com/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave";
    private static final String WDP_LINK =
            "https://www.brave.com/browser/privacy/#web-discovery-project";

    private static final String TAG = "WelcomeOnboarding";

    // mInitializeViewsDone and mInvokePostWorkAtInitializeViews are accessed
    // from the same thread, so no need to use extra locks
    private boolean mInitializeViewsDone;
    private boolean mInvokePostWorkAtInitializeViews;

    private boolean mIsTablet;
    private BraveFirstRunFlowSequencer mFirstRunFlowSequencer;
    private int mCurrentStep = -1;

    private View mVLeafAlignTop;
    private View mVLeafAlignBottom;
    private ImageView mIvLeafTop;
    private ImageView mIvLeafBottom;
    private ImageView mIvBrave;
    private ImageView mIvArrowDown;
    private LinearLayout mLayoutCard;
    private TextView mTvCard;
    private TextView mTvDefault;
    private Button mBtnPositive;
    private Button mBtnNegative;
    private CheckBox mCheckboxCrash;
    private CheckBox mCheckboxP3a;

    private enum CurrentOnboardingPage {
        SET_AS_DEFAULT,
        NOTIFICATION_PERMISSION,
        WDP_PAGE,
        ANALYTICS_CONSENT_PAGE
    }

    private CurrentOnboardingPage mCurrentOnboardingPage;

    /**
     * Initializes the views and sets up the onboarding activity UI. This method handles the initial
     * setup of the welcome onboarding screen, including loading the layout, initializing views and
     * click listeners, and performing first-run setup tasks.
     */
    private void initializeViews() {
        assert !mInitializeViewsDone;

        setContentView(R.layout.activity_welcome_onboarding);

        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(this);

        initViews();

        onClickViews();

        mInitializeViewsDone = true;

        if (mInvokePostWorkAtInitializeViews) {
            finishNativeInitializationPostWork();
        }

        checkReferral();

        maybeUpdateFirstRunDefaultValues();
    }

    private void checkReferral() {
        InstallReferrerClient referrerClient = InstallReferrerClient.newBuilder(this).build();
        referrerClient.startConnection(
                new InstallReferrerStateListener() {
                    @Override
                    public void onInstallReferrerSetupFinished(int responseCode) {
                        switch (responseCode) {
                            case InstallReferrerResponse.OK:
                                try {
                                    ReferrerDetails response = referrerClient.getInstallReferrer();
                                    String referrerUrl = response.getInstallReferrer();
                                    if (referrerUrl == null) return;

                                    if (referrerUrl.equals(
                                            BraveConstants.DEEPLINK_ANDROID_PLAYLIST)) {
                                        ChromeSharedPreferences.getInstance()
                                                .writeBoolean(
                                                        BravePreferenceKeys
                                                                .BRAVE_DEFERRED_DEEPLINK_PLAYLIST,
                                                        true);
                                    } else if (referrerUrl.equals(
                                            BraveConstants.DEEPLINK_ANDROID_VPN)) {
                                        ChromeSharedPreferences.getInstance()
                                                .writeBoolean(
                                                        BravePreferenceKeys
                                                                .BRAVE_DEFERRED_DEEPLINK_VPN,
                                                        true);
                                    }
                                } catch (RemoteException e) {
                                    Log.e(TAG, "Could not get referral: " + e.getMessage());
                                }
                                // Connection established.
                                break;
                            case InstallReferrerResponse.FEATURE_NOT_SUPPORTED:
                                // API not available on the current Play Store app.
                                Log.e(TAG, "InstallReferrerResponse.FEATURE_NOT_SUPPORTED");
                                break;
                            case InstallReferrerResponse.SERVICE_UNAVAILABLE:
                                // Connection couldn't be established.
                                Log.e(TAG, "InstallReferrerResponse.SERVICE_UNAVAILABLE");
                                break;
                        }
                    }

                    @Override
                    public void onInstallReferrerServiceDisconnected() {}
                });
    }

    private void initViews() {
        mIvLeafTop = findViewById(R.id.iv_leaf_top);
        mIvLeafBottom = findViewById(R.id.iv_leaf_bottom);
        mVLeafAlignTop = findViewById(R.id.view_leaf_top_align);
        mVLeafAlignBottom = findViewById(R.id.view_leaf_bottom_align);
        mIvBrave = findViewById(R.id.iv_brave);
        mIvArrowDown = findViewById(R.id.iv_arrow_down);
        mLayoutCard = findViewById(R.id.layout_card);
        mTvCard = findViewById(R.id.tv_card);
        mTvDefault = findViewById(R.id.tv_default);
        mCheckboxCrash = findViewById(R.id.checkbox_crash);
        mCheckboxP3a = findViewById(R.id.checkbox_p3a);
        mBtnPositive = findViewById(R.id.btn_positive);
        mBtnNegative = findViewById(R.id.btn_negative);
        LinearLayout layoutData = findViewById(R.id.layout_data);
        LayoutTransition layoutTransition = new LayoutTransition();
        layoutTransition.setDuration(1000);
        if (layoutData != null) {
            layoutData.setLayoutTransition(layoutTransition);
        }

        int margin = mIsTablet ? 200 : 50;

        if (mVLeafAlignTop != null) {
            ViewGroup.MarginLayoutParams topLeafParams =
                    (ViewGroup.MarginLayoutParams) mVLeafAlignTop.getLayoutParams();
            topLeafParams.bottomMargin = margin;
            mVLeafAlignTop.setLayoutParams(topLeafParams);
        }

        if (mVLeafAlignBottom != null) {
            ViewGroup.MarginLayoutParams bottomLeafParams =
                    (ViewGroup.MarginLayoutParams) mVLeafAlignBottom.getLayoutParams();
            bottomLeafParams.topMargin = margin;
            mVLeafAlignBottom.setLayoutParams(bottomLeafParams);
        }

        if (mBtnPositive != null) {
            BraveTouchUtils.ensureMinTouchTarget(mBtnPositive);
        }
        if (mCheckboxCrash != null) {
            BraveTouchUtils.ensureMinTouchTarget(mCheckboxCrash);
        }
        if (mCheckboxP3a != null) {
            BraveTouchUtils.ensureMinTouchTarget(mCheckboxP3a);
        }
    }

    private void onClickViews() {
        if (mBtnPositive != null) {
            mBtnPositive.setOnClickListener(
                    view -> {
                        if (mCurrentStep == 0 && !isDefaultBrowser()) {
                            setDefaultBrowserAndProceedToNextStep();
                        } else if (isWDPEnabled()
                                && mCurrentOnboardingPage == CurrentOnboardingPage.WDP_PAGE) {
                            UserPrefs.get(getProfileProviderSupplier().get().getOriginalProfile())
                                    .setBoolean(BravePref.WEB_DISCOVERY_ENABLED, true);
                            nextOnboardingStep();
                        } else {
                            nextOnboardingStep();
                        }
                    });
        }

        if (mBtnNegative != null) {
            mBtnNegative.setOnClickListener(
                    view -> {
                        if (mCurrentOnboardingPage
                                == CurrentOnboardingPage.ANALYTICS_CONSENT_PAGE) {
                            CustomTabActivity.showInfoPage(this, P3A_URL);
                        } else {
                            nextOnboardingStep();
                        }
                    });
        }
    }

    private void setDefaultBrowserAndProceedToNextStep() {
        BraveSetDefaultBrowserUtils.setDefaultBrowser(this, true);
        if (!BraveSetDefaultBrowserUtils.supportsDefaultRoleManager()) {
            nextOnboardingStep();
        }
        // onActivityResult will call nextOnboardingStep().
    }

    private boolean isDefaultBrowser() {
        return BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(this);
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == BravePermissionUtils.NOTIFICATION_PERMISSION_CODE) {
            nextOnboardingStep();
        }
    }

    private void nextOnboardingStep() {
        if (isActivityFinishingOrDestroyed()) return;

        mCurrentStep++;
        String variant = DayZeroHelper.getDayZeroVariant();
        switch (variant) {
            case "b":
                handleOnboardingStepForVariantB(mCurrentStep);
                break;
            case "c":
                handleOnboardingStepForVariantC(mCurrentStep);
                break;
            case "d":
                handleOnboardingStepForVariantD(mCurrentStep);
                break;
            default:
                handleOnboardingStepForVariantA(mCurrentStep);
                break;
        }
    }

    private void handleOnboardingStepForVariantA(int step) {
        if (step == 0) {
            handleSetAsDefaultStep();
        } else if (step == 1) {
            handleWDPStep();
        } else if (step == 2) {
            handleAnalyticsConsentPage();
        } else {
            finalStep();
        }
    }

    private void handleOnboardingStepForVariantB(int step) {
        if (step == 0) {
            handleSetAsDefaultStep();
        } else if (step == 1) {
            // Notification permission
            handleNotificationPermission();
        } else if (step == 2) {
            handleWDPStep();
        } else if (step == 3) {
            handleAnalyticsConsentPage();
        } else {
            finalStep();
        }
    }

    private void handleOnboardingStepForVariantC(int step) {
        if (step == 0) {
            handleSetAsDefaultStep();
        } else if (step == 1) {
            handleWDPStep();
        } else if (step == 2) {
            handleAnalyticsConsentPage();
        } else {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(OnboardingPrefManager.SHOULD_SHOW_SEARCH_WIDGET_PROMO, true);
            finalStep();
        }
    }

    private void handleOnboardingStepForVariantD(int step) {
        if (step == 0) {
            handleSetAsDefaultStep();
        } else if (step == 1) {
            // Notification permission
            handleNotificationPermission();
        } else if (step == 2) {
            handleWDPStep();
        } else if (step == 3) {
            handleAnalyticsConsentPage();
        } else {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(OnboardingPrefManager.SHOULD_SHOW_SEARCH_WIDGET_PROMO, true);
            finalStep();
        }
    }

    private void finalStep() {
        OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);

        FirstRunStatus.setFirstRunFlowComplete(true);

        ChromeSharedPreferences.getInstance()
                .writeBoolean(ChromePreferenceKeys.FIRST_RUN_CACHED_TOS_ACCEPTED, true);
        FirstRunUtils.setEulaAccepted();

        finish();
        sendFirstRunCompleteIntent();
    }

    private void handleNotificationPermission() {
        mCurrentOnboardingPage = CurrentOnboardingPage.NOTIFICATION_PERMISSION;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            BravePermissionUtils.showNotificationPermissionDialog(WelcomeOnboardingActivity.this);
        } else {
            nextOnboardingStep();
        }
    }

    private void handleSetAsDefaultStep() {
        mCurrentOnboardingPage = CurrentOnboardingPage.SET_AS_DEFAULT;
        if (!BraveSetDefaultBrowserUtils.supportsDefaultRoleManager()) {
            if (mIvBrave != null) {
                mIvBrave.setVisibility(View.VISIBLE);
            }
            showBrowserSelectionPage();
        } else if (!isDefaultBrowser()) {
            setDefaultBrowserAndProceedToNextStep();
        } else {
            nextOnboardingStep();
        }
    }

    private void handleWDPStep() {
        if (!isWDPEnabled()) {
            nextOnboardingStep();
            return;
        }

        mCurrentOnboardingPage = CurrentOnboardingPage.WDP_PAGE;
        if (mIvBrave != null) {
            mIvBrave.setVisibility(View.VISIBLE);
        }
        showWDPPage();
    }

    private boolean isWDPEnabled() {
        return BraveConfig.WEB_DISCOVERY_ENABLED;
    }

    private void showBrowserSelectionPage() {
        int margin = mIsTablet ? 200 : 30;
        setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1.3f, margin, true);
        setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1.3f, margin, false);

        if (isDefaultBrowser()) {
            if (mBtnPositive != null) {
                mBtnPositive.setText(getResources().getString(R.string.continue_text));
            }
            if (mBtnNegative != null) {
                mBtnNegative.setVisibility(View.GONE);
            }
        }
        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.VISIBLE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.VISIBLE);
        }
        String countryCode = Locale.getDefault().getCountry();
        if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)) {
            if (mTvCard != null) {
                mTvCard.setText(getResources().getString(R.string.privacy_onboarding_india));
            }
            if (mTvDefault != null) {
                mTvDefault.setText(getResources().getString(R.string.onboarding_set_default_india));
            }
        }
    }

    private void handleAnalyticsConsentPage() {
        mCurrentOnboardingPage = CurrentOnboardingPage.ANALYTICS_CONSENT_PAGE;
        int margin = mIsTablet ? 250 : 60;
        setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1.5f, margin, true);
        setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1.5f, margin, false);

        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.GONE);
        }
        if (mTvDefault != null) {
            mTvDefault.setVisibility(View.GONE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.GONE);
        }

        if (mTvCard != null) {
            mTvCard.setText(getResources().getString(R.string.p3a_title));
        }
        if (mBtnPositive != null) {
            mBtnPositive.setText(getResources().getString(R.string.continue_text));
        }
        if (mBtnNegative != null) {
            mBtnNegative.setText(getResources().getString(R.string.learn_more_onboarding));
            mBtnNegative.setVisibility(View.VISIBLE);
        }

        // Handle crash reporting consent based on installation status
        if (PackageUtils.isFirstInstall(this)
                && !OnboardingPrefManager.getInstance().isP3aCrashReportingMessageShown()) {
            // For first time installs, enable crash reporting by default
            if (mCheckboxCrash != null) {
                mCheckboxCrash.setChecked(true);
            }
            // Update metrics reporting consent
            UmaSessionStats.changeMetricsReportingConsent(
                    true, ChangeMetricsReportingStateCalledFrom.UI_FIRST_RUN);
            // Mark crash reporting message as shown
            OnboardingPrefManager.getInstance().setP3aCrashReportingMessageShown(true);
        } else {
            // For existing installations, restore previous crash reporting preference
            boolean isCrashReporting = false;
            try {
                // Get current crash reporting permission status
                isCrashReporting =
                        PrivacyPreferencesManagerImpl.getInstance()
                                .isUsageAndCrashReportingPermittedByUser();
            } catch (Exception e) {
                Log.e(TAG, "isCrashReportingOnboarding: " + e.getMessage());
            }
            // Update checkbox to match current preference
            if (mCheckboxCrash != null) {
                mCheckboxCrash.setChecked(isCrashReporting);
            }
        }

        if (mCheckboxCrash != null) {
            mCheckboxCrash.setOnCheckedChangeListener(
                    new CompoundButton.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                            try {
                                UmaSessionStats.changeMetricsReportingConsent(
                                        isChecked,
                                        ChangeMetricsReportingStateCalledFrom.UI_FIRST_RUN);
                            } catch (Exception e) {
                                Log.e(TAG, "CrashReportingOnboarding: " + e.getMessage());
                            }
                        }
                    });
        }

        boolean isP3aEnabled = true;

        try {
            isP3aEnabled = BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);
        } catch (Exception e) {
            Log.e(TAG, "P3aOnboarding: " + e.getMessage());
        }

        if (mCheckboxP3a != null) {
            mCheckboxP3a.setChecked(isP3aEnabled);
            mCheckboxP3a.setOnCheckedChangeListener(
                    new CompoundButton.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                            try {
                                BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, isChecked);
                                BraveLocalState.get()
                                        .setBoolean(BravePref.P3A_NOTICE_ACKNOWLEDGED, true);
                                BraveLocalState.commitPendingWrite();
                            } catch (Exception e) {
                                Log.e(TAG, "P3aOnboarding: " + e.getMessage());
                            }
                        }
                    });
        }

        if (mTvCard != null) {
            mTvCard.setVisibility(View.VISIBLE);
        }
        if (mCheckboxCrash != null) {
            mCheckboxCrash.setVisibility(View.VISIBLE);
        }
        if (mCheckboxP3a != null) {
            mCheckboxP3a.setVisibility(View.VISIBLE);
        }
        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.VISIBLE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.VISIBLE);
        }
    }

    private void showWDPPage() {
        int margin = mIsTablet ? 250 : 60;
        setLeafAnimation(mVLeafAlignTop, mIvLeafTop, 1.5f, margin, true);
        setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, 1.5f, margin, false);

        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.GONE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.GONE);
        }

        if (mTvCard != null) {
            mTvCard.setText(getResources().getString(R.string.wdp_title));
        }
        if (mTvDefault != null) {
            ChromeClickableSpan wdpLearnMoreClickableSpan =
                    new ChromeClickableSpan(
                            WelcomeOnboardingActivity.this,
                            R.color.brave_blue_tint_color,
                            (textView) -> {
                                CustomTabActivity.showInfoPage(this, WDP_LINK);
                            });
            String wdpText = getResources().getString(R.string.wdp_text);

            SpannableString wdpLearnMoreSpannableString =
                    SpanApplier.applySpans(
                            wdpText,
                            new SpanInfo(
                                    "<learn_more>", "</learn_more>", wdpLearnMoreClickableSpan));

            mTvDefault.setMovementMethod(LinkMovementMethod.getInstance());
            mTvDefault.setText(wdpLearnMoreSpannableString);
        }
        if (mBtnPositive != null) {
            mBtnPositive.setText(getResources().getString(R.string.sure_ill_help_onboarding));
        }
        if (mBtnNegative != null) {
            mBtnNegative.setText(getResources().getString(R.string.maybe_later));
            mBtnNegative.setVisibility(View.VISIBLE);
        }

        if (mTvCard != null) {
            mTvCard.setVisibility(View.VISIBLE);
        }

        if (mTvDefault != null) {
            mTvDefault.setVisibility(View.VISIBLE);
        }
        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.VISIBLE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.VISIBLE);
        }
        if (mCheckboxCrash != null) {
            mCheckboxCrash.setVisibility(View.GONE);
        }
        if (mCheckboxP3a != null) {
            mCheckboxP3a.setVisibility(View.GONE);
        }
    }

    private void setLeafAnimation(
            View leafAlignView,
            ImageView leafView,
            float scale,
            float leafMargin,
            boolean isTopLeaf) {
        if (leafMargin > 0 && leafAlignView != null) {
            int margin = (int) dpToPx(this, leafMargin);
            Animation animation =
                    new Animation() {
                        @Override
                        protected void applyTransformation(
                                float interpolatedTime, Transformation t) {
                            if (leafAlignView != null) {
                                ViewGroup.MarginLayoutParams layoutParams =
                                        (ViewGroup.MarginLayoutParams)
                                                leafAlignView.getLayoutParams();
                                if (isTopLeaf) {
                                    layoutParams.bottomMargin =
                                            margin
                                                    - (int)
                                                            ((margin - layoutParams.bottomMargin)
                                                                    * interpolatedTime);
                                } else {
                                    layoutParams.topMargin =
                                            margin
                                                    - (int)
                                                            ((margin - layoutParams.topMargin)
                                                                    * interpolatedTime);
                                }
                                leafAlignView.setLayoutParams(layoutParams);
                            }
                        }
                    };
            animation.setDuration(800);
            leafAlignView.startAnimation(animation);
        }
        if (leafView != null) {
            leafView.animate().scaleX(scale).scaleY(scale).setDuration(800);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (isActivityFinishingOrDestroyed()) return;
        nextOnboardingStep();
    }

    private void finishNativeInitializationPostWork() {
        assert mInitializeViewsDone;
        nextOnboardingStep();
    }

    @Override
    public void finishNativeInitialization() {
        ThreadUtils.assertOnUiThread();
        super.finishNativeInitialization();

        if (mInitializeViewsDone) {
            finishNativeInitializationPostWork();
        } else {
            mInvokePostWorkAtInitializeViews = true;
        }
    }

    @Override
    public @BackPressResult int handleBackPress() {
        return BackPressResult.SUCCESS;
    }

    @Override
    public void triggerLayoutInflation() {
        super.triggerLayoutInflation();

        mFirstRunFlowSequencer =
                new BraveFirstRunFlowSequencer(getProfileProviderSupplier()) {
                    @Override
                    public void onFlowIsKnown(boolean isChild) {
                        initializeViews();
                    }
                };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }

    private void maybeUpdateFirstRunDefaultValues() {
        if (PackageUtils.isFirstInstall(this)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(
                            BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE, false);
        }
    }
}
