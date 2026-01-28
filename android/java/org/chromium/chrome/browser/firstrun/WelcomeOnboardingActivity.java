/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser;
import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.setDefaultBrowser;
import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.Animator;
import android.animation.LayoutTransition;
import android.content.Intent;
import android.graphics.drawable.Animatable2;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.os.RemoteException;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.OvershootInterpolator;
import android.view.animation.Transformation;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.Guideline;
import androidx.viewpager2.widget.ViewPager2;

import com.android.installreferrer.api.InstallReferrerClient;
import com.android.installreferrer.api.InstallReferrerClient.InstallReferrerResponse;
import com.android.installreferrer.api.InstallReferrerStateListener;
import com.android.installreferrer.api.ReferrerDetails;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
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
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.web_discovery.WebDiscoveryPrefs;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

/**
 * Activity responsible for handling the first-run onboarding experience for new Brave browser
 * installations.
 *
 * <p>This activity extends {@link FirstRunActivityBase} and guides users through a series of
 * onboarding steps:
 *
 * <ul>
 *   <li>Setting Brave as the default browser
 *   <li>Configuring privacy and analytics preferences (P3A and crash reporting)
 *   <li>Accepting the Terms of Service
 * </ul>
 *
 * <p>The onboarding flow uses animations and clear UI elements to introduce Brave’s key features
 * and privacy-focused approach.
 */
@NullMarked
public class WelcomeOnboardingActivity extends FirstRunActivityBase
        implements OnboardingStepAdapter.OnboardingNavigationListener {
    private static final String P3A_URL =
            "https://support.brave.app/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave";
    private static final String WDP_LINK =
            "https://www.brave.com/browser/privacy/#web-discovery-project";

    private static final String TAG = "WelcomeOnboarding";
    private static final String DAY_ZERO_DEFAULT_VARIANT = "a";
    private static final String DAY_ZERO_VARIANT_B = "b";

    private static final float LEAF_SCALE_ANIMATION = 1.5f;
    private static final float REDUCED_TENSION_OVERSHOOT_INTERPOLATOR = 1f;
    private static final float BRAVE_SPLASH_SCALE_ANIMATION = 0.4f;
    private static final int BRAVE_SPLASH_ANIMATION_DURATION_MS = 600;

    private boolean mIsTablet;
    private int mCurrentStep = -1;

    private ConstraintLayout mDefaultConstraintLayout;
    private ConstraintLayout mVariantBConstraintLayout;
    private View mVLeafAlignTop;
    private View mVLeafAlignBottom;
    private ImageView mIvLeafTop;
    private ImageView mIvLeafBottom;
    private ImageView mBraveSplash;
    private ImageView mIvBrave;
    private ImageView mIvArrowDown;
    private LinearLayout mLayoutCard;
    private TextView mTvCard;
    private TextView mTvDefault;
    private Button mBtnPositive;
    private Button mBtnNegative;
    private CheckBox mCheckboxCrash;
    private CheckBox mCheckboxP3a;

    private Guideline mSplashGuideline;
    private ViewPager2 mVariantBPager;
    @Nullable private OnboardingStepAdapter mVariantBAdapter;

    private String mDayZeroVariant = "";
    private SpannableString mWdpLearnMore;
    @Nullable private PageBounceAnimator mPageBounceAnimator;

    private enum CurrentOnboardingPage {
        SET_AS_DEFAULT,
        NOTIFICATION_PERMISSION,
        WDP_PAGE,
        ANALYTICS_CONSENT_PAGE
    }

    @Nullable private CurrentOnboardingPage mCurrentOnboardingPage;

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
                                    Log.e(TAG, "Could not get referral", e);
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

    private void enableWebDiscoverPreference() {
        final ProfileProvider profileProvider = getProfileProviderSupplier().get();
        if (profileProvider == null) {
            return;
        }
        if ((isWDPSettingAvailable() && mCurrentOnboardingPage == CurrentOnboardingPage.WDP_PAGE)
                || mDayZeroVariant.equals(DAY_ZERO_VARIANT_B)) {
            UserPrefs.get(profileProvider.getOriginalProfile())
                    .setBoolean(BravePref.WEB_DISCOVERY_ENABLED, true);
        }
    }

    private void nextOnboardingStepForDefaultVariant() {
        if (isActivityFinishingOrDestroyed()) {
            return;
        }

        mCurrentStep++;
        if (mCurrentStep == 0) {
            handleSetAsDefaultStep();
        } else if (mCurrentStep == 1) {
            handleWDPStep();
        } else if (mCurrentStep == 2) {
            handleAnalyticsConsentPage();
        } else {
            finalStep(false);
        }
    }

    private void finalStep(final boolean showSearchWidgetPromoPanel) {
        if (showSearchWidgetPromoPanel) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(OnboardingPrefManager.SHOULD_SHOW_SEARCH_WIDGET_PROMO, true);
        }
        OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);

        FirstRunStatus.setFirstRunFlowComplete(true);

        FirstRunUtils.setEulaAccepted();

        finish();
        sendFirstRunCompleteIntent();
    }

    private void handleSetAsDefaultStep() {
        mCurrentOnboardingPage = CurrentOnboardingPage.SET_AS_DEFAULT;
        if (!isBraveSetAsDefaultBrowser(this)) {
            setDefaultBrowser(this);
        } else {
            nextOnboardingStepForDefaultVariant();
        }
    }

    private void handleWDPStep() {
        if (!isWDPSettingAvailable()) {
            nextOnboardingStepForDefaultVariant();
            return;
        }

        mCurrentOnboardingPage = CurrentOnboardingPage.WDP_PAGE;
        if (mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT) && mIvBrave != null) {
            mIvBrave.setVisibility(View.VISIBLE);
        }
        showWDPPage();
    }

    private boolean isWDPSettingAvailable() {
        if (!BraveConfig.WEB_DISCOVERY_ENABLED) {
            return false;
        }

        final ProfileProvider profileProvider = getProfileProviderSupplier().get();
        return profileProvider != null
                && !UserPrefs.get(profileProvider.getOriginalProfile())
                        .isManagedPreference(WebDiscoveryPrefs.WEB_DISCOVERY_ENABLED);
    }

    private void handleAnalyticsConsentPage() {
        boolean isP3aManaged = BraveLocalState.get().isManagedPreference(BravePref.P3A_ENABLED);
        boolean isCrashReportingManaged =
                !PrivacyPreferencesManagerImpl.getInstance()
                        .isUsageAndCrashReportingPermittedByPolicy();

        // If both settings are managed by policy, skip this page entirely
        if (isP3aManaged && isCrashReportingManaged) {
            nextOnboardingStepForDefaultVariant();
            return;
        }

        mCurrentOnboardingPage = CurrentOnboardingPage.ANALYTICS_CONSENT_PAGE;
        int margin = mIsTablet ? 250 : 60;
        setLeafAnimation(mVLeafAlignTop, mIvLeafTop, margin, true);
        setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, margin, false);

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

        if (!isCrashReportingManaged) {
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
                    Log.e(TAG, "CrashReportingOnboarding", e);
                }
                // Update checkbox to match current preference
                if (mCheckboxCrash != null) {
                    mCheckboxCrash.setChecked(isCrashReporting);
                }
            }

            if (mCheckboxCrash != null) {
                mCheckboxCrash.setOnCheckedChangeListener(
                        (buttonView, isChecked) -> setMetricsReportingConsent(isChecked));
            }
        }

        if (!isP3aManaged) {
            boolean isP3aEnabled = true;

            try {
                isP3aEnabled = BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);
            } catch (Exception e) {
                Log.e(TAG, "P3aOnboarding", e);
            }

            if (mCheckboxP3a != null) {
                mCheckboxP3a.setChecked(isP3aEnabled);
                mCheckboxP3a.setOnCheckedChangeListener(
                        (buttonView, isChecked) -> setP3aConsent(isChecked));
            }
        }

        if (mTvCard != null) {
            mTvCard.setVisibility(View.VISIBLE);
        }
        if (mCheckboxCrash != null) {
            mCheckboxCrash.setVisibility(isCrashReportingManaged ? View.GONE : View.VISIBLE);
        }
        if (mCheckboxP3a != null) {
            mCheckboxP3a.setVisibility(isP3aManaged ? View.GONE : View.VISIBLE);
        }
        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.VISIBLE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.VISIBLE);
        }
    }

    public static void setMetricsReportingConsent(final boolean consent) {
        try {
            UmaSessionStats.changeMetricsReportingConsent(
                    consent, ChangeMetricsReportingStateCalledFrom.UI_FIRST_RUN);
        } catch (Exception e) {
            Log.e(TAG, "CrashReportingOnboarding", e);
        }
    }

    public static void setP3aConsent(final boolean consent) {
        try {
            BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, consent);
            BraveLocalState.get().setBoolean(BravePref.P3A_NOTICE_ACKNOWLEDGED, true);
            BraveLocalState.commitPendingWrite();
        } catch (Exception e) {
            Log.e(TAG, "P3aOnboarding", e);
        }
    }

    private void showWDPPage() {
        if (mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT)) {
            int margin = mIsTablet ? 250 : 60;
            setLeafAnimation(mVLeafAlignTop, mIvLeafTop, margin, true);
            setLeafAnimation(mVLeafAlignBottom, mIvLeafBottom, margin, false);

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
                mTvDefault.setMovementMethod(LinkMovementMethod.getInstance());
                mTvDefault.setText(mWdpLearnMore);
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
    }

    private void setLeafAnimation(
            View leafAlignView, ImageView leafView, float leafMargin, boolean isTopLeaf) {
        if (leafMargin > 0 && leafAlignView != null) {
            int margin = dpToPx(this, leafMargin);
            Animation animation =
                    new Animation() {
                        @Override
                        protected void applyTransformation(float time, Transformation t) {
                            transformLeaf(time, margin, leafAlignView, isTopLeaf);
                        }
                    };
            animation.setDuration(800);
            leafAlignView.startAnimation(animation);
        }
        if (leafView != null) {
            leafView.animate()
                    .scaleX(LEAF_SCALE_ANIMATION)
                    .scaleY(LEAF_SCALE_ANIMATION)
                    .setDuration(800);
        }
    }

    private void transformLeaf(
            final float time, final int margin, final View leafAlignView, final boolean isTopLeaf) {
        ViewGroup.MarginLayoutParams layoutParams =
                (ViewGroup.MarginLayoutParams) leafAlignView.getLayoutParams();
        if (isTopLeaf) {
            layoutParams.bottomMargin =
                    margin - (int) ((margin - layoutParams.bottomMargin) * time);
        } else {
            layoutParams.topMargin = margin - (int) ((margin - layoutParams.topMargin) * time);
        }
        leafAlignView.setLayoutParams(layoutParams);
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == BravePermissionUtils.NOTIFICATION_PERMISSION_CODE) {
            if (mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT)) {
                nextOnboardingStepForDefaultVariant();
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        assert requestCode == BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE;
        if (mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT)) {
            nextOnboardingStepForDefaultVariant();
        }
    }

    @Override
    protected void performPreInflationStartup() {
        super.performPreInflationStartup();
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(this);
    }

    @Override
    public void triggerLayoutInflation() {
        super.triggerLayoutInflation();
        setContentView(R.layout.activity_welcome_onboarding);

        mSplashGuideline = findViewById(R.id.splash_anchor);
        mVariantBPager = findViewById(R.id.onboarding_steps_pager);

        final ChromeClickableSpan wdpLearnMoreClickableSpan =
                new ChromeClickableSpan(
                        getColor(R.color.brave_blue_tint_color),
                        (textView) -> CustomTabActivity.showInfoPage(this, WDP_LINK));
        final String wdpText = getResources().getString(R.string.wdp_text);

        mWdpLearnMore =
                SpanApplier.applySpans(
                        wdpText,
                        new SpanInfo("<learn_more>", "</learn_more>", wdpLearnMoreClickableSpan));

        if (mVariantBPager != null) {
            mVariantBPager.setUserInputEnabled(true);
            mPageBounceAnimator = new PageBounceAnimator(mVariantBPager);
            mVariantBAdapter = new OnboardingStepAdapter(mWdpLearnMore, this);
            mVariantBPager.setAdapter(mVariantBAdapter);
        }

        mDefaultConstraintLayout = findViewById(R.id.onboarding_default_variant);
        mVariantBConstraintLayout = findViewById(R.id.onboarding_variant_b);
        mBraveSplash = findViewById(R.id.brave_splash);
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

        if (mBtnPositive != null) {
            mBtnPositive.setOnClickListener(
                    view -> {
                        if (mCurrentStep == 0 && !isBraveSetAsDefaultBrowser(this)) {
                            setDefaultBrowser(this);
                        } else {
                            enableWebDiscoverPreference();
                            nextOnboardingStepForDefaultVariant();
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
                            nextOnboardingStepForDefaultVariant();
                        }
                    });
        }
        checkReferral();
        if (PackageUtils.isFirstInstall(this)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(
                            BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE, false);
        }
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        mDayZeroVariant = DayZeroHelper.getDayZeroVariant();
        if (TextUtils.isEmpty(mDayZeroVariant)) {
            mDayZeroVariant = DAY_ZERO_DEFAULT_VARIANT;
        }

        if (mDayZeroVariant.equals(DAY_ZERO_VARIANT_B)) {
            mVariantBConstraintLayout.setVisibility(View.VISIBLE);
            final AnimatedVectorDrawable vectorDrawable = getAnimatedVectorDrawable();
            vectorDrawable.start();

        } else {
            mDefaultConstraintLayout.setVisibility(View.VISIBLE);
            nextOnboardingStepForDefaultVariant();
        }
    }

    private AnimatedVectorDrawable getAnimatedVectorDrawable() {
        final AnimatedVectorDrawable result = (AnimatedVectorDrawable) mBraveSplash.getDrawable();
        result.registerAnimationCallback(
                new Animatable2.AnimationCallback() {

                    @Override
                    public void onAnimationStart(Drawable drawable) {
                        super.onAnimationStart(drawable);
                        mBraveSplash.setAlpha(1f);
                    }

                    @Override
                    public void onAnimationEnd(Drawable drawable) {
                        animateBraveSplash(result);
                    }
                });
        return result;
    }

    private void animateBraveSplash(final AnimatedVectorDrawable vectorDrawable) {
        vectorDrawable.clearAnimationCallbacks();

        final View parent = (View) mBraveSplash.getParent();
        final float deltaY = parent.getPaddingTop() - mBraveSplash.getY();
        final int splashHeight = mBraveSplash.getHeight();
        // Compensate because shrinking around center moves the top edge down.
        final float compensation = (splashHeight * (1f - BRAVE_SPLASH_SCALE_ANIMATION)) / 2f;

        mBraveSplash
                .animate()
                .translationY(deltaY - compensation)
                .scaleX(BRAVE_SPLASH_SCALE_ANIMATION)
                .scaleY(BRAVE_SPLASH_SCALE_ANIMATION)
                .setDuration(BRAVE_SPLASH_ANIMATION_DURATION_MS)
                .setInterpolator(new OvershootInterpolator(REDUCED_TENSION_OVERSHOOT_INTERPOLATOR))
                .setListener(
                        new Animator.AnimatorListener() {
                            @Override
                            public void onAnimationCancel(@NonNull Animator animation) {
                                /* No-op. */
                            }

                            @Override
                            public void onAnimationEnd(@NonNull Animator animation) {
                                // Splash visual bottom in parent coordinates (center pivot
                                // scaling).
                                final float splashBottomPx =
                                        mBraveSplash.getBottom()
                                                + mBraveSplash.getTranslationY()
                                                - splashHeight
                                                        * (1f - mBraveSplash.getScaleY())
                                                        / 2f;

                                ConstraintLayout.LayoutParams guidelineLayoutParams =
                                        (ConstraintLayout.LayoutParams)
                                                mSplashGuideline.getLayoutParams();
                                guidelineLayoutParams.guideBegin = Math.round(splashBottomPx);
                                mSplashGuideline.setLayoutParams(guidelineLayoutParams);

                                if (mVariantBPager != null) {
                                    mVariantBPager.setCurrentItem(
                                            isWDPSettingAvailable() ? 0 : 1, false);
                                    mVariantBPager.setVisibility(View.VISIBLE);
                                }

                                if (!isBraveSetAsDefaultBrowser(WelcomeOnboardingActivity.this)) {
                                    setDefaultBrowser(WelcomeOnboardingActivity.this);
                                }
                            }

                            @Override
                            public void onAnimationRepeat(@NonNull Animator animation) {
                                /* No-op. */
                            }

                            @Override
                            public void onAnimationStart(@NonNull Animator animation) {
                                /* No-op. */
                            }
                        })
                .start();
    }

    @Override
    public @BackPressResult int handleBackPress() {
        return BackPressResult.SUCCESS;
    }

    @Override
    public void onRequestPageChange(final int position) {
        if (position == 2) {
            boolean isP3aManaged = BraveLocalState.get().isManagedPreference(BravePref.P3A_ENABLED);
            boolean isCrashReportingManaged =
                    !PrivacyPreferencesManagerImpl.getInstance()
                            .isUsageAndCrashReportingPermittedByPolicy();

            // If both settings are managed by policy, skip this page entirely
            if (isP3aManaged && isCrashReportingManaged) {
                finalStep(true);
                return;
            }

            if (!isCrashReportingManaged) {
                // Handle crash reporting consent based on installation status
                if (PackageUtils.isFirstInstall(this)
                        && !OnboardingPrefManager.getInstance().isP3aCrashReportingMessageShown()) {
                    // For first time installs, enable crash reporting by default
                    if (mVariantBAdapter != null) {
                        mVariantBAdapter.setCrashReportingChecked(true);
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
                        Log.e(TAG, "CrashReportingOnboarding", e);
                    }
                    // Update checkbox to match current preference
                    if (mVariantBAdapter != null) {
                        mVariantBAdapter.setCrashReportingChecked(isCrashReporting);
                    }
                }
            } else if (mVariantBAdapter != null) {
                mVariantBAdapter.setCrashReportingManaged(true);
            }

            if (!isP3aManaged) {
                boolean isP3aEnabled = true;

                try {
                    isP3aEnabled = BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);
                } catch (Exception e) {
                    Log.e(TAG, "P3aOnboarding", e);
                }

                if (mVariantBAdapter != null) {
                    mVariantBAdapter.setP3aChecked(isP3aEnabled);
                }
            } else if (mVariantBAdapter != null) {
                mVariantBAdapter.setP3aManaged(true);
            }
        }
        if (mPageBounceAnimator != null) {
            mPageBounceAnimator.animateToPosition(position);
        }
    }

    @Override
    public void onDismiss() {
        finalStep(true);
    }

    @Override
    public void onWebDiscoverPreferenceEnabled() {
        enableWebDiscoverPreference();
    }
}
