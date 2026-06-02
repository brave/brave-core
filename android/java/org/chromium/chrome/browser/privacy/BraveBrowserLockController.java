/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import android.app.Activity;
import android.content.SharedPreferences;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.modaldialog.DialogDismissalCause;

import java.util.function.Consumer;

/** Controls the browser-level biometrics lock, showing a lock screen when the app is resumed. */
@NullMarked
// Chromium's wrapper doesn't give us a way to register a listener for changes.
@SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
public class BraveBrowserLockController
        implements ApplicationStatus.TaskVisibilityListener,
                ApplicationStatus.ActivityStateListener {
    private final Activity mActivity;
    private final Profile mProfile;
    private final int mTaskId;

    private boolean mBrowserLockPending;
    private @Nullable BraveBrowserLockCoordinator mCoordinator;

    private final SharedPreferences.OnSharedPreferenceChangeListener mPrefChangeListener =
            (sharedPreferences, key) -> {
                if (BravePreferenceKeys.BRAVE_BROWSER_LOCK.equals(key)) {
                    updateSecureFlagForTask();
                }
            };

    private final IncognitoReauthManager.IncognitoReauthCallback mReauthCallback =
            new IncognitoReauthManager.IncognitoReauthCallback() {
                @Override
                public void onIncognitoReauthNotPossible() {
                    // Keep showing — user can re-add a device lock and retry.
                }

                @Override
                public void onIncognitoReauthSuccess() {
                    mBrowserLockPending = false;
                    hideCoordinatorIfShowing(DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
                }

                @Override
                public void onIncognitoReauthFailure() {
                    // User cancelled or failed — keep showing the lock screen.
                }
            };

    public BraveBrowserLockController(
            Activity activity,
            Profile profile,
            int taskId,
            OneshotSupplier<IncognitoReauthController> incognitoReauthControllerSupplier) {
        mActivity = activity;
        mProfile = profile;
        mTaskId = taskId;
        mBrowserLockPending = isBrowserLockEnabled();

        ApplicationStatus.registerTaskVisibilityListener(this);
        ApplicationStatus.registerStateListenerForAllActivities(this);
        ContextUtils.getAppSharedPreferences()
                .registerOnSharedPreferenceChangeListener(mPrefChangeListener);

        updateSecureFlagForTask();

        // onActivityStateChange fires only for future transitions. If the tabbed activity is
        // already started (native initialized before our constructor ran), show the lock now.
        int state = ApplicationStatus.getStateForActivity(mActivity);
        if (state == ActivityState.STARTED || state == ActivityState.RESUMED) {
            showLockIfRequired(mActivity);
        }
    }

    @VisibleForTesting
    boolean isLockPendingForTesting() {
        return mBrowserLockPending;
    }

    @VisibleForTesting
    IncognitoReauthManager.IncognitoReauthCallback getReauthCallbackForTesting() {
        return mReauthCallback;
    }

    public void destroy() {
        ContextUtils.getAppSharedPreferences()
                .unregisterOnSharedPreferenceChangeListener(mPrefChangeListener);
        ApplicationStatus.unregisterActivityStateListener(this);
        ApplicationStatus.unregisterTaskVisibilityListener(this);
        hideCoordinatorIfShowing(DialogDismissalCause.ACTIVITY_DESTROYED);
    }

    // Returns whether the browser lock setting is currently enabled and enforceable.
    public static boolean isBrowserLockEnabled() {
        return IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    @Override
    public void onActivityStateChange(Activity activity, @ActivityState int newState) {
        if (activity.getTaskId() != mTaskId) return;
        if (newState == ActivityState.CREATED && isBrowserLockEnabled()) {
            activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
        } else if (newState == ActivityState.STARTED) {
            showLockIfRequired(activity);
        }
    }

    @Override
    public void onTaskVisibilityChanged(int taskId, boolean isVisible) {
        if (!isVisible && mTaskId == taskId) {
            mBrowserLockPending = isBrowserLockEnabled();
            updateSecureFlagForTask();
        }
    }

    private void updateSecureFlagForTask() {
        Consumer<Window> func =
                isBrowserLockEnabled()
                        ? w -> w.addFlags(WindowManager.LayoutParams.FLAG_SECURE)
                        : w -> w.clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        for (Activity activity : ApplicationStatus.getRunningActivities()) {
            if (activity.getTaskId() != mTaskId) continue;
            func.accept(activity.getWindow());
        }
    }

    private void showLockIfRequired(Activity activity) {
        if (!mBrowserLockPending || !isBrowserLockEnabled() || mCoordinator != null) {
            return;
        }
        mCoordinator = createCoordinator(activity);
        mCoordinator.show();
    }

    @VisibleForTesting
    BraveBrowserLockCoordinator createCoordinator(Activity activity) {
        return new BraveBrowserLockCoordinator(activity, mProfile, mReauthCallback);
    }

    private void hideCoordinatorIfShowing(@DialogDismissalCause int cause) {
        if (mCoordinator != null) {
            mCoordinator.hide(cause);
            mCoordinator = null;
        }
    }
}
