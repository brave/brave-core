/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.when;

import android.app.Activity;
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
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.user_prefs.UserPrefsJni;
import org.chromium.ui.base.ViewUtils;

import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Regression test for a RecyclerView crash ({@code IllegalArgumentException}, either "Added View
 * has RecyclerView as parent but view is not a real child" or "Called attach on a child which is
 * not detached") when toggling "Show top sites" off then back on while the NTP isn't performing a
 * layout pass (e.g. behind the Settings screen) - both adapter notifications land in the same
 * deferred RecyclerView layout pass. See BraveNtpAdapter#getTopSitesCount for the fix: the Top
 * Sites row is a permanent adapter slot, toggled via rebind rather than insert/remove, so the
 * shared mMvTilesContainerLayout view is only ever adopted by onCreateViewHolder once.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveNtpAdapterTopSitesToggleTest {
    private static final int WIDTH = 1080;
    private static final int HEIGHT = 1920;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private PrefService mPrefService;
    @Mock private UserPrefs.Natives mUserPrefsNatives;

    private Activity mActivity;
    private BraveNtpAdapter mAdapter;
    private RecyclerView mRecyclerView;

    @Before
    public void setUp() {
        // onBindViewHolder's image-credit row reads this pref; false short-circuits the rest of
        // that branch so it doesn't touch anything else native-backed.
        UserPrefsJni.setInstanceForTesting(mUserPrefsNatives);
        when(mUserPrefsNatives.get(mProfile)).thenReturn(mPrefService);
        when(mPrefService.getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE))
                .thenReturn(false);
        ProfileManager.setLastUsedProfileForTesting(mProfile);

        mActivity = Robolectric.buildActivity(Activity.class).setup().get();

        mAdapter =
                new BraveNtpAdapter(
                        mActivity,
                        /* onBraveNtpListener= */ null,
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
                        /* recyclerViewHeight= */ 0,
                        /* isTopSitesEnabled= */ true,
                        /* isBraveStatsEnabled= */ false,
                        /* isDisplayNewsFeed= */ false,
                        /* isDisplayNewsOptin= */ false);

        mRecyclerView = new RecyclerView(mActivity);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(mActivity));
        mRecyclerView.setAdapter(mAdapter);
        // Matches BraveNewTabPageLayout#setNtpRecyclerView: without this, a plain
        // notifyItemChanged() makes RecyclerView spawn a fresh old/new ViewHolder pair for the
        // change animation instead of rebinding in place.
        if (mRecyclerView.getItemAnimator() instanceof SimpleItemAnimator) {
            ((SimpleItemAnimator) mRecyclerView.getItemAnimator())
                    .setSupportsChangeAnimations(false);
        }

        // Establish the first real layout pass, so the Top Sites row's ViewHolder/view pairing
        // already exists before the toggle sequence below.
        layoutOnce();
    }

    private void layoutOnce() {
        mRecyclerView.measure(
                MeasureSpec.makeMeasureSpec(WIDTH, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(HEIGHT, MeasureSpec.EXACTLY));
        mRecyclerView.layout(0, 0, WIDTH, HEIGHT);
    }

    @Test
    public void toggleOffThenOnWithoutInterveningLayout_doesNotCrash() {
        int itemCountBefore = mAdapter.getItemCount();

        // Mirrors the real repro: Settings -> toggle off -> toggle on -> close Settings. No
        // layout pass happens while Settings is on top, so both notifications below get batched
        // into the single layout pass forced afterwards.
        mAdapter.setTopSitesEnabled(false);
        mAdapter.setTopSitesEnabled(true);

        ViewUtils.requestLayout(
                mRecyclerView,
                "BraveNtpAdapterTopSitesToggleTest"
                        + ".toggleOffThenOnWithoutInterveningLayout_doesNotCrash");
        layoutOnce(); // Must not throw.

        // The row's presence never changes - only its bound content does.
        assertEquals(itemCountBefore, mAdapter.getItemCount());
    }
}
