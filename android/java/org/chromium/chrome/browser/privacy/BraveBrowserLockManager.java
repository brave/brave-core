/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import android.app.Activity;
import android.app.Application;
import android.content.SharedPreferences;
import android.os.Bundle;
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
import org.chromium.ui.modaldialog.DialogDismissalCause;

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
 *   <li><b>Phase 2 (native ready)</b> — {@link #onNativeInitialized} is called from {@link
 *       BraveTabbedRootUiCoordinator#onFinishNativeInitialization()}, supplying the {@link Profile}
 *       needed to show the biometric prompt.
 * </ol>
 *
 * <p>The overlay is shown unconditionally whenever the lock is armed. Because it is attached to the
 * window's decor view, in-app navigation (e.g. back-pressing out of the incognito reauth dialog to
 * regular tabs) moves content around beneath the overlay but never dismisses it — there is no
 * escape path without authenticating.
 */
@NullMarked
// Chromium's wrapper doesn't give us a way to register a listener for changes.
@SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
public class BraveBrowserLockManager implements ApplicationStatus.ActivityStateListener {
    private static @Nullable BraveBrowserLockManager sInstance;

    private @Nullable Profile mProfile;

    private boolean mNativeInitializedOnce;
    private boolean mLockArmed;
    private @Nullable BraveBrowserLockCoordinator mCoordinator;
    private @Nullable IncognitoReauthManager mIncognitoReauthManager;

    private final Application.ActivityLifecycleCallbacks mAppLifecycleCallbacks =
            new Application.ActivityLifecycleCallbacks() {
                @Override
                public void onActivityCreated(
                        Activity activity, @Nullable Bundle savedInstanceState) {
                    if (isBrowserLockEnabled()) {
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
                public void onActivityDestroyed(Activity activity) {}
            };

    private final ApplicationStatus.ApplicationStateListener mAppStateListener =
            newState -> {
                if (newState == ApplicationState.HAS_STOPPED_ACTIVITIES) {
                    mLockArmed = isBrowserLockEnabled();
                    applySecureFlagToAllActivities();
                }
            };

    private final SharedPreferences.OnSharedPreferenceChangeListener mPrefChangeListener =
            (sharedPreferences, key) -> {
                if (BravePreferenceKeys.BRAVE_BROWSER_LOCK.equals(key)
                        || BravePreferenceKeys.BRAVE_BROWSER_LOCK_PRIVATE_TABS_ONLY.equals(key)) {
                    applySecureFlagToAllActivities();
                }
            };

    private final IncognitoReauthManager.IncognitoReauthCallback mReauthCallback =
            new IncognitoReauthManager.IncognitoReauthCallback() {
                @Override
                public void onIncognitoReauthNotPossible() {}

                @Override
                public void onIncognitoReauthSuccess() {
                    mLockArmed = false;
                    hideCoordinatorIfShowing(DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
                }

                @Override
                public void onIncognitoReauthFailure() {}
            };

    public static void initialize(Application application) {
        assert sInstance == null : "BraveBrowserLockManager already initialized";
        sInstance = new BraveBrowserLockManager();
        application.registerActivityLifecycleCallbacks(sInstance.mAppLifecycleCallbacks);
        ApplicationStatus.registerStateListenerForAllActivities(sInstance);
        ApplicationStatus.registerApplicationStateListener(sInstance.mAppStateListener);
        ContextUtils.getAppSharedPreferences()
                .registerOnSharedPreferenceChangeListener(sInstance.mPrefChangeListener);
    }

    public static @Nullable BraveBrowserLockManager getInstance() {
        return sInstance;
    }

    BraveBrowserLockManager() {}

    public void onNativeInitialized(Profile profile) {
        mProfile = profile;
        applySecureFlagToAllActivities();

        if (!mNativeInitializedOnce) {
            mNativeInitializedOnce = true;
            if (isBrowserLockEnabled()) {
                mLockArmed = true;
            }
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
            showLockIfRequired(activity);
        }
    }

    public static boolean isBrowserLockEnabled() {
        return IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    private void applySecureFlagToAllActivities() {
        boolean secure = isBrowserLockEnabled();
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
        mCoordinator.show();
        mIncognitoReauthManager.startReauthenticationFlow(mReauthCallback);
    }

    private void hideCoordinatorIfShowing(@DialogDismissalCause int cause) {
        if (mCoordinator != null) {
            mCoordinator.hide(cause);
            mCoordinator = null;
        }
        if (mIncognitoReauthManager != null) {
            mIncognitoReauthManager.destroy();
            mIncognitoReauthManager = null;
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
}
