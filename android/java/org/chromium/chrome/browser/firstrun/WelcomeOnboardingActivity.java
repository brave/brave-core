/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import static org.chromium.build.NullUtil.assumeNonNull;
import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser;
import static org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils.setDefaultBrowser;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.app.Activity;
import android.graphics.drawable.Animatable2;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.RemoteException;
import android.text.SpannableString;
import android.view.View;
import android.view.animation.OvershootInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;

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
import org.chromium.chrome.browser.BraveConstants;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.brave_origin.BraveOriginDeepLinkHandler;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.metrics.ChangeMetricsReportingStateCalledFrom;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.privacy.settings.PrivacyPreferencesManagerImpl;
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
    private static final String WDP_LINK =
            "https://www.brave.com/browser/privacy/#web-discovery-project";

    private static final String TAG = "WelcomeOnboarding";

    private static final String KEY_SPLASH_ANIMATION_FINISHED =
            "WelcomeOnboardingActivity.SplashAnimationFinished";
    private static final String KEY_PAGE_INDEX = "WelcomeOnboardingActivity.PageIndex";

    private static final float REDUCED_TENSION_OVERSHOOT_INTERPOLATOR = 1f;
    private static final float BRAVE_SPLASH_SCALE_ANIMATION = 0.4f;
    private static final int BRAVE_SPLASH_ANIMATION_DURATION_MS = 600;

    private boolean mIsTablet;
    private boolean mSplashAnimationFinished;
    private int mRestoredPageIndex;

    private ImageView mBraveSplash;
    @Nullable private FrameLayout mBraveSplashContainer;

    private Guideline mSplashGuideline;
    private ViewPager2 mOnboardingPager;

    private OnboardingStepAdapter mStepAdapter;
    private PageBounceAnimator mPageBounceAnimator;

    private boolean mIsP3aManaged;
    private boolean mIsCrashReportingManaged;

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
                                    } else if (referrerUrl.equals(
                                            BraveOriginDeepLinkHandler.PATH_TOKEN)) {
                                        ChromeSharedPreferences.getInstance()
                                                .writeBoolean(
                                                        BravePreferenceKeys
                                                                .BRAVE_DEFERRED_DEEPLINK_ORIGIN_PROMO, // presubmit: ignore-long-line
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
        UserPrefs.get(assumeNonNull(getProfileProviderSupplier().get()).getOriginalProfile())
                .setBoolean(BravePref.WEB_DISCOVERY_ENABLED, true);
    }

    private void finalStep() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(OnboardingPrefManager.SHOULD_SHOW_SEARCH_WIDGET_PROMO, true);
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

    private boolean isWDPSettingAvailable() {
        if (!BraveConfig.WEB_DISCOVERY_ENABLED) {
            return false;
        }
        return !UserPrefs.get(
                        assumeNonNull(getProfileProviderSupplier().get()).getOriginalProfile())
                .isManagedPreference(WebDiscoveryPrefs.WEB_DISCOVERY_ENABLED);
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
        mOnboardingPager = findViewById(R.id.onboarding_steps_pager);

        final ChromeClickableSpan wdpLearnMoreClickableSpan =
                new ChromeClickableSpan(
                        getColor(R.color.brave_blue_tint_color),
                        (textView) -> CustomTabActivity.showInfoPage(this, WDP_LINK));
        final String wdpText = getResources().getString(R.string.wdp_text);

        final SpannableString wdpLearnMore =
                SpanApplier.applySpans(
                        wdpText,
                        new SpanInfo("<learn_more>", "</learn_more>", wdpLearnMoreClickableSpan));

        mOnboardingPager.setUserInputEnabled(false);
        mPageBounceAnimator = new PageBounceAnimator(mOnboardingPager);
        mStepAdapter = new OnboardingStepAdapter(wdpLearnMore, this);
        mOnboardingPager.setAdapter(mStepAdapter);

        mBraveSplash = findViewById(R.id.brave_splash);
        mBraveSplashContainer = findViewById(R.id.brave_splash_container);
        assert !mIsTablet || mBraveSplashContainer != null
                : "R.id.brave_splash_container must be declared on tablet layout.";

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

        if (!mIsCrashReportingManaged) {
            final boolean isCrashReporting = getCrashReportingPreference();
            mStepAdapter.setCrashReportingChecked(isCrashReporting);
        } else {
            mStepAdapter.setCrashReportingManaged(true);
        }

        if (!mIsP3aManaged) {
            final boolean isP3aEnabled = getP3aPreference();
            mStepAdapter.setP3aChecked(isP3aEnabled);
        } else {
            mStepAdapter.setP3aManaged(true);
        }
        if (mSplashAnimationFinished) {
            if (mBraveSplashContainer != null) {
                mBraveSplashContainer.setVisibility(View.GONE);
            }
            mOnboardingPager.setCurrentItem(mRestoredPageIndex, false);
            mOnboardingPager.setVisibility(View.VISIBLE);
        } else {
            playAnimatedVectorDrawable();
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(KEY_SPLASH_ANIMATION_FINISHED, mSplashAnimationFinished);
        outState.putInt(KEY_PAGE_INDEX, mOnboardingPager.getCurrentItem());
    }

    @Override
    protected void onRestoreInstanceState(@Nullable Bundle state) {
        super.onRestoreInstanceState(state);
        if (state == null) {
            return;
        }
        mSplashAnimationFinished = state.getBoolean(KEY_SPLASH_ANIMATION_FINISHED, false);
        mRestoredPageIndex = state.getInt(KEY_PAGE_INDEX, 0);
    }

    private void playAnimatedVectorDrawable() {
        final AnimatedVectorDrawable vectorDrawable =
                (AnimatedVectorDrawable) mBraveSplash.getDrawable();
        vectorDrawable.registerAnimationCallback(
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
                            animateBraveSplash(vectorDrawable);
                        }
                    }
                });
        vectorDrawable.start();
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

    private void showPagerAfterSplash() {
        mOnboardingPager.setCurrentItem(isWDPSettingAvailable() ? 0 : 1, false);
        mOnboardingPager.setVisibility(View.VISIBLE);
        mSplashAnimationFinished = true;
        maybeRequestDefaultBrowser();
    }

    private void fadeBraveSplashContainer() {
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
                        assumeNonNull(mBraveSplashContainer).setVisibility(View.GONE);
                        showPagerAfterSplash();
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
                                showPagerAfterSplash();
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

    private void maybeRequestDefaultBrowser() {
        if (isActivityFinishingOrDestroyed()) {
            return;
        }
        if (!isBraveSetAsDefaultBrowser(this)) {
            setDefaultBrowser(this);
        }
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
                finalStep();
                return;
            }
        }
        mPageBounceAnimator.animateToPosition(position);
    }

    @Override
    public void onDismiss() {
        finalStep();
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
