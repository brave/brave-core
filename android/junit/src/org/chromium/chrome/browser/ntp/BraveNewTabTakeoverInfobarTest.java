/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.widget.FrameLayout;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.UnownedUserDataHost;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsNativeWorkerJni;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.ui.messages.snackbar.BraveSnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.user_prefs.UserPrefsJni;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * Unit tests for {@link BraveNewTabTakeoverInfobar}. The notice is a window-scoped snackbar owned
 * by a single NTP, so it has to go away as soon as that NTP's tab stops being visible - the tab
 * switcher and other tabs would otherwise keep showing it - without counting as an opt out.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveNewTabTakeoverInfobarTest {
    private static final int REMAINING_DISPLAY_COUNT = 3;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private PrefService mPrefService;
    @Mock private UserPrefs.Natives mUserPrefsNatives;
    @Mock private BraveRewardsNativeWorker.Natives mRewardsNatives;
    @Mock private Tab mTab;
    @Mock private WebContents mWebContents;
    @Mock private WindowAndroid mWindowAndroid;

    private Activity mActivity;
    private BraveSnackbarManager mSnackbarManager;

    @Before
    public void setUp() {
        // The notice is only for users without Rewards. The mocked natives leave the worker's
        // native pointer unset, which is how BraveRewardsHelper reports Rewards as disabled.
        BraveRewardsNativeWorkerJni.setInstanceForTesting(mRewardsNatives);

        UserPrefsJni.setInstanceForTesting(mUserPrefsNatives);
        when(mUserPrefsNatives.get(mProfile)).thenReturn(mPrefService);
        when(mPrefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT))
                .thenReturn(REMAINING_DISPLAY_COUNT);

        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        // The snackbar layout resolves Chrome styles and semantic colors.
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        // Snackbars are only shown while the activity is in the foreground.
        ApplicationStatus.onStateChangeForTesting(mActivity, ActivityState.RESUMED);

        // The snackbar has to live in a shown hierarchy for SnackbarManager#isShowing().
        FrameLayout snackbarParentView = new FrameLayout(mActivity);
        mActivity.setContentView(snackbarParentView);

        mSnackbarManager =
                new BraveSnackbarManager(
                        mActivity,
                        snackbarParentView,
                        /* windowAndroid= */ null,
                        /* additionalBottomMarginPxSupplier= */ null,
                        /* modalDialogManager= */ null);

        // The notice looks its manager up through the tab's window.
        when(mWindowAndroid.getUnownedUserDataHost()).thenReturn(new UnownedUserDataHost());
        SnackbarManagerProvider.attach(mWindowAndroid, mSnackbarManager);
        when(mWebContents.getTopLevelNativeWindow()).thenReturn(mWindowAndroid);
        when(mTab.getWebContents()).thenReturn(mWebContents);
    }

    @After
    public void tearDown() {
        SnackbarManagerProvider.detach(mSnackbarManager);
        mSnackbarManager.destroy();
    }

    /** Displays a notice the way an NTP does and returns the observer it put on the tab. */
    private TabObserver displayNotice() {
        new BraveNewTabTakeoverInfobar(mProfile).maybeDisplayAndIncrementCounter(mActivity, mTab);

        ArgumentCaptor<TabObserver> observerCaptor = ArgumentCaptor.forClass(TabObserver.class);
        verify(mTab).addObserver(observerCaptor.capture());
        return observerCaptor.getValue();
    }

    /**
     * Asserts hiding the tab for {@code hidingType} dismisses the notice but does not suppress it.
     */
    private void assertHidingTabDismissesNotice(@TabHidingType int hidingType) {
        TabObserver observer = displayNotice();
        assertTrue(mSnackbarManager.isShowing());

        observer.onHidden(mTab, hidingType);

        assertFalse(mSnackbarManager.isShowing());
        assertFalse(mSnackbarManager.hasNewTabTakeoverInfobar());
        // The observer is only useful while the notice is up; left on the tab it would dismiss
        // whichever snackbar happens to show next.
        verify(mTab).removeObserver(observer);
        // Hiding is not an opt out, so the remaining displays are left alone.
        verify(mPrefService, never())
                .setInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT, 0);
    }

    @Test
    public void displayShowsNoticeAndCountsIt() {
        displayNotice();

        assertTrue(mSnackbarManager.isShowing());
        assertTrue(mSnackbarManager.hasNewTabTakeoverInfobar());
        verify(mPrefService)
                .setInteger(
                        BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT,
                        REMAINING_DISPLAY_COUNT - 1);
    }

    @Test
    public void noticeIsDismissedWhenTabSwitcherIsShown() {
        assertHidingTabDismissesNotice(TabHidingType.TAB_SWITCHER_SHOWN);
    }

    @Test
    public void noticeIsDismissedWhenSwitchingToAnotherTab() {
        assertHidingTabDismissesNotice(TabHidingType.CHANGED_TABS);
    }

    @Test
    public void noticeIsDismissedWhenActivityIsHidden() {
        assertHidingTabDismissesNotice(TabHidingType.ACTIVITY_HIDDEN);
    }

    @Test
    public void noticeIsDismissedWhenTabIsReparented() {
        assertHidingTabDismissesNotice(TabHidingType.REPARENTED);
    }

    @Test
    public void noticeCanBeShownAgainAfterItsTabWasHidden() {
        displayNotice().onHidden(mTab, TabHidingType.TAB_SWITCHER_SHOWN);
        assertFalse(mSnackbarManager.isShowing());

        // A later NTP brings its own notice, so the dismissed one must not hold on to the manager's
        // single outstanding slot.
        new BraveNewTabTakeoverInfobar(mProfile).maybeDisplayAndIncrementCounter(mActivity, mTab);

        assertTrue(mSnackbarManager.isShowing());
        assertTrue(mSnackbarManager.hasNewTabTakeoverInfobar());
    }

    @Test
    public void noticeStopsObservingItsTabOnceDismissed() {
        TabObserver observer = displayNotice();

        mSnackbarManager.dismissAllSnackbars();

        assertFalse(mSnackbarManager.hasNewTabTakeoverInfobar());
        verify(mTab).removeObserver(observer);
    }

    @Test
    public void learnMoreSuppressesNoticeAndStopsObservingItsTab() {
        TabObserver observer = displayNotice();

        // Pressing the action button; the support page it opens needs a BraveActivity, which this
        // test has not, and TabUtils tolerates that.
        mSnackbarManager.onClick(/* v= */ null);

        assertFalse(mSnackbarManager.isShowing());
        assertFalse(mSnackbarManager.hasNewTabTakeoverInfobar());
        verify(mTab).removeObserver(observer);
        verify(mPrefService)
                .setInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT, 0);
    }

    @Test
    public void noticeIsNotDisplayedWhenNoDisplaysAreLeft() {
        when(mPrefService.getInteger(BravePref.NEW_TAB_TAKEOVER_INFOBAR_REMAINING_DISPLAY_COUNT))
                .thenReturn(0);

        new BraveNewTabTakeoverInfobar(mProfile).maybeDisplayAndIncrementCounter(mActivity, mTab);

        assertFalse(mSnackbarManager.isShowing());
        verify(mTab, never()).addObserver(any());
        verify(mPrefService, never()).setInteger(anyString(), anyInt());
    }
}
