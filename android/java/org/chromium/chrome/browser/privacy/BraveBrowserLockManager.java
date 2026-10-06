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

import androidx.activity.ComponentActivity;
import androidx.activity.OnBackPressedCallback;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveExternalActivityLaunchTracker;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.FeatureList;
import org.chromium.base.ResettersForTesting;
import org.chromium.base.ThreadUtils;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.BraveIncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManagerHolder;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

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
 *
 * <p><b>Multi-instance:</b> {@code ChromeTabbedActivity} supports running as multiple concurrent
 * instances (e.g. desktop-Android multi-window, or {@code FLAG_ACTIVITY_MULTIPLE_TASK}). A lock
 * must therefore be tracked per-activity rather than as a single manager-wide slot.
 *
 * <p><b>Accessibility:</b> FLAG_SECURE only blocks screenshots/screen recording — it does nothing
 * to stop an accessibility service (e.g. TalkBack, or a malicious app abusing the Accessibility
 * API) from reading the view hierarchy underneath the lock overlay. Whenever a lock or pre-native
 * overlay is shown for an activity, that activity's content view is excluded from the accessibility
 * tree ({@link View#IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS}) so only the lock screen
 * itself is reachable; it is restored once the lock is dismissed.
 *
 * <p><b>Input:</b> A visual overlay alone does not stop input from reaching the content behind it.
 * Touches are consumed by the overlay views themselves (clickable, with the real lock view's own
 * touch listener consuming everything). Keyboard/D-pad focus is pulled onto the overlay and the
 * content view's descendants are blocked from being focused at all ({@link
 * ViewGroup#FOCUS_BLOCK_DESCENDANTS}) for as long as a lock is showing, so focus can't be returned
 * to (or kept on) a background view. The system Back button is independently blocked via an {@link
 * androidx.activity.OnBackPressedCallback} — without it, Back would navigate the content behind the
 * overlay (e.g. closing a tab) without ever authenticating.
 */
@NullMarked
// Chromium's wrapper doesn't give us a way to register a listener for changes.
@SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
public class BraveBrowserLockManager implements ApplicationStatus.ActivityStateListener {
    private static final Object PRE_NATIVE_OVERLAY_TAG = new Object();

    // How recently BraveExternalActivityLaunchTracker's timestamp must have been set for a
    // return to Brave to be treated as "a quick round trip through something we launched"
    // rather than a real backgrounding. Deliberately generous — covers slower file-picker
    // browsing, not just an instant share-sheet dismiss — since exceeding it simply means a
    // normal re-arm happens, matching what would have happened anyway without this mechanism.
    private static final long MAX_EXTERNAL_LAUNCH_REARM_SUPPRESSION_MS = 90_000;

    private static @Nullable BraveBrowserLockManager sInstance;

    /**
     * Notified whenever the screenshot-protection mode changes, so a live per-activity incognito
     * screenshot controller (e.g. {@link
     * org.chromium.chrome.browser.incognito.BraveIncognitoTabbedSnapshotController}) can recompute
     * immediately instead of waiting for its own unrelated trigger (a tab-model switch or Hub
     * show/hide for the tabbed case; nothing at all, post-construction, for Custom Tabs).
     */
    public interface ScreenshotModeObserver {
        void onScreenshotModeChanged();
    }

    private @Nullable Profile mProfile;

    private boolean mNativeInitializedOnce;
    private boolean mLockArmed;

    private static class ActiveLock {
        final BraveBrowserLockCoordinator mCoordinator;
        final IncognitoReauthManager mReauthManager;
        boolean mReauthStarted;

        ActiveLock(BraveBrowserLockCoordinator coordinator, IncognitoReauthManager reauthManager) {
            mCoordinator = coordinator;
            mReauthManager = reauthManager;
        }
    }

    private final Map<Activity, ActiveLock> mActiveLocks = new HashMap<>();

    // Activities Brave itself added FLAG_SECURE to (via EVERYTHING mode). Tracked so clearing it
    // later only undoes what Brave set, never a flag an upstream per-activity incognito-aware
    // controller (e.g. BraveIncognitoTabbedSnapshotController) is independently managing.
    private final Set<Activity> mForcedSecureActivities = new HashSet<>();

    // At most one per activity — ChromeTabbedActivity and CustomTabActivity each wire up their
    // own single incognito screenshot controller. Removed in onActivityDestroyed so this never
    // leaks a reference to a destroyed activity's controller.
    private final Map<Activity, ScreenshotModeObserver> mScreenshotModeObservers = new HashMap<>();

    // Lazily created per activity the first time a lock (pre-native or real) needs to block the
    // system Back button; reused (via setEnabled) thereafter. Removed in onActivityDestroyed.
    private final Map<Activity, OnBackPressedCallback> mBackPressBlockers = new HashMap<>();

    private boolean mReauthInFlight;

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
                    if (shouldForceSecureWindow()) {
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
                        mForcedSecureActivities.add(activity);
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
                    mForcedSecureActivities.remove(activity);
                    mScreenshotModeObservers.remove(activity);
                    OnBackPressedCallback backPressBlocker = mBackPressBlockers.remove(activity);
                    if (backPressBlocker != null) backPressBlocker.remove();
                    ActiveLock lock = mActiveLocks.remove(activity);
                    if (lock == null) return;

                    lock.mCoordinator.hide(DialogDismissalCause.ACTIVITY_DESTROYED);
                    IncognitoReauthManager reauthManager = lock.mReauthManager;
                    // Must defer to ensure we don't destroy within the object's own usage scope.
                    ThreadUtils.postOnUiThread(reauthManager::destroy);
                    if (lock.mReauthStarted) {
                        mReauthInFlight = false;
                        maybeStartNextReauth();
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
                        || BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE.equals(key)) {
                    applySecureFlagToAllActivities();
                    if (!isBrowserLockEnabled()) {
                        mLockArmed = false;
                        mReauthInFlight = false;
                        removeAllPreNativeOverlays();
                        // Also dismiss any lock already showing on another activity (e.g. a
                        // second multi-instance window) — the pref may have been disabled from
                        // elsewhere while that lock is still up.
                        hideAllCoordinators(DialogDismissalCause.ACTION_ON_DIALOG_COMPLETED);
                    }
                }
            };

    private final IncognitoReauthManager.IncognitoReauthCallback mReauthCallback =
            new IncognitoReauthManager.IncognitoReauthCallback() {
                @Override
                public void onIncognitoReauthNotPossible() {
                    mReauthInFlight = false;
                    mLockArmed = false;
                    hideAllCoordinators(DialogDismissalCause.ACTION_ON_DIALOG_NOT_POSSIBLE);
                }

                @Override
                public void onIncognitoReauthSuccess() {
                    mReauthInFlight = false;
                    mLockArmed = false;
                    hideAllCoordinators(DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
                }

                @Override
                public void onIncognitoReauthFailure() {
                    mReauthInFlight = false;
                    maybeStartNextReauth();
                }
            };

    @VisibleForTesting
    public static void setInstanceForTesting(@Nullable BraveBrowserLockManager instance) {
        BraveBrowserLockManager previous = sInstance;
        sInstance = instance;
        ResettersForTesting.register(() -> sInstance = previous);
    }

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
                }
            }
        }
    }

    @Override
    public void onActivityStateChange(Activity activity, @ActivityState int newState) {
        if (newState == ActivityState.STARTED) {
            if (mLockArmed && isReturningFromRecentExternalLaunch()) {
                // We background whenever any external activity takes over — a file picker, a
                // share sheet, a link that opens another app — exactly like a genuine
                // backgrounding, since there is no way to tell those apart at the moment they
                // take over. This only runs once we're back, when it's finally knowable whether
                // the round trip was quick (not a meaningful absence from Brave) or not (treated
                // as a real backgrounding — see BraveExternalActivityLaunchTracker).
                mLockArmed = false;
            }
            if (mProfile == null) {
                showPreNativeOverlayIfRequired(activity);
            } else {
                showLockIfRequired(activity);
            }
        }
    }

    /**
     * Whether this {@code STARTED} transition looks like a quick return from an external activity
     * Brave itself launched (via {@link
     * BraveExternalActivityLaunchTracker#notifyLaunchingExternalActivity()}), rather than a
     * meaningful absence from the app — regardless of whether the user was actively using that
     * external activity the whole time or wandered off elsewhere before coming back; either way,
     * exceeding the bound here means a real re-arm is warranted.
     */
    private static boolean isReturningFromRecentExternalLaunch() {
        long elapsedMs = BraveExternalActivityLaunchTracker.consumeElapsedMsSinceLastLaunch();
        return elapsedMs >= 0 && elapsedMs < MAX_EXTERNAL_LAUNCH_REARM_SUPPRESSION_MS;
    }

    public static boolean isBrowserLockEnabled() {
        return IncognitoReauthManager.isIncognitoReauthFeatureAvailable()
                && IncognitoReauthSettingUtils.isDeviceScreenLockEnabled()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    /**
     * Returns the current screenshot-protection mode: {@link
     * BravePreferenceKeys#BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW}, {@link
     * BravePreferenceKeys#BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY}, or {@link
     * BravePreferenceKeys#BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING}.
     *
     * <p>The first time this is read (pref key absent — i.e. an upgrade from before this tri-state
     * setting existed, or a fresh install), the value is migrated from the pre-existing {@link
     * ChromeFeatureList#sIncognitoScreenshot} feature so an upgrading user's prior choice carries
     * over: feature enabled ("allow incognito screenshots") maps to {@code ALLOW}; disabled (the
     * upstream default, and what every fresh install already has) maps to {@code
     * PRIVATE_TABS_ONLY}. {@code EVERYTHING} has no pre-existing equivalent, so it is never
     * auto-selected — it is purely opt-in. The migrated value is only persisted once native is
     * initialized, since the feature flag's Java wrapper cannot report the real value before then;
     * otherwise a pre-native caller would permanently lock in an unreliable guess.
     */
    public static int getScreenshotMode() {
        SharedPreferencesManager prefs = ChromeSharedPreferences.getInstance();
        if (prefs.contains(BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE)) {
            return prefs.readInt(BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE);
        }
        int migrated =
                ChromeFeatureList.sIncognitoScreenshot.isEnabled()
                        ? BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW
                        : BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY;
        if (FeatureList.isNativeInitialized()) {
            prefs.writeInt(BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE, migrated);
        }
        return migrated;
    }

    public static void setScreenshotMode(int mode) {
        ChromeSharedPreferences.getInstance()
                .writeInt(BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE, mode);
        BraveBrowserLockManager instance = sInstance;
        if (instance == null) return;
        for (ScreenshotModeObserver observer : instance.mScreenshotModeObservers.values()) {
            observer.onScreenshotModeChanged();
        }
    }

    /**
     * Registers {@code observer} to be notified of future {@link #setScreenshotMode} calls, until
     * {@code activity} is destroyed (at which point it is automatically unregistered — callers do
     * not need their own unregister path).
     */
    public static void addScreenshotModeObserver(
            Activity activity, ScreenshotModeObserver observer) {
        BraveBrowserLockManager instance = sInstance;
        if (instance == null) return;
        instance.mScreenshotModeObservers.put(activity, observer);
    }

    /**
     * Returns whether Brave should force FLAG_SECURE on regardless of what tab/layout state
     * upstream's own incognito-scoped screenshot protection (e.g. {@link
     * org.chromium.chrome.browser.incognito.IncognitoSnapshotController}) would otherwise decide.
     * Called from Brave subclasses of those upstream controllers so they can defer to this decision
     * instead of unconditionally clearing the flag whenever no incognito tab is showing.
     *
     * <p>Deliberately independent of {@link #isBrowserLockEnabled()} — changing this setting
     * already requires authentication, so there is no reason to also require a lock to be enabled
     * first. A user may want screenshot/capture protection for the whole browser without wanting
     * the biometric lock screen at all.
     */
    public static boolean shouldForceSecureWindow() {
        return getScreenshotMode()
                == BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING;
    }

    /**
     * Returns whether FLAG_SECURE should be set for a window given the current screenshot mode and
     * whether an incognito tab is currently showing in it.
     *
     * <p>Used by Brave's subclasses of upstream's per-activity incognito screenshot controllers
     * (e.g. {@link org.chromium.chrome.browser.incognito.BraveIncognitoTabbedSnapshotController})
     * so the decision is driven directly by {@link #getScreenshotMode()} — instantly, via the pref
     * — instead of upstream's own logic, which depends on {@link
     * org.chromium.chrome.browser.flags.ChromeFeatureList#sIncognitoScreenshot}. That flag is only
     * read from a cache populated at native-init time, so it stays stale (reflecting whatever mode
     * was active at the last relaunch) until the user restarts the app — meaning PRIVATE_TABS_ONLY
     * protection would otherwise silently not apply to a just-selected mode until relaunch.
     */
    public static boolean shouldSecureForIncognitoVisibility(boolean isShowingIncognito) {
        int mode = getScreenshotMode();
        if (mode == BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING) {
            return true;
        }
        if (mode == BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW) {
            return false;
        }
        return isShowingIncognito;
    }

    private void applySecureFlagToAllActivities() {
        boolean secure = shouldForceSecureWindow();
        for (Activity activity : ApplicationStatus.getRunningActivities()) {
            if (secure) {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
                mForcedSecureActivities.add(activity);
            } else if (mForcedSecureActivities.remove(activity)) {
                // Only clear a flag Brave itself set here or in onActivityCreated — never one an
                // upstream per-activity incognito-aware controller is independently managing
                // (e.g. for a currently-showing incognito tab under PRIVATE_TABS_ONLY).
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
            }
        }
    }

    private void showLockIfRequired(Activity activity) {
        Profile profile = mProfile;
        if (!mLockArmed
                || !isBrowserLockEnabled()
                || mActiveLocks.containsKey(activity)
                || profile == null) {
            return;
        }
        IncognitoReauthManager reauthManager = new BraveIncognitoReauthManager(activity, profile);
        BraveBrowserLockCoordinator coordinator = createCoordinator(activity, reauthManager);
        mActiveLocks.put(activity, new ActiveLock(coordinator, reauthManager));
        dismissModalDialogsIfPresent(activity);
        coordinator.show();
        setContentLocked(activity, true);
        setBackPressBlocked(activity, true);
        maybeStartNextReauth();
    }

    /**
     * Dismisses any dialog tracked by {@code activity}'s own {@link ModalDialogManager}, if it has
     * one. A dialog lives in a separate window the lock overlay (attached to the decor view) can't
     * visually or input-wise cover — if one was already showing when the app backgrounds, it would
     * otherwise reappear above the lock on resume. This only reaches dialogs routed through
     * Chromium's own dialog system; a raw platform {@code PopupWindow} or system-level dialog (e.g.
     * an autofill save prompt) is outside its reach.
     */
    private void dismissModalDialogsIfPresent(Activity activity) {
        if (activity instanceof ModalDialogManagerHolder holder) {
            holder.getModalDialogManager().dismissAllDialogs(DialogDismissalCause.UNKNOWN);
        }
    }

    private void maybeStartNextReauth() {
        if (mReauthInFlight) return;
        for (var entry : mActiveLocks.entrySet()) {
            ActiveLock lock = entry.getValue();
            if (!lock.mReauthStarted) {
                lock.mReauthStarted = true;
                mReauthInFlight = true;
                lock.mReauthManager.startReauthenticationFlow(mReauthCallback);
                return;
            }
        }
    }

    private void hideAllCoordinators(@DialogDismissalCause int cause) {
        for (var entry : mActiveLocks.entrySet()) {
            ActiveLock lock = entry.getValue();
            lock.mCoordinator.hide(cause);
            IncognitoReauthManager reauthManager = lock.mReauthManager;
            // Must defer since we could be called from within the scope of a living object.
            ThreadUtils.postOnUiThread(reauthManager::destroy);
            setContentLocked(entry.getKey(), false);
            setBackPressBlocked(entry.getKey(), false);
        }
        mActiveLocks.clear();
    }

    private void showPreNativeOverlayIfRequired(Activity activity) {
        if (!mLockArmed) return;
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        if (decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG) != null) return;
        dismissModalDialogsIfPresent(activity);
        View overlay = new View(activity);
        overlay.setTag(PRE_NATIVE_OVERLAY_TAG);
        overlay.setBackgroundColor(Color.BLACK);
        // Clickable so touches are consumed here instead of falling through to the content
        // beneath; focusable so it can steal focus away from (and block it returning to) that
        // content, mirroring what BraveBrowserLockCoordinator's real lock view already does.
        overlay.setClickable(true);
        overlay.setFocusable(true);
        overlay.setFocusableInTouchMode(true);
        decor.addView(
                overlay,
                new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        overlay.requestFocus();
        setContentLocked(activity, true);
        setBackPressBlocked(activity, true);
    }

    private void removeAllPreNativeOverlays() {
        for (Activity activity : ApplicationStatus.getRunningActivities()) {
            ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
            View overlay = decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG);
            if (overlay != null) {
                decor.removeView(overlay);
                setContentLocked(activity, false);
                setBackPressBlocked(activity, false);
            }
        }
    }

    /**
     * Excludes (or restores) an activity's content view from the accessibility tree and from normal
     * keyboard/D-pad focus traversal. See the class-level "Accessibility" doc for why the
     * accessibility exclusion is needed alongside FLAG_SECURE; focus is blocked for the same
     * underlying reason touches are — so a hardware keyboard or D-pad can't reach (or keep) a view
     * already focused in the content behind the lock overlay.
     */
    private void setContentLocked(Activity activity, boolean locked) {
        View content = activity.findViewById(android.R.id.content);
        if (content == null) return;
        content.setImportantForAccessibility(
                locked
                        ? View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS
                        : View.IMPORTANT_FOR_ACCESSIBILITY_AUTO);
        ((ViewGroup) content)
                .setDescendantFocusability(
                        locked
                                ? ViewGroup.FOCUS_BLOCK_DESCENDANTS
                                : ViewGroup.FOCUS_BEFORE_DESCENDANTS);
    }

    /**
     * Blocks (or allows) the system Back button for {@code activity} while a lock (pre-native or
     * real) is showing — otherwise Back navigates the content behind the overlay (e.g. closing a
     * tab) without ever authenticating.
     */
    private void setBackPressBlocked(Activity activity, boolean blocked) {
        OnBackPressedCallback callback = mBackPressBlockers.get(activity);
        if (callback != null) {
            callback.setEnabled(blocked);
            return;
        }
        if (!blocked || !(activity instanceof ComponentActivity componentActivity)) return;
        callback =
                new OnBackPressedCallback(/* enabled= */ true) {
                    @Override
                    public void handleOnBackPressed() {}
                };
        componentActivity.getOnBackPressedDispatcher().addCallback(callback);
        mBackPressBlockers.put(activity, callback);
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
    SharedPreferences.OnSharedPreferenceChangeListener getPrefChangeListenerForTesting() {
        return mPrefChangeListener;
    }

    @VisibleForTesting
    Application.ActivityLifecycleCallbacks getAppLifecycleCallbacksForTesting() {
        return mAppLifecycleCallbacks;
    }

    @VisibleForTesting
    ApplicationStatus.ApplicationStateListener getAppStateListenerForTesting() {
        return mAppStateListener;
    }

    @VisibleForTesting
    boolean isPreNativeOverlayShownForTesting(Activity activity) {
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        return decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG) != null;
    }

    @VisibleForTesting
    @Nullable View getPreNativeOverlayForTesting(Activity activity) {
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        return decor.findViewWithTag(PRE_NATIVE_OVERLAY_TAG);
    }

    @VisibleForTesting
    boolean isLockShownForTesting(Activity activity) {
        return mActiveLocks.containsKey(activity);
    }

    @VisibleForTesting
    int getActiveLockCountForTesting() {
        return mActiveLocks.size();
    }

    @VisibleForTesting
    boolean isReauthInFlightForTesting() {
        return mReauthInFlight;
    }

    @VisibleForTesting
    boolean isForcedSecureForTesting(Activity activity) {
        return mForcedSecureActivities.contains(activity);
    }
}
