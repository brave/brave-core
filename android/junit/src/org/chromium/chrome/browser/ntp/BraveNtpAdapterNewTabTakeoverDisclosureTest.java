/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.os.Looper;
import android.view.View;
import android.view.View.MeasureSpec;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.user_prefs.UserPrefsJni;

import java.util.concurrent.CopyOnWriteArrayList;

/** Unit tests for the New Tab Takeover disclosure label in {@link BraveNtpAdapter}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveNtpAdapterNewTabTakeoverDisclosureTest {
    private static final int WIDTH = 1080;
    private static final int HEIGHT = 1920;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private PrefService mPrefService;
    @Mock private UserPrefs.Natives mUserPrefsNatives;
    @Mock private OnBraveNtpListener mOnBraveNtpListener;

    private Activity mActivity;
    private RecyclerView mRecyclerView;

    @Before
    public void setUp() {
        UserPrefsJni.setInstanceForTesting(mUserPrefsNatives);
        when(mUserPrefsNatives.get(mProfile)).thenReturn(mPrefService);
        when(mPrefService.getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE))
                .thenReturn(false);
        ProfileManager.setLastUsedProfileForTesting(mProfile);

        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
    }

    private BraveNtpAdapter createAdapter(int recyclerViewHeight) {
        BraveNtpAdapter adapter =
                new BraveNtpAdapter(
                        mActivity,
                        mOnBraveNtpListener,
                        /* glide= */ null,
                        new CopyOnWriteArrayList<FeedItemsCard>(),
                        /* braveNewsController= */ null,
                        new View(mActivity),
                        /* ntpImage= */ null,
                        /* sponsoredTab= */ null,
                        /* wallpaper= */ null,
                        /* sponsoredLogo= */ null,
                        /* nTPBackgroundImagesBridge= */ null,
                        /* isNewsLoading= */ false,
                        recyclerViewHeight,
                        /* isTopSitesEnabled= */ true,
                        /* isBraveStatsEnabled= */ false,
                        /* isDisplayNewsFeed= */ false,
                        /* isDisplayNewsOptin= */ false);

        mRecyclerView = new RecyclerView(mActivity);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(mActivity));
        mRecyclerView.setAdapter(adapter);
        if (mRecyclerView.getItemAnimator() instanceof SimpleItemAnimator) {
            ((SimpleItemAnimator) mRecyclerView.getItemAnimator())
                    .setSupportsChangeAnimations(false);
        }
        layout();
        return adapter;
    }

    private void layout() {
        shadowOf(Looper.getMainLooper()).runToEndOfTasks();
        mRecyclerView.measure(
                MeasureSpec.makeMeasureSpec(WIDTH, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(HEIGHT, MeasureSpec.EXACTLY));
        mRecyclerView.layout(0, 0, WIDTH, HEIGHT);
    }

    private View getDisclosureLabel() {
        return mRecyclerView.findViewById(R.id.sponsored_label);
    }

    @Test
    public void testHideDisclosureLabelByDefault() {
        createAdapter(HEIGHT);

        assertEquals(View.GONE, getDisclosureLabel().getVisibility());
    }

    @Test
    public void testShowLabelWhenDisclosureIsVisible() {
        BraveNtpAdapter adapter = createAdapter(HEIGHT);

        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();

        assertEquals(View.VISIBLE, getDisclosureLabel().getVisibility());
    }

    @Test
    public void testHideLabelWhenDisclosureIsNotVisible() {
        BraveNtpAdapter adapter = createAdapter(HEIGHT);
        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();

        adapter.setNewTabTakeoverDisclosureVisible(false);
        layout();

        assertEquals(View.GONE, getDisclosureLabel().getVisibility());
    }

    @Test
    public void testHideDisclosureLabelWhenRecyclerViewHeightIsUnknown() {
        BraveNtpAdapter adapter = createAdapter(/* recyclerViewHeight= */ 0);

        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();

        assertEquals(View.GONE, getDisclosureLabel().getVisibility());
    }

    @Test
    public void testShowDisclosureLabelWhenRecyclerViewHeightIsSet() {
        BraveNtpAdapter adapter = createAdapter(/* recyclerViewHeight= */ 0);
        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();

        adapter.setRecyclerViewHeight(HEIGHT);
        layout();

        assertEquals(View.VISIBLE, getDisclosureLabel().getVisibility());
    }

    @Test
    public void testDisableDisclosureLabelWhenImageCreditAlphaIsZero() {
        BraveNtpAdapter adapter = createAdapter(HEIGHT);
        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();

        adapter.setImageCreditAlpha(0f);
        layout();

        assertFalse(getDisclosureLabel().isEnabled());
    }

    @Test
    public void testNotifyListenerWhenDisclosureLabelIsClicked() {
        BraveNtpAdapter adapter = createAdapter(HEIGHT);
        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();
        View disclosureLabel = getDisclosureLabel();

        disclosureLabel.performClick();

        verify(mOnBraveNtpListener).onDisclosureLabelClicked(disclosureLabel);
    }

    @Test
    public void testNotifyListenerWhenDisclosureLabelIsLongClicked() {
        BraveNtpAdapter adapter = createAdapter(HEIGHT);
        adapter.setNewTabTakeoverDisclosureVisible(true);
        layout();
        View disclosureLabel = getDisclosureLabel();

        disclosureLabel.performLongClick();

        verify(mOnBraveNtpListener).onDisclosureLabelClicked(disclosureLabel);
    }

    @Test
    public void testDoNotNotifyListenerWhenDisclosureLabelIsClickedWhileNotVisible() {
        createAdapter(HEIGHT);

        getDisclosureLabel().performClick();

        verifyNoInteractions(mOnBraveNtpListener);
    }
}
