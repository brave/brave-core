/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import android.app.Activity;
import android.app.Application;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.ui.modaldialog.DialogDismissalCause;

import java.lang.ref.WeakReference;

/**
 * Singleton manager for the browser-wide biometric lock. Its lifetime matches the Application
 * process, so it remains active regardless of which individual activities are alive.
 *
 * <p>Initialization is split into two phases:
 *
 * <ol>
 *   <li><b>Phase 1 (app startup)</b> — {@link #initialize(Application)} is called from {@link
 *       BraveApplicationImplBase#onCreate()}, before native loads. This registers application
 *       lifecycle callbacks so FLAG_SECURE is set on every new activity window immediately on
 *       creation, and the lock is armed whenever all activities are stopped.
 *   <li><b>Phase 2 (native ready)</b> — {@link #onNativeInitialized} is invoked as soon as the
 *       first regular {@link Profile} is added (via a {@link ProfileManager.Observer} registered in
 *       Phase 1), so it fires for <em>any</em> activity that initialises the browser process — not
 *       only {@link BraveTabbedRootUiCoordinator}. The tabbed coordinator also calls it directly as
 *       a belt-and-suspenders update.
 * </ol>
 *
 * <p>The overlay is shown unconditionally whenever the lock is armed. Because it is attached to the
 * window's decor view, in-app navigation (e.g. back-pressing out of the incognito reauth dialog to
 * regular tabs) moves content around beneath the overlay but never dismisses it — there is no
 * escape path without authenticating.
 *
 * <p>Before native is initialized the {@link Profile} needed for biometric auth is unavailable. To
 * prevent content from being visible during that window, a lightweight opaque overlay is attached
 * to each activity's decor view as soon as it starts. When {@link #onNativeInitialized} fires the
 * placeholder overlays are removed and replaced by the real coordinator.
 */
@NullMarked
// Chromium's wrapper doesn't give us a way to register a listener for changes.
@SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
public class BraveBrowserLockManager implements ApplicationStatus.ActivityStateListener {
    private static final Object PRE_NATIVE_OVERLAY_TAG = new Object();

    private static @Nullable BraveBrowserLockManager sInstance;

    private @Nullable Profile mProfile;

    private boolean mNativeInitializedOnce;
    private boolean mLockArmed;
    private @Nullable BraveBrowserLockCoordinator mCoordinator;
    private @Nullable IncognitoReauthManager mIncognitoReauthManager;
    private @Nullable WeakReference<Activity> mCoordinatorActivity;

    private final ProfileManager.Observer mProfileObserver =
            new ProfileManager.Observer() {
                @Override
                public void onProfileAdded(Profile profile) {
                    if (!profile.isOffTheRecord()) {
                        ProfileManager.removeObserver(this);
                        onNativeInitialized(profile);
                    }
                }

                @Override
                public void onProfileDestroyed(Profile profile) {}
            };

    private final Application.ActivityLifecycleCallbacks mAppLifecycleCallbacks =
            new Application.ActivityLifecycleCallbacks() {
                @Override
                public void onActivityCreated(
                        Activity activity, @Nullable Bundle savedInstanceState) {
                    if (isBrowserLockEnabled() && isPreventCaptureEnabled()) {
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
                    }
                }

                @Override
                public void onActivityStarted(Activity activity) {}

                @Override
                public void onActivityResumed(Activity activity) {}

                @Override
                public void onActivityPaused(Activity activity) {}

                @Override
                public void onActivityStopped(Activity activity) {}

                @Override
                public void onActivitySaveInstanceState(Activity activity, Bundle b) {}

                @Override
                public void onActivityDestroyed(Activity activity) {
                    if (mCoordinatorActivity != null && activity == mCoordinatorActivity.get()) {
                        hideCoordinatorIfShowing(DialogDismissalCause.ACTIVITY_DESTROYED);
                    }
                }
            };

    private final ApplicationStatus.ApplicationStateListener mAppStateListener =
            newState -> {
                if (newState == ApplicationState.HAS_STOPPED_ACTIVITIES
                        || newState == ApplicationState.HAS_DESTROYED_ACTIVITIES) {
                    mLockArmed = isBrowserLockEnabled();
                    applySecureFlagToAllActivities();
                }
            };

    private final SharedPreferences.OnSharedPreferenceChangeListener mPrefChangeListener =
            (sharedPreferences, key) -> {
                if (BravePreferenceKeys.BRAVE_BROWSER_LOCK.equals(key)
                        || BravePreferenceKeys.BRAVE_BROWSER_LOCK_PRIVATE_TABS_ONLY.equals(key)
                        || BravePreferenceKeys.BRAVE_BROWSER_LOCK_PREVENT_CAPTURE.equals(key)) {
                    applySecureFlagToAllActivities();
                    if (!isBrowserLockEnabled()) {
                        removeAllPreNativeOverlays();
                    }
                }
            };

    private final IncognitoReauthManager.IncognitoReauthCallback mReauthCallback =
            new IncognitoReauthManager.IncognitoReauthCallback() {
                @Override
                public void onIncognitoReauthNotPossible() {
                    mLockArmed = false;
                    hideCoordinatorIfShowing(DialogDismissalCause.ACTION_ON_DIALOG_NOT_POSSIBLE);
                }

                @Override
                public void onIncognitoReauthSuccess() {
                    mLockArmed = false;
                    hideCoordinatorIfShowing(DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
                }

                @Override
                public void onIncognitoReauthFailure() {}
            };

    public static void initialize(Application application) {
        if (sInstance != null) {
            return;
        }
        sInstance = new BraveBrowserLockManager();
        sInstance.mLockArmed =
                ContextUtils.getAppSharedPreferences()
                        .getBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
        application.registerActivityLifecycleCallbacks(sInstance.mAppLifecycleCallbacks);
        ApplicationStatus.registerStateListenerForAllActivities(sInstance);
        ApplicationStatus.registerApplicationStateListener(sInstance.mAppStateListener);
        ContextUtils.getAppSharedPreferences()
                .registerOnSharedPreferenceChangeListener(sInstance.mPrefChangeListener);
        ProfileManager.addObserver(sInstance.mProfileObserver);
    }

    public static @Nullable BraveBrowserLockManager getInstance() {
        return sInstance;
    }

    BraveBrowserLockManager() {}

    public void onNativeInitialized(Profile profile) {
        mProfile = profile;
        applySecureFlagToAllActivities();
        removeAllPreNativeOverlays();

        if (!mNativeInitializedOnce) {
            mNativeInitializedOnce = true;
            mLockArmed = isBrowserLockEnabled();
        }

        if (mLockArmed) {
            for (Activity activity : ApplicationStatus.getRunningActivities()) {
                int state = ApplicationStatus.getStateForActivity(activity);
                if (state == ActivityState.STARTED || state == ActivityState.RESUMED) {
                    showLockIfRequired(activity);
                    break;
                }
            }
        }
    }

    @Override
    public void onActivityStateChange(Activity activity, @ActivityState int newState) {
        if (newState == ActivityState.STARTED) {
            if (mProfile == null) {
                showPreNativeOverlayIfRequired(activity);
            } else {
                showLockIfRequired(activity);
            }
        }
    }

    public static boolean isBrowserLockEnabled() {
        return IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    public static boolean isPreventCaptureEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK_PREVENT_CAPTURE, true);
    }

    private void applySecureFlagToAllActivities() {
        boolean secure = isBrowserLockEnabled() && isPreventCaptureEnabled();
        for (Activity activity : ApplicationStatus.getRunningActivities()) {
            if (secure) {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
            } else {
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
            }
        }
    }

    private void showLockIfRequired(Activity activity) {
        Profile profile = mProfile;
        if (!mLockArmed || !isBrowserLockEnabled() || mCoordinator != null || profile == null) {
            return;
        }
        mIncognitoReauthManager = new IncognitoReauthManager(activity, profile);
        mCoordinator = createCoordinator(activity, mIncognitoReauthManager);
        mCoordinatorActivity = new WeakReference<>(activity);
        mCoordinator.show();
        mIncognitoReauthManager.startReauthenticationFlow(mReauthCallback);
    }

    private void hideCoordinatorIfShowing(@DialogDismissalCause int cause) {
        if (mCoordinator != null) {
            mCoordinator.hide(cause);
            mCoordinator = null;
            mCoordinatorActivity = null;
        }
        if (mIncognitoReauthManager != null) {
            mIncognitoReauthManager.destroy();
            mIncognitoReauthManager = null;
        }
    }

    private void showPreNativeOverlayIfRequired(Activity activity) {
        if (!mLockArmed) return;
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        if (decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG) != null) return;
        View overlay = new View(activity);
        overlay.setTag(PRE_NATIVE_OVERLAY_TAG);
        overlay.setBackgroundColor(Color.BLACK);
        decor.addView(
                overlay,
                new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private void removeAllPreNativeOverlays() {
        for (Activity activity : ApplicationStatus.getRunningActivities()) {
            ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
            View overlay = decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG);
            if (overlay != null) {
                decor.removeView(overlay);
            }
        }
    }

    @VisibleForTesting
    BraveBrowserLockCoordinator createCoordinator(
            Activity activity, IncognitoReauthManager incognitoReauthManager) {
        return new BraveBrowserLockCoordinator(activity, incognitoReauthManager, mReauthCallback);
    }

    @VisibleForTesting
    boolean isLockArmedForTesting() {
        return mLockArmed;
    }

    @VisibleForTesting
    void setLockArmedForTesting(boolean armed) {
        mLockArmed = armed;
    }

    @VisibleForTesting
    void setNativeInitializedOnceForTesting(boolean value) {
        mNativeInitializedOnce = value;
    }

    @VisibleForTesting
    IncognitoReauthManager.IncognitoReauthCallback getReauthCallbackForTesting() {
        return mReauthCallback;
    }

    @VisibleForTesting
    boolean isPreNativeOverlayShownForTesting(Activity activity) {
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        return decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG) != null;
    }
}
