/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

import static org.chromium.build.NullUtil.assumeNonNull;
import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser;
import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.setDefaultBrowser;
import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.LayoutTransition;
import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.Animatable2;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
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
import android.widget.FrameLayout;
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

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ApplicationStatus.ActivityStateListener;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.brave.browser.customize_menu.CustomizeBraveMenu;
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
    private static final String DAY_ZERO_DEFAULT_VARIANT = "X";
    private static final String DAY_ZERO_VARIANT_Y = "Y";

    private static final String KEY_SPLASH_ANIMATION_FINISHED =
            "WelcomeOnboardingActivity.SplashAnimationFinished";
    private static final String KEY_VARIANT_Y_PAGE_INDEX =
            "WelcomeOnboardingActivity.VariantYPageIndex";

    private static final float LEAF_SCALE_ANIMATION = 1.5f;
    private static final float REDUCED_TENSION_OVERSHOOT_INTERPOLATOR = 1f;
    private static final float BRAVE_SPLASH_SCALE_ANIMATION = 0.4f;
    private static final int BRAVE_SPLASH_ANIMATION_DURATION_MS = 600;

    private boolean mIsTablet;
    private int mCurrentStep = -1;
    private boolean mSplashAnimationFinished;
    private int mRestoredVariantYPageIndex;

    private ConstraintLayout mDefaultConstraintLayout;
    private ConstraintLayout mVariantYConstraintLayout;
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
    private FrameLayout mBraveSplashContainer;

    private Guideline mSplashGuideline;
    private ViewPager2 mVariantYPager;
    @Nullable private OnboardingStepAdapter mVariantYAdapter;

    private String mDayZeroVariant = "";
    private SpannableString mWdpLearnMore;
    @Nullable private PageBounceAnimator mPageBounceAnimator;
    private boolean mIsP3aManaged;
    private boolean mIsCrashReportingManaged;

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
        if ((isWDPSettingAvailable() && mCurrentOnboardingPage == CurrentOnboardingPage.WDP_PAGE)
                || isVariantY()) {
            UserPrefs.get(assumeNonNull(getProfileProviderSupplier().get()).getOriginalProfile())
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
        CustomizeBraveMenu.initDefaultInvisibleItems(getResources());
        OnboardingPrefManager.getInstance().setP3aOnboardingShown(true);

        FirstRunStatus.setFirstRunFlowComplete(true);

        FirstRunUtils.setEulaAccepted();

        launchPendingIntentAndFinish();
    }

    /**
     * Sends the post-FRE intent first and delays finishing this activity until either another
     * activity is resumed or this activity is already stopped/destroyed.
     *
     * <p>This avoids handoff races where finishing too early can cause a brief re-show/flash of the
     * onboarding activity during task/window transitions (for example around fold/unfold state
     * changes).
     *
     * <p>This mirrors the upstream robust pattern used in FirstRunActivity and is specifically
     * meant to avoid transition glitches (for example on foldable state changes).
     */
    private void launchPendingIntentAndFinish() {
        if (!sendFirstRunCompleteIntent()) {
            finish();
        } else {
            ApplicationStatus.registerStateListenerForAllActivities(
                    new ActivityStateListener() {
                        @Override
                        public void onActivityStateChange(Activity activity, int newState) {
                            boolean shouldFinish;
                            if (activity == WelcomeOnboardingActivity.this) {
                                shouldFinish =
                                        (newState == ActivityState.STOPPED
                                                || newState == ActivityState.DESTROYED);
                            } else {
                                shouldFinish = newState == ActivityState.RESUMED;
                            }
                            if (shouldFinish) {
                                finish();
                                ApplicationStatus.unregisterActivityStateListener(this);
                            }
                        }
                    });
        }
    }

    private void handleSetAsDefaultStep() {
        mCurrentOnboardingPage = CurrentOnboardingPage.SET_AS_DEFAULT;
        if (!maybeRequestDefaultBrowser()) {
            nextOnboardingStepForDefaultVariant();
        }
    }

    private void handleWDPStep() {
        if (!isWDPSettingAvailable()) {
            nextOnboardingStepForDefaultVariant();
            return;
        }

        mCurrentOnboardingPage = CurrentOnboardingPage.WDP_PAGE;
        if (isDefaultVariant() && mIvBrave != null) {
            mIvBrave.setVisibility(View.VISIBLE);
        }
        showWDPPage();
    }

    private boolean isWDPSettingAvailable() {
        if (!BraveConfig.WEB_DISCOVERY_ENABLED) {
            return false;
        }
        return !UserPrefs.get(
                        assumeNonNull(getProfileProviderSupplier().get()).getOriginalProfile())
                .isManagedPreference(WebDiscoveryPrefs.WEB_DISCOVERY_ENABLED);
    }

    private void handleAnalyticsConsentPage() {
        // If both settings are managed by policy, skip this page entirely
        if (mIsP3aManaged && mIsCrashReportingManaged) {
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

        if (!mIsCrashReportingManaged) {
            final boolean isCrashReporting = getCrashReportingPreference();
            if (mCheckboxCrash != null) {
                mCheckboxCrash.setChecked(isCrashReporting);
                mCheckboxCrash.setOnCheckedChangeListener(
                        (buttonView, isChecked) -> setMetricsReportingConsent(isChecked, false));
            }
        }

        if (!mIsP3aManaged) {
            final boolean isP3aEnabled = getP3aPreference();
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
            mCheckboxCrash.setVisibility(mIsCrashReportingManaged ? View.GONE : View.VISIBLE);
        }
        if (mCheckboxP3a != null) {
            mCheckboxP3a.setVisibility(mIsP3aManaged ? View.GONE : View.VISIBLE);
        }
        if (mLayoutCard != null) {
            mLayoutCard.setVisibility(View.VISIBLE);
        }
        if (mIvArrowDown != null) {
            mIvArrowDown.setVisibility(View.VISIBLE);
        }
    }

    private void setMetricsReportingConsent(final boolean consent, final boolean markAsShown) {
        try {
            // Updates reporting consent for first run.
            UmaSessionStats.changeMetricsReportingConsent(
                    consent, ChangeMetricsReportingStateCalledFrom.UI_FIRST_RUN);
        } catch (Exception e) {
            Log.e(TAG, "CrashReportingOnboarding", e);
        }
        if (markAsShown) {
            // Marks crash reporting message as shown.
            OnboardingPrefManager.getInstance().setP3aCrashReportingMessageShown(true);
        }
    }

    private void setP3aConsent(final boolean consent) {
        try {
            BraveLocalState.get().setBoolean(BravePref.P3A_ENABLED, consent);
            BraveLocalState.get().setBoolean(BravePref.P3A_NOTICE_ACKNOWLEDGED, true);
            BraveLocalState.commitPendingWrite();
        } catch (Exception e) {
            Log.e(TAG, "P3aOnboarding", e);
        }
    }

    private void showWDPPage() {
        if (isDefaultVariant()) {
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
            if (isDefaultVariant()) {
                nextOnboardingStepForDefaultVariant();
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        assert requestCode == BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE;
        if (isDefaultVariant()) {
            nextOnboardingStepForDefaultVariant();
        }
    }

    @Override
    protected void performPreInflationStartup() {
        super.performPreInflationStartup();
        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(this);
        if (mIsTablet && Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM) {
            setRequestedOrientation(SCREEN_ORIENTATION_UNSPECIFIED);
        }
    }

    @Override
    public void triggerLayoutInflation() {
        super.triggerLayoutInflation();
        setContentView(R.layout.activity_welcome_onboarding);

        mSplashGuideline = findViewById(R.id.splash_anchor);
        mVariantYPager = findViewById(R.id.onboarding_steps_pager);

        final ChromeClickableSpan wdpLearnMoreClickableSpan =
                new ChromeClickableSpan(
                        getColor(R.color.brave_blue_tint_color),
                        (textView) -> CustomTabActivity.showInfoPage(this, WDP_LINK));
        final String wdpText = getResources().getString(R.string.wdp_text);

        mWdpLearnMore =
                SpanApplier.applySpans(
                        wdpText,
                        new SpanInfo("<learn_more>", "</learn_more>", wdpLearnMoreClickableSpan));

        if (mVariantYPager != null) {
            mVariantYPager.setUserInputEnabled(false);
            mPageBounceAnimator = new PageBounceAnimator(mVariantYPager);
            mVariantYAdapter = new OnboardingStepAdapter(mWdpLearnMore, this);
            mVariantYPager.setAdapter(mVariantYAdapter);
        }

        mDefaultConstraintLayout = findViewById(R.id.onboarding_default_variant);
        mVariantYConstraintLayout = findViewById(R.id.onboarding_variant_y);
        mBraveSplash = findViewById(R.id.brave_splash);
        mBraveSplashContainer = findViewById(R.id.brave_splash_container);
        assert !mIsTablet || mBraveSplashContainer != null
                : "R.id.brave_splash_container must be declared on tablet layout.";
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
                        if (mCurrentStep != 0 || !maybeRequestDefaultBrowser()) {
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

        mIsP3aManaged = BraveLocalState.get().isManagedPreference(BravePref.P3A_ENABLED);
        mIsCrashReportingManaged =
                !PrivacyPreferencesManagerImpl.getInstance()
                        .isUsageAndCrashReportingPermittedByPolicy();

        mDayZeroVariant = DayZeroHelper.getDayZeroVariant();
        // Filter out day zero variants different from X and Y.
        if (TextUtils.isEmpty(mDayZeroVariant)
                || (!mDayZeroVariant.equals(DAY_ZERO_VARIANT_Y)
                        && !mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT))) {
            mDayZeroVariant = DAY_ZERO_DEFAULT_VARIANT;
        }

        if (isVariantY()) {
            if (!mIsCrashReportingManaged) {
                final boolean isCrashReporting = getCrashReportingPreference();
                if (mVariantYAdapter != null) {
                    mVariantYAdapter.setCrashReportingChecked(isCrashReporting);
                }
            } else if (mVariantYAdapter != null) {
                mVariantYAdapter.setCrashReportingManaged(true);
            }

            if (!mIsP3aManaged) {
                final boolean isP3aEnabled = getP3aPreference();
                if (mVariantYAdapter != null) {
                    mVariantYAdapter.setP3aChecked(isP3aEnabled);
                }
            } else if (mVariantYAdapter != null) {
                mVariantYAdapter.setP3aManaged(true);
            }
            mVariantYConstraintLayout.setVisibility(View.VISIBLE);
            if (mSplashAnimationFinished) {
                if (mBraveSplashContainer != null) {
                    mBraveSplashContainer.setVisibility(View.GONE);
                }
                if (mVariantYPager != null) {
                    mVariantYPager.setCurrentItem(mRestoredVariantYPageIndex, false);
                    mVariantYPager.setVisibility(View.VISIBLE);
                }
            } else {
                final AnimatedVectorDrawable vectorDrawable = getAnimatedVectorDrawable();
                vectorDrawable.start();
            }

        } else {
            mDefaultConstraintLayout.setVisibility(View.VISIBLE);
            nextOnboardingStepForDefaultVariant();
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(KEY_SPLASH_ANIMATION_FINISHED, mSplashAnimationFinished);
        if (mVariantYPager != null) {
            outState.putInt(KEY_VARIANT_Y_PAGE_INDEX, mVariantYPager.getCurrentItem());
        }
    }

    @Override
    protected void onRestoreInstanceState(@Nullable Bundle state) {
        super.onRestoreInstanceState(state);
        if (state == null) {
            return;
        }
        mSplashAnimationFinished = state.getBoolean(KEY_SPLASH_ANIMATION_FINISHED, false);
        mRestoredVariantYPageIndex = state.getInt(KEY_VARIANT_Y_PAGE_INDEX, 0);
    }

    private boolean isDefaultVariant() {
        return mDayZeroVariant.equals(DAY_ZERO_DEFAULT_VARIANT);
    }

    private boolean isVariantY() {
        return mDayZeroVariant.equals(DAY_ZERO_VARIANT_Y);
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
                        if (isActivityFinishingOrDestroyed()) {
                            return;
                        }
                        if (mIsTablet) {
                            fadeBraveSplashContainer();
                        } else {
                            animateBraveSplash(result);
                        }
                    }
                });
        return result;
    }

    private boolean getCrashReportingPreference() {
        if (PackageUtils.isFirstInstall(this)
                && !OnboardingPrefManager.getInstance().isP3aCrashReportingMessageShown()) {
            setMetricsReportingConsent(true, true);
            return true;
        }

        boolean isCrashReporting = false;
        try {
            isCrashReporting =
                    PrivacyPreferencesManagerImpl.getInstance()
                            .isUsageAndCrashReportingPermittedByUser();
        } catch (Exception e) {
            Log.e(TAG, "CrashReportingOnboarding", e);
        }
        return isCrashReporting;
    }

    private boolean getP3aPreference() {
        boolean isP3aEnabled = true;
        try {
            isP3aEnabled = BraveLocalState.get().getBoolean(BravePref.P3A_ENABLED);
        } catch (Exception e) {
            Log.e(TAG, "P3aOnboarding", e);
        }
        return isP3aEnabled;
    }

    private void showVariantYPagerAfterSplash() {
        if (mVariantYPager != null) {
            mVariantYPager.setCurrentItem(isWDPSettingAvailable() ? 0 : 1, false);
            mVariantYPager.setVisibility(View.VISIBLE);
        }
        mSplashAnimationFinished = true;
        maybeRequestDefaultBrowser();
    }

    private void fadeBraveSplashContainer() {
        if (mBraveSplashContainer == null) {
            return;
        }
        Animator animator =
                AnimatorInflater.loadAnimator(
                        WelcomeOnboardingActivity.this, R.animator.ic_brave_splash_fade_out);
        animator.setTarget(mBraveSplashContainer);
        animator.addListener(
                new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationCancel(@NonNull Animator animation) {
                        /* No-op. */
                    }

                    @Override
                    public void onAnimationEnd(@NonNull Animator animation) {
                        mBraveSplashContainer.setVisibility(View.GONE);
                        showVariantYPagerAfterSplash();
                    }

                    @Override
                    public void onAnimationRepeat(@NonNull Animator animation) {
                        /* No-op. */
                    }

                    @Override
                    public void onAnimationStart(@NonNull Animator animation) {
                        /* No-op. */
                    }
                });
        animator.start();
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
                                if (mBraveSplashContainer != null) {
                                    mBraveSplashContainer.setVisibility(View.GONE);
                                }
                                showVariantYPagerAfterSplash();
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

    private boolean maybeRequestDefaultBrowser() {
        if (isActivityFinishingOrDestroyed()) {
            return false;
        }
        if (!isBraveSetAsDefaultBrowser(this)) {
            setDefaultBrowser(this);
            return true;
        }
        return false;
    }

    @Override
    public @BackPressResult int handleBackPress() {
        return BackPressResult.SUCCESS;
    }

    @Override
    public void onRequestPageChange(final int position) {
        if (position == 2) {
            // If both settings are managed by policy, skip this page entirely
            if (mIsP3aManaged && mIsCrashReportingManaged) {
                finalStep(true);
                return;
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

    @Override
    public void onCrashReportingPreferenceChanged(final boolean enabled) {
        setMetricsReportingConsent(enabled, false);
    }

    @Override
    public void onP3aPreferenceChanged(final boolean enabled) {
        setP3aConsent(enabled);
    }
}
