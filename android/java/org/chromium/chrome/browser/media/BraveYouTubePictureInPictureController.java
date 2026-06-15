/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.KeyguardManager;
import android.app.PictureInPictureParams;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.PowerManager;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.MediaSession;
import org.chromium.content_public.browser.MediaSessionObserver;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.media_session.mojom.MediaSession.SuspendType;

/**
 * Owns the lifecycle of a Brave-managed YouTube Picture-in-Picture session.
 *
 * <p>A session begins when either the manual YouTube PiP button or the upstream fullscreen-video
 * PiP path requests it via {@link #onSessionRequested}; it ends either when PiP is dismissed
 * normally, when the screen-lock interrupt sequence resolves, or when the entry attempt fails.
 */
@NullMarked
public class BraveYouTubePictureInPictureController {
    private static final String TAG = "BraveYTPiPController";

    @VisibleForTesting
    static final String KEY_ACTIVE = "org.chromium.chrome.browser.media.YT_PIP_ACTIVE";

    @VisibleForTesting
    static final String KEY_INTERRUPTED_BY_SCREEN_LOCK =
            "org.chromium.chrome.browser.media.YT_PIP_INTERRUPTED_BY_SCREEN_LOCK";

    @VisibleForTesting
    static final String KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY =
            "org.chromium.chrome.browser.media.YT_PIP_RESUME_MEDIA_ON_ENTRY";

    @VisibleForTesting
    static final String KEY_WAS_PLAYING_BEFORE_SCREEN_LOCK =
            "org.chromium.chrome.browser.media.YT_PIP_WAS_PLAYING_BEFORE_SCREEN_LOCK";

    @VisibleForTesting
    static final String KEY_TAB_ID = "org.chromium.chrome.browser.media.YT_PIP_TAB_ID";

    /**
     * Hard ceiling on how long we wait for YouTube's DOM to drop out of fullscreen after we ask it
     * to. The fast path is event driven: a one shot WebContentsObserver fires the cleanup as soon
     * as the renderer reports the fullscreen exit, which is typically much sooner than this
     * timeout. The ceiling exists only as a safety net for the case where the renderer never
     * reports the exit (broken page, JS failure, etc.).
     */
    @VisibleForTesting static final long FULLSCREEN_EXIT_FALLBACK_MS = 500L;

    /**
     * Time given to the user/system to confirm dismissal of the PiP window before we suspend the
     * media session. Long enough to cover an Android animation back to the activity, short enough
     * that a true dismiss feels responsive.
     */
    @VisibleForTesting static final long DISMISS_CHECK_DELAY_MS = 1000L;

    private final BraveActivity mActivity;

    private @Nullable WebContents mWebContents;
    private boolean mActive;
    private boolean mExiting;
    private boolean mInterruptedByScreenLock;
    private int mSessionId;
    private @Nullable BroadcastReceiver mScreenStateReceiver;

    /**
     * Snapshot of whether media was playing when the current screen-lock interruption began, taken
     * by {@link #markInterruptedByScreenLock} before any pause is issued. Consumed by {@link
     * #maybeResumeAfterScreenLock} so the PiP re-entry resume only restores playback that the lock
     * interrupted. (Mirrors the intent of upstream's {@code mIsSuspendedForStash} guard).
     */
    private boolean mWasPlayingBeforeScreenLock;

    /**
     * Whether the session's media is currently playing, kept up to date by {@link
     * #mMediaSessionObserver}. The Java MediaSession API has no state getter and observer callbacks
     * are delivered asynchronously, so the state must be tracked continuously while the session is
     * bound; it cannot be queried on demand at interrupt time.
     */
    private boolean mIsMediaPlaying;

    /**
     * Observes play/pause state changes of the media session bound to the session's WebContents.
     * Attached by {@link #startObservingMediaSession} whenever the session binds to a WebContents
     * (initial request or post-recreation recovery) and detached when the session ends. (Mirrors
     * upstream's {@code mIsPlaying} tracking in {@code FullscreenVideoPictureInPictureController}.)
     */
    private @Nullable MediaSessionObserver mMediaSessionObserver;

    /**
     * One shot observer wired up by {@link #handleSessionExited} to fire {@code
     * maybeRestoreFullscreenUi} as soon as the renderer reports DOM fullscreen has exited. Held
     * here only so we can detach it eagerly when the session ends or the activity is destroyed, so
     * the observer never outlives the controller.
     */
    private @Nullable WebContentsObserver mPendingFullscreenExitObserver;

    /**
     * Identifier of the tab whose WebContents owns the active session, or {@link
     * Tab#INVALID_TAB_ID} when no session is bound. Tab IDs are stable across activity recreation
     * (the TabModel outlives the Activity), so persisting this lets us recover the session's
     * WebContents after a config change without depending on the registry's WeakReference, which
     * can be cleared by GC.
     */
    private int mTabId = Tab.INVALID_TAB_ID;

    /**
     * True if a PiP entry should resume the media session once the system reports the activity is
     * in PiP mode. Set by {@link #onSessionRequested} and consumed by {@link
     * #onEnterPictureInPictureMode}.
     */
    private boolean mResumeMediaSessionOnPipEntry;

    public BraveYouTubePictureInPictureController(BraveActivity activity) {
        mActivity = activity;
    }

    /** Restore persisted state on activity creation. */
    public void onPostCreate(@Nullable Bundle savedInstanceState) {
        if (savedInstanceState == null) {
            return;
        }
        final boolean wasActive = savedInstanceState.getBoolean(KEY_ACTIVE, false);
        final boolean wasInterrupted =
                savedInstanceState.getBoolean(KEY_INTERRUPTED_BY_SCREEN_LOCK, false);
        // Validate the persisted snapshot against ground truth before adopting it: a restored
        // session is only real if the PiP window survived the recreation or the session is
        // parked behind a screen-lock interruption. Anything else (saved-instance bundles also
        // survive process death and PiP dismissals in the recreation gap) means no callback
        // will ever end the session, and a stale mActive would keep
        // isYouTubePictureInPictureActive() latched true for the whole activity lifetime,
        // suppressing upstream's fullscreen teardown browser-wide.
        if (!wasActive || (!wasInterrupted && !mActivity.isInPictureInPictureMode())) {
            return;
        }
        mActive = true;
        mInterruptedByScreenLock = wasInterrupted;
        mResumeMediaSessionOnPipEntry =
                savedInstanceState.getBoolean(KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY, false);
        mWasPlayingBeforeScreenLock =
                savedInstanceState.getBoolean(KEY_WAS_PLAYING_BEFORE_SCREEN_LOCK, false);
        mTabId = savedInstanceState.getInt(KEY_TAB_ID, Tab.INVALID_TAB_ID);
        // The screen state receiver is deliberately NOT registered here: onPostCreate runs
        // synchronously inside onCreate, before native and the tab models initialize, and a
        // broadcast delivered in that window would reach getOrFindWebContents() pre-init.
        // onResume() (called from onResumeWithNative) arms it.
    }

    /** Persist state ahead of activity recreation. */
    public void onSaveInstanceState(Bundle outState) {
        outState.putBoolean(KEY_ACTIVE, mActive);
        outState.putBoolean(KEY_INTERRUPTED_BY_SCREEN_LOCK, mInterruptedByScreenLock);
        outState.putBoolean(KEY_RESUME_MEDIA_SESSION_ON_PIP_ENTRY, mResumeMediaSessionOnPipEntry);
        outState.putBoolean(KEY_WAS_PLAYING_BEFORE_SCREEN_LOCK, mWasPlayingBeforeScreenLock);
        outState.putInt(KEY_TAB_ID, mTabId);
    }

    /** Hook from {@code Activity#onResumeWithNative}. */
    public void onResume() {
        // Arm the receiver for a session restored by onPostCreate (which cannot register it
        // pre-native). Harmless no-op when already registered.
        if (mActive) {
            registerScreenStateReceiver();
        }
        maybeResumeAfterScreenLock();
    }

    /** Hook from {@code Activity#onDestroy}. */
    public void onDestroy() {
        unregisterScreenStateReceiver();
        // On a configuration change the recreated activity restores the session from the
        // saved instance bundle, so leave the registry intact for it to pick up. Only clear
        // with a non-null WebContents to match against: clear(null) wipes the process-global
        // slot unconditionally, and when this controller never (re)bound a WebContents the slot
        // may belong to another window's live session (e.g. ours was lock-interrupted and a
        // second window started its own PiP). clear(non-null) is self-guarded and it only
        // clears a matching or dead entry.
        if (!mActivity.isChangingConfigurations() && mWebContents != null) {
            BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(mWebContents);
        }
        // Reset session state so any delayed callbacks queued by handleSessionExited() return
        // early via isExitingForSession() instead of touching a destroyed activity. State has
        // already been written to the saved-instance bundle, so the next activity restores from
        // the bundle rather than from these fields.
        resetSessionState();
    }

    /**
     * Hook from {@code onPictureInPictureModeChanged(true)}. Resets any prior interrupt state,
     * adopts a system-initiated PiP if this is a YouTube tab, and resumes the media session if a
     * pending request asked us to.
     *
     * @return true if the caller should run the PiP-bounds-update workaround. (Returned for
     *     symmetry with the previous BraveActivity behavior.)
     */
    public boolean onEnterPictureInPictureMode() {
        mExiting = false;
        mInterruptedByScreenLock = false;
        maybeAdoptSession();

        if (!mResumeMediaSessionOnPipEntry) {
            return false;
        }
        mResumeMediaSessionOnPipEntry = false;

        // Resume the session's own WebContents, recovered if needed. After a screen-lock
        // re-entry the foreground tab can differ from the session tab (a new-tab intent arrived
        // while locked, or the activity was recreated), and resuming the foreground tab would
        // start unrelated media while the video in the PiP window stays paused.
        final WebContents webContents = getOrFindWebContents();
        if (webContents != null) {
            final MediaSession mediaSession = getMediaSession(webContents);
            if (mediaSession != null) {
                mediaSession.resume(SuspendType.SYSTEM);
            }
        }
        return true;
    }

    /** Hook from {@code onPictureInPictureModeChanged(false)}. */
    public void onExitPictureInPictureMode() {
        if (!mActive) {
            return;
        }
        if (isScreenOffOrLocked()) {
            markInterruptedByScreenLock();
            return;
        }
        handleSessionExited();
    }

    /**
     * Marks that a Brave-managed YouTube PiP session is starting against {@code webContents}. The
     * caller is expected to invoke {@code Activity#enterPictureInPictureMode} immediately after.
     */
    public void onSessionRequested(final WebContents webContents) {
        mResumeMediaSessionOnPipEntry = true;
        mWebContents = webContents;
        // Capture the owning tab id so we can recover the WebContents after activity recreation
        // even if the registry's WeakReference has been cleared. Both callers (the YouTube PiP
        // entry path and maybeAdoptSession) pass the current activity tab's WebContents, so
        // resolving via the current tab is correct; the equality guard is defensive in case that
        // ever changes; without it, a mismatched binding would silently retarget recovery at
        // the wrong tab.
        final TabModelSelector tabModelSelector = getTabModelSelectorIfReady();
        final Tab currentTab = tabModelSelector != null ? tabModelSelector.getCurrentTab() : null;
        mTabId =
                currentTab != null && currentTab.getWebContents() == webContents
                        ? currentTab.getId()
                        : Tab.INVALID_TAB_ID;
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(webContents);
        startObservingMediaSession(webContents);
        mActive = true;
        mExiting = false;
        mInterruptedByScreenLock = false;
        mSessionId++;
        // Arm the screen-state receiver for the whole session: SCREEN_OFF is the only reliable
        // lock signal when the PiP window survives the lock (see registerScreenStateReceiver).
        registerScreenStateReceiver();
    }

    /** The corresponding PiP entry attempt failed. Abandon the session. */
    public void onSessionEnterFailed() {
        mResumeMediaSessionOnPipEntry = false;
        clearSession(mSessionId, mWebContents);
    }

    public boolean isActive() {
        return mActive;
    }

    /**
     * Signaled by the upstream PiP exit observers when YouTube reports leaving fullscreen while the
     * device looks asleep. Treat as a transient interruption.
     */
    public void onFullscreenInterrupted() {
        if (mActive) {
            markInterruptedByScreenLock();
        }
    }

    /**
     * Called when a new tab arrives during an active YouTube PiP session (typically via a launcher
     * shortcut). Exit persistent fullscreen UI synchronously and release the YouTube WebContents
     * from fullscreen state so it is in a normal layout when the user switches back.
     */
    public void onNewTabDuringPictureInPicture() {
        if (!mActive) {
            return;
        }

        // Mirror onExitPictureInPictureMode: defer teardown while the device is locked and let
        // the screen-state receiver pick the session back up on unlock.
        if (isScreenOffOrLocked()) {
            markInterruptedByScreenLock();
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents != null && !webContents.isDestroyed()) {
            // Pause YouTube playback so audio does not continue while the user is on the new
            // tab. Mirrors maybeSuspendDismissed() but without the visibility check, since a
            // new-tab event is an explicit "user moved on" signal.
            suspendMediaSession(webContents);
            // Tell the YouTube WebContents to drop fullscreen directly. We do not rely on
            // FullscreenManager.exitPersistentFullscreenMode() to forward this: by the
            // time the tab-change observer has fired and we get here, the manager's internal
            // mWebContentsInFullscreen pointer can already be stale, and the WebContents-level
            // exit it would otherwise emit silently no-ops. Calling exitFullscreen() directly on
            // the tracked YouTube WebContents ensures the renderer leaves fullscreen so the tab
            // is in a normal layout when the user switches back.
            webContents.exitFullscreen();
        }

        // Defer the layout-side exit. We are called from upstream
        // DismissActivityOnTabChangeObserver.onResult, which still runs registerTabEventObserver
        // and updateAutoPictureInPictureStatusIfNeeded after we return. Posting to UI_DEFAULT
        // lets the rest of the tab-change chain settle first.
        final int sessionId = mSessionId;
        final WebContents capturedWebContents = mWebContents;
        PostTask.postTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (!mActive
                            || sessionId != mSessionId
                            || capturedWebContents != mWebContents
                            || mActivity.isActivityFinishingOrDestroyed()) {
                        return;
                    }
                    final FullscreenManager fullscreenManager = mActivity.getFullscreenManager();
                    final TabModelSelector tabModelSelector = getTabModelSelectorIfReady();
                    final Tab activityTab =
                            tabModelSelector != null ? tabModelSelector.getCurrentTab() : null;
                    if (fullscreenManager.getPersistentFullscreenMode() && activityTab != null) {
                        // Canonical exit path: clears persistent-fullscreen layout state and
                        // fans out to FullscreenManager.Observer (InfoBarContainer,
                        // ContextualSearchManager, navbar coloring, etc.).
                        fullscreenManager.onExitFullscreen(activityTab);
                    }
                    final MonotonicObservableSupplier<BrowserControlsManager> bcmSupplier =
                            mActivity.getBrowserControlsManagerSupplier();
                    final BrowserControlsManager bcm =
                            bcmSupplier != null ? bcmSupplier.get() : null;
                    if (bcm != null) {
                        // Force the browser-controls offset back to 0. On tablets the toolbar can
                        // stay translated off-screen after the canonical exit and
                        // BrowserControlsManager.onConstraintsChanged doesn't always recover it.
                        // Writing the offsets directly is the most reliable way to bring it back.
                        bcm.showAndroidControls(false);
                    }
                    clearSession(sessionId, capturedWebContents);
                });
    }

    /**
     * Returns true if the device screen is off or the keyguard is locked. Shared by the controller
     * and {@link BraveFullscreenVideoPictureInPictureController}. Uses the application context
     * because the underlying signals (PowerManager interactivity, keyguard state) are
     * device-global, which also lets Robolectric tests drive them via shadows without an Activity.
     */
    public static boolean isScreenOffOrLocked() {
        Context context = ContextUtils.getApplicationContext();
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        if (powerManager != null && !powerManager.isInteractive()) {
            return true;
        }

        KeyguardManager keyguardManager =
                (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
        return keyguardManager != null && keyguardManager.isKeyguardLocked();
    }

    private void maybeAdoptSession() {
        if (mActive) {
            return;
        }

        final WebContents currentWebContents = mActivity.getCurrentWebContents();
        if (currentWebContents == null
                || currentWebContents.isDestroyed()
                || !BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        currentWebContents)) {
            return;
        }

        onSessionRequested(currentWebContents);
    }

    private void handleSessionExited() {
        if (mExiting) {
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents == null || webContents.isDestroyed()) {
            clearSession(mSessionId, mWebContents);
            return;
        }

        final int sessionId = mSessionId;
        mExiting = true;
        // Primary exit path: ask the renderer to drop DOM fullscreen directly. The page
        // receives the same fullscreen change event it gets when the user presses Back in a
        // fullscreen video, so YouTube's player restores its layout.
        webContents.exitFullscreen();

        // Drive the restore from the actual renderer signal. didToggleFullscreenModeForTab
        // fires shortly after the renderer acts on the exit above, which is much sooner than the
        // hard timeout below. mPendingFullscreenExitObserver doubles as the single shot sentinel:
        // both paths claim ownership by clearing the field, so whichever wins runs the cleanup
        // and the loser short circuits. resetSessionState clearing the field makes the loser
        // path a no op too if the session ends in between.
        final WebContentsObserver exitObserver =
                new WebContentsObserver(webContents) {
                    @Override
                    public void didToggleFullscreenModeForTab(
                            boolean enteredFullscreen, boolean willCauseResize) {
                        if (enteredFullscreen || mPendingFullscreenExitObserver != this) {
                            return;
                        }
                        mPendingFullscreenExitObserver = null;
                        observe(null);
                        maybeRestoreFullscreenUi(sessionId, webContents);
                    }
                };
        mPendingFullscreenExitObserver = exitObserver;
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (mPendingFullscreenExitObserver != exitObserver) {
                        return;
                    }
                    mPendingFullscreenExitObserver = null;
                    exitObserver.observe(null);
                    // The renderer never reported a fullscreen exit. The benign causes are a page
                    // that had already left DOM fullscreen before the exit request (no transition
                    // to report) or a renderer that is slow/hung; in all of them the browser-side
                    // restore below is the only part we can and need to drive, and the page heals
                    // its own layout off the late fullscreenchange, if any.
                    maybeRestoreFullscreenUi(sessionId, webContents);
                },
                FULLSCREEN_EXIT_FALLBACK_MS);
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> maybeSuspendDismissed(sessionId, webContents),
                DISMISS_CHECK_DELAY_MS);
    }

    @Nullable
    private WebContents getOrFindWebContents() {
        if (mWebContents != null) {
            // The session is bound to a specific WebContents. If it has since been destroyed the
            // YouTube tab is gone for good, so refuse to rebind to whatever foreground tab is
            // current — that would silently retarget exit logic at the wrong WebContents.
            return mWebContents.isDestroyed() ? null : mWebContents;
        }

        // mWebContents was never set on this controller instance (typically after activity
        // recreation, when mActive is restored from the saved-instance bundle but the WebContents
        // reference cannot be persisted). Ask BraveMediaSessionHelper for the WebContents it
        // tracks for the active session: that reference is process wide and survives the
        // configuration change, whereas the foreground tab may already be a newly opened one.
        final WebContents pictureInPictureWebContents =
                BraveMediaSessionHelper.getYouTubePictureInPictureWebContents();
        if (pictureInPictureWebContents != null) {
            mWebContents = pictureInPictureWebContents;
            startObservingMediaSession(pictureInPictureWebContents);
            return pictureInPictureWebContents;
        }

        // BraveMediaSessionHelper holds the active WebContents through a WeakReference that GC
        // may have collected between save and restore. Recover via the persisted tab id instead:
        // the TabModel outlives the Activity, so the id resolves back to the original tab and
        // through it to its WebContents. We deliberately do not fall back to whatever tab is now
        // foreground; if the user switched tabs during recreation, that would silently retarget
        // the session at the wrong WebContents.
        if (mTabId == Tab.INVALID_TAB_ID) {
            return null;
        }
        final TabModelSelector tabModelSelector = getTabModelSelectorIfReady();
        if (tabModelSelector == null) {
            return null;
        }
        final Tab tab = tabModelSelector.getTabById(mTabId);
        if (tab == null) {
            return null;
        }
        final WebContents tabWebContents = tab.getWebContents();
        if (tabWebContents == null || tabWebContents.isDestroyed()) {
            return null;
        }
        mWebContents = tabWebContents;
        BraveMediaSessionHelper.setYouTubePictureInPictureWebContents(tabWebContents);
        startObservingMediaSession(tabWebContents);
        return tabWebContents;
    }

    private void maybeRestoreFullscreenUi(final int sessionId, final WebContents webContents) {
        if (!isExitingForSession(sessionId, webContents)
                || mActivity.isActivityFinishingOrDestroyed()
                || mActivity.isInPictureInPictureMode()) {
            return;
        }

        if (isScreenOffOrLocked()) {
            markInterruptedByScreenLock();
            return;
        }

        // The activity can be recreated into the tab switcher while the PiP window is up (e.g. a
        // configuration change mid-session). Expanding the PiP must land the user on the video
        // tab, so leave overview mode before restoring the fullscreen UI. Mirrors upstream's
        // exitOverviewModeOnActorPiPExpand handling for the actor PiP, which faces the same
        // layout problem on expand.
        mActivity.exitOverviewModeForYouTubePictureInPicture();

        final FullscreenManager fullscreenManager = mActivity.getFullscreenManager();
        if (fullscreenManager.getPersistentFullscreenMode()) {
            // Tear down the browser fullscreen UI through the stock upstream path. Besides
            // restoring the UI, exitPersistentFullscreenMode() also asks the WebContents to
            // exit DOM fullscreen. That second part is harmless here in both ways this method
            // can be reached:
            // - Fast path: the renderer already left DOM fullscreen (handleSessionExited
            //   requested it, and the renderer's exit report is what triggered this call), so
            //   the extra exit request is a no-op.
            // - Fallback path: the renderer never reported leaving fullscreen within the
            //   timeout, so asking it to exit again is exactly what we want.
            // This is why no Brave-specific teardown variant is needed.
            fullscreenManager.exitPersistentFullscreenMode();
        }
    }

    private void maybeSuspendDismissed(final int sessionId, final WebContents webContents) {
        if (!isExitingForSession(sessionId, webContents)) {
            return;
        }

        if (isScreenOffOrLocked()) {
            markInterruptedByScreenLock();
            return;
        }

        if (!isActivityVisibleAfterPictureInPicture()) {
            suspendMediaSession(webContents);
        }
        clearSession(sessionId, webContents);
    }

    private boolean isActivityVisibleAfterPictureInPicture() {
        return ApplicationStatus.getStateForActivity(mActivity) == ActivityState.RESUMED
                && mActivity.hasWindowFocus();
    }

    private void maybeResumeAfterScreenLock() {
        if (!mActive || !mInterruptedByScreenLock || isScreenOffOrLocked()) {
            return;
        }

        final WebContents webContents = getOrFindWebContents();
        if (webContents == null || webContents.isDestroyed()) {
            clearSession(mSessionId, mWebContents);
            return;
        }

        // The session was already tearing down when the screen locked: the user had
        // deliberately expanded out of PiP. Resurrecting it would drop them back into
        // a PiP window with the player no longer in fullscreen. Finish the exit that
        // the lock deferred instead of entering PiP again.
        if (mExiting) {
            mInterruptedByScreenLock = false;
            maybeRestoreFullscreenUi(mSessionId, webContents);
            maybeSuspendDismissed(mSessionId, webContents);
            return;
        }

        if (mActivity.isInPictureInPictureMode()) {
            // The PiP window survived the lock, so there is no re-entry to drive. Consume the
            // interrupt so the next screen-off can pause playback again (the user may resume
            // from the PiP controls in between). Playback itself deliberately stays paused:
            // YouTube videos do not auto-resume after unlock.
            mInterruptedByScreenLock = false;
            mWasPlayingBeforeScreenLock = false;
            return;
        }

        // Conditions under which we cannot re-enter PiP. Bail out *before* mutating state so the
        // session stays consistent and the receiver keeps listening for a later wake-up.
        if (mActivity.isChangingConfigurations()
                || mActivity.isActivityFinishingOrDestroyed()
                || !BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        webContents)) {
            return;
        }

        // Clear the interrupt flag *before* requesting PiP entry. The screen-state broadcast
        // receiver and the activity-resume hook can both call into here for a single wake-up
        // (e.g. broadcast races the activity lifecycle), and any later re-entry must short-circuit
        // on the !mInterruptedByScreenLock guard above instead of racing a second
        // enterPictureInPictureMode call against the in-flight one. The receiver itself stays
        // registered: it serves the whole session, including the next lock.
        mInterruptedByScreenLock = false;

        // Defer resuming the media session until Android confirms PiP entry. Mirrors the path
        // taken by an initial PiP request: onEnterPictureInPictureMode() will consume this flag
        // from onPictureInPictureModeChanged(true). If PiP entry fails below, we clear the
        // session without ever resuming, so the user does not get foreground audio after
        // unlocking.
        //
        // The resume exists to restore playback that the lock (and the PiP exit it caused)
        // interrupted, so it is scheduled only when both hold:
        // - Background video playback is enabled. Otherwise maybeSuspendForScreenLock() paused
        //   the video for the locked gap, and YouTube videos do not auto-resume after unlock
        //   outside PiP either; re-enter paused and let the user resume from the PiP controls.
        // - Media was actually playing when the lock hit. Otherwise the user had paused before
        //   locking the device, and resuming would force playback they did not ask for.
        mResumeMediaSessionOnPipEntry =
                mWasPlayingBeforeScreenLock && isBackgroundVideoPlaybackEnabled(webContents);
        mWasPlayingBeforeScreenLock = false;

        try {
            if (!mActivity.enterPictureInPictureMode(
                    new PictureInPictureParams.Builder().build())) {
                mResumeMediaSessionOnPipEntry = false;
                clearSession(mSessionId, webContents);
            }
        } catch (IllegalStateException | IllegalArgumentException e) {
            Log.e(TAG, "Error restoring YouTube picture-in-picture mode.", e);
            mResumeMediaSessionOnPipEntry = false;
            clearSession(mSessionId, webContents);
        }
    }

    private void markInterruptedByScreenLock() {
        if (mInterruptedByScreenLock) {
            // Already parked: the receiver is armed and playback has been handled.
            return;
        }

        // Re-bind before snapshotting: after activity recreation the media-session observer is
        // only re-attached by getOrFindWebContents(), and MediaSessionImpl replays the current
        // state synchronously on attach, so the snapshot below is accurate even on the first
        // interrupt after recreation.
        final WebContents webContents = getOrFindWebContents();
        mWasPlayingBeforeScreenLock = mIsMediaPlaying;
        mInterruptedByScreenLock = true;
        registerScreenStateReceiver();
        maybeSuspendForScreenLock(webContents);
    }

    /**
     * Pauses playback for the duration of a screen-lock interruption when background video playback
     * is disabled, modeling what happens to a regular tab: the video pauses when the screen goes
     * off and stays paused after unlock (YouTube videos do not auto-resume). Brave deliberately
     * swallows upstream's screen-off dismiss to keep the PiP session alive across a lock (see
     * {@code BraveFullscreenVideoPictureInPictureController#shouldKeepPictureInPictureAlive}), but
     * that dismiss is also what would have suspended playback. Letting audio continue through the
     * locked gap is only wanted when the user opted into background playback; otherwise pause here
     * and let the user resume playback from the PiP window controls after unlock.
     */
    private void maybeSuspendForScreenLock(@Nullable final WebContents webContents) {
        if (webContents == null || webContents.isDestroyed()) {
            return;
        }

        if (!isBackgroundVideoPlaybackEnabled(webContents)) {
            suspendMediaSession(webContents);
        }
    }

    /**
     * Returns true when background video playback is enabled: both the feature and the profile
     * preference. Also serves as a test seam: Profile and UserPrefs go through JNI that has no mock
     * in the Robolectric environment.
     */
    @VisibleForTesting
    protected boolean isBackgroundVideoPlaybackEnabled(final WebContents webContents) {
        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK)) {
            return false;
        }
        final Profile profile = Profile.fromWebContents(webContents);
        return profile != null
                && UserPrefs.get(profile).getBoolean(BravePref.BACKGROUND_VIDEO_PLAYBACK_ENABLED);
    }

    /**
     * Suspends the media session bound to {@code webContents}, if any. Also serves as the test
     * seam: {@link MediaSession#fromWebContents} goes through JNI that has no mock in the
     * Robolectric environment.
     */
    @VisibleForTesting
    protected void suspendMediaSession(final WebContents webContents) {
        final MediaSession mediaSession = getMediaSession(webContents);
        if (mediaSession != null) {
            mediaSession.suspend(SuspendType.SYSTEM);
        }
    }

    /**
     * Returns the media session bound to {@code webContents}, if any. Also serves as the test seam:
     * {@link MediaSession#fromWebContents} goes through JNI that has no mock in the Robolectric
     * environment.
     */
    @VisibleForTesting
    protected @Nullable MediaSession getMediaSession(final WebContents webContents) {
        return MediaSession.fromWebContents(webContents);
    }

    /**
     * Returns the activity's TabModelSelector, or null before the tab models initialize.
     * Deliberately NOT ChromeActivity#getTabModelSelector(), which THROWS (rather than returning
     * null) pre-initialization and this controller is reachable from the screen-state receiver
     * during startup. Also serves as the test seam: the supplier accessor on ChromeActivity is
     * final and cannot be stubbed.
     */
    @VisibleForTesting
    protected @Nullable TabModelSelector getTabModelSelectorIfReady() {
        final MonotonicObservableSupplier<TabModelSelector> supplier =
                mActivity.getTabModelSelectorSupplier();
        return supplier != null ? supplier.get() : null;
    }

    /**
     * Starts tracking the play/pause state of the media session bound to {@code webContents} in
     * {@code mIsMediaPlaying}. Called whenever the session binds to a WebContents; any observer
     * from a previous binding is detached first so at most one observer is ever live.
     */
    private void startObservingMediaSession(final WebContents webContents) {
        stopObservingMediaSession();
        final MediaSession mediaSession = getMediaSession(webContents);
        if (mediaSession == null) {
            return;
        }

        mMediaSessionObserver =
                new MediaSessionObserver(mediaSession) {
                    @Override
                    public void mediaSessionStateChanged(
                            boolean isControllable, boolean isSuspended) {
                        mIsMediaPlaying = !isSuspended;
                    }

                    @Override
                    public void mediaSessionDestroyed() {
                        mIsMediaPlaying = false;
                    }
                };
    }

    private void stopObservingMediaSession() {
        if (mMediaSessionObserver != null) {
            mMediaSessionObserver.stopObserving();
            mMediaSessionObserver = null;
        }
        mIsMediaPlaying = false;
    }

    /**
     * Registers the session-lifetime screen-state receiver. SCREEN_OFF is the only reliable signal
     * that the device locked while the PiP window stays alive: in that state Android does not exit
     * PiP mode and the still-playing media produces no upstream fullscreen-left signal, so without
     * this broadcast the controller would never learn about the lock — and the pause-for-lock
     * behavior (background playback disabled) would silently not happen. SCREEN_ON/USER_PRESENT
     * drive the resume side. Registered for the whole session (from {@link #onSessionRequested}
     * until {@link #clearSession}/{@link #onDestroy}) so repeated lock/unlock cycles keep working.
     */
    private void registerScreenStateReceiver() {
        if (mScreenStateReceiver != null) {
            return;
        }

        mScreenStateReceiver =
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        final String action = intent.getAction();
                        if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                            if (mActive) {
                                markInterruptedByScreenLock();
                            }
                            return;
                        }
                        if (Intent.ACTION_SCREEN_ON.equals(action)
                                || Intent.ACTION_USER_PRESENT.equals(action)) {
                            maybeResumeAfterScreenLock();
                        }
                    }
                };
        final IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_USER_PRESENT);
        ContextUtils.registerProtectedBroadcastReceiver(mActivity, mScreenStateReceiver, filter);
    }

    private void unregisterScreenStateReceiver() {
        if (mScreenStateReceiver == null) {
            return;
        }

        try {
            mActivity.unregisterReceiver(mScreenStateReceiver);
        } catch (IllegalArgumentException ignored) {
            // Screen-state receiver was already unregistered.
        }
        mScreenStateReceiver = null;
    }

    /**
     * Returns true if the (sessionId, webContents) pair refers to the currently *exiting* YT-PiP
     * session. The {@code mExiting} guard is intentional: callers of this helper are scheduled
     * during exit teardown and must no-op if the session was renewed in the meantime.
     *
     * @noinspection BooleanMethodIsAlwaysInverted
     */
    private boolean isExitingForSession(final int sessionId, final WebContents webContents) {
        return mActive
                && mExiting
                && sessionId == mSessionId
                && webContents == mWebContents
                && !webContents.isDestroyed();
    }

    private void clearSession(final int sessionId, @Nullable final WebContents webContents) {
        if (sessionId != mSessionId || webContents != mWebContents) {
            return;
        }

        unregisterScreenStateReceiver();
        // Only clear the process-global registry with a non-null WebContents to match against;
        // clear(null) wipes the slot unconditionally, and a null here (the identity guard above
        // makes it equal to mWebContents) means this session never bound one, so the slot may
        // belong to another window's live session. See the matching guard in onDestroy().
        if (webContents != null) {
            BraveMediaSessionHelper.clearYouTubePictureInPictureWebContents(webContents);
        }
        resetSessionState();
    }

    /**
     * Resets the in-memory session fields. Shared by {@link #clearSession} and {@link #onDestroy};
     * receiver unregistration and registry clearing stay at the call sites because their gating
     * differs (session-id match vs. configuration-change).
     */
    private void resetSessionState() {
        mActive = false;
        mExiting = false;
        mInterruptedByScreenLock = false;
        mWasPlayingBeforeScreenLock = false;
        mResumeMediaSessionOnPipEntry = false;
        mWebContents = null;
        mTabId = Tab.INVALID_TAB_ID;
        stopObservingMediaSession();
        if (mPendingFullscreenExitObserver != null) {
            mPendingFullscreenExitObserver.observe(null);
            mPendingFullscreenExitObserver = null;
        }
    }

    @VisibleForTesting
    public boolean isExitingForTesting() {
        return mExiting;
    }

    @VisibleForTesting
    public boolean isInterruptedByScreenLockForTesting() {
        return mInterruptedByScreenLock;
    }

    @VisibleForTesting
    public boolean hasScreenStateReceiverForTesting() {
        return mScreenStateReceiver != null;
    }

    @VisibleForTesting
    public @Nullable BroadcastReceiver getScreenStateReceiverForTesting() {
        return mScreenStateReceiver;
    }
}
