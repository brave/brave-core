/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.graphics.Bitmap;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.url.GURL;
import org.chromium.url.JUnitTestGURLs;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;

/**
 * Tests for the blocked-tracker favicon display logic in {@link BraveUnifiedPanelHandler}.
 * Specifically exercises {@code populateBlockedItemsContainer} which prioritises
 * successfully-loaded favicons over letter placeholders and caps at {@code MAX_BLOCKED_ICONS} (3).
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveUnifiedPanelFaviconPriorityTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    private Activity mActivity;
    private BraveUnifiedPanelHandler mHandler;
    private LinearLayout mBlockedItemsContainer;

    @Before
    public void setUp() throws Exception {
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mHandler = new BraveUnifiedPanelHandler(mActivity);

        mBlockedItemsContainer = new LinearLayout(mActivity);
        setField("mContext", mActivity);
        setField("mBlockedItemsContainer", mBlockedItemsContainer);
        setField("mFaviconIconSize", 48);
    }

    @Test
    public void testFaviconsDisplayedBeforePlaceholders() throws Exception {
        GURL failOrigin1 = JUnitTestGURLs.URL_1;
        GURL failOrigin2 = JUnitTestGURLs.URL_2;
        Bitmap successBitmap = Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888);

        getSuccessBitmaps().add(successBitmap);
        getFailedOrigins().add(failOrigin1);
        getFailedOrigins().add(failOrigin2);

        invokePopulate();

        assertEquals(3, mBlockedItemsContainer.getChildCount());
        assertChildIsFavicon(0);
        assertChildIsPlaceholder(1);
        assertChildIsPlaceholder(2);
    }

    @Test
    public void testMaxThreeIconsDisplayed() throws Exception {
        for (int i = 0; i < 5; i++) {
            getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        }

        invokePopulate();

        assertEquals(3, mBlockedItemsContainer.getChildCount());
    }

    @Test
    public void testOnlyPlaceholdersWhenNoFaviconsLoaded() throws Exception {
        getFailedOrigins().add(JUnitTestGURLs.URL_1);
        getFailedOrigins().add(JUnitTestGURLs.URL_2);

        invokePopulate();

        assertEquals(2, mBlockedItemsContainer.getChildCount());
        assertChildIsPlaceholder(0);
        assertChildIsPlaceholder(1);
    }

    @Test
    public void testOnlyFaviconsWhenAllSucceeded() throws Exception {
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));

        invokePopulate();

        assertEquals(2, mBlockedItemsContainer.getChildCount());
        assertChildIsFavicon(0);
        assertChildIsFavicon(1);
    }

    @Test
    public void testEmptyWhenNoResults() throws Exception {
        invokePopulate();

        assertEquals(0, mBlockedItemsContainer.getChildCount());
    }

    @Test
    public void testPlaceholderSlotsFillAfterFavicons() throws Exception {
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        getFailedOrigins().add(JUnitTestGURLs.URL_1);
        getFailedOrigins().add(JUnitTestGURLs.URL_2);

        invokePopulate();

        assertEquals(3, mBlockedItemsContainer.getChildCount());
        assertChildIsFavicon(0);
        assertChildIsFavicon(1);
        assertChildIsPlaceholder(2);
    }

    @Test
    public void testOnFaviconResult_countsDownAndPopulates() throws Exception {
        setField("mPendingFaviconLoads", 2);

        Bitmap bitmap = Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888);
        invokeOnFaviconResult(JUnitTestGURLs.URL_1, bitmap);
        assertEquals("Should not populate yet", 0, mBlockedItemsContainer.getChildCount());

        invokeOnFaviconResult(JUnitTestGURLs.URL_2, null);
        assertEquals(
                "Should populate after all loads complete",
                2,
                mBlockedItemsContainer.getChildCount());
        assertChildIsFavicon(0);
        assertChildIsPlaceholder(1);
    }

    @Test
    public void testCascadingMargins() throws Exception {
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));
        getSuccessBitmaps().add(Bitmap.createBitmap(48, 48, Bitmap.Config.ARGB_8888));

        invokePopulate();

        LinearLayout.LayoutParams firstParams =
                (LinearLayout.LayoutParams) mBlockedItemsContainer.getChildAt(0).getLayoutParams();
        LinearLayout.LayoutParams secondParams =
                (LinearLayout.LayoutParams) mBlockedItemsContainer.getChildAt(1).getLayoutParams();
        LinearLayout.LayoutParams thirdParams =
                (LinearLayout.LayoutParams) mBlockedItemsContainer.getChildAt(2).getLayoutParams();

        assertEquals("First icon has no start margin", 0, firstParams.getMarginStart());
        assertTrue("Second icon has negative start margin", secondParams.getMarginStart() < 0);
        assertTrue("Third icon has negative start margin", thirdParams.getMarginStart() < 0);
    }

    /**
     * Checks that the child at the given index is a FrameLayout containing an ImageView with a
     * bitmap (i.e., a real favicon, not a letter placeholder).
     */
    private void assertChildIsFavicon(int index) {
        assertTrue(
                "Child " + index + " should be a FrameLayout",
                mBlockedItemsContainer.getChildAt(index) instanceof FrameLayout);
        FrameLayout frame = (FrameLayout) mBlockedItemsContainer.getChildAt(index);
        boolean hasBitmapImageView = false;
        for (int i = 0; i < frame.getChildCount(); i++) {
            if (frame.getChildAt(i) instanceof ImageView
                    && !(frame.getChildAt(i) instanceof TextView)) {
                hasBitmapImageView = true;
                break;
            }
        }
        assertTrue("Child " + index + " should contain a favicon ImageView", hasBitmapImageView);
    }

    /**
     * Checks that the child at the given index is a FrameLayout containing a TextView (the letter
     * placeholder).
     */
    private void assertChildIsPlaceholder(int index) {
        assertTrue(
                "Child " + index + " should be a FrameLayout",
                mBlockedItemsContainer.getChildAt(index) instanceof FrameLayout);
        FrameLayout frame = (FrameLayout) mBlockedItemsContainer.getChildAt(index);
        boolean hasTextView = false;
        for (int i = 0; i < frame.getChildCount(); i++) {
            if (frame.getChildAt(i) instanceof TextView) {
                hasTextView = true;
                break;
            }
        }
        assertTrue("Child " + index + " should contain a placeholder TextView", hasTextView);
    }

    @SuppressWarnings("unchecked")
    private List<Bitmap> getSuccessBitmaps() throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField("mFaviconSuccessBitmaps");
        field.setAccessible(true);
        return (List<Bitmap>) field.get(mHandler);
    }

    @SuppressWarnings("unchecked")
    private List<GURL> getFailedOrigins() throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField("mFaviconFailedOrigins");
        field.setAccessible(true);
        return (List<GURL>) field.get(mHandler);
    }

    private void setField(String name, Object value) throws Exception {
        Field field = BraveUnifiedPanelHandler.class.getDeclaredField(name);
        field.setAccessible(true);
        field.set(mHandler, value);
    }

    private void invokePopulate() throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod("populateBlockedItemsContainer");
        method.setAccessible(true);
        method.invoke(mHandler);
    }

    private void invokeOnFaviconResult(GURL origin, Bitmap bitmap) throws Exception {
        Method method =
                BraveUnifiedPanelHandler.class.getDeclaredMethod(
                        "onFaviconResult", GURL.class, Bitmap.class);
        method.setAccessible(true);
        method.invoke(mHandler, origin, bitmap);
    }
}
