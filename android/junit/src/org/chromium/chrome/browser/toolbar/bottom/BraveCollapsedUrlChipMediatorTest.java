/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.verify;

import android.content.ContentResolver;
import android.content.Context;
import android.provider.Settings;
import android.view.View;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import androidx.test.core.app.ApplicationProvider;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;

import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.function.Supplier;

/** Tests for {@link BraveCollapsedUrlChipMediator}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveCollapsedUrlChipMediatorTest {
    private static final String URL_SPEC = "https://example.com/page";
    private static final String FORMATTED = "example.com";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private BrowserControlsSizer mSizer;

    private ArgumentCaptor<BrowserControlsStateProvider.Observer> mObserverCaptor;
    private DelegateFake mDelegate;
    private AtomicReference<String> mUrlSpecRef;
    private AtomicReference<Boolean> mFullscreenRef;
    private AtomicReference<Boolean> mCanShowRef;
    private Supplier<Boolean> mEnabledSupplier;
    private Function<String, String> mTextFormatter;
    private BraveCollapsedUrlChipMediator mMediator;

    @Before
    public void setUp() {
        mObserverCaptor = ArgumentCaptor.forClass(BrowserControlsStateProvider.Observer.class);
        mDelegate = new DelegateFake();
        mUrlSpecRef = new AtomicReference<>(URL_SPEC);
        mFullscreenRef = new AtomicReference<>(Boolean.FALSE);
        mCanShowRef = new AtomicReference<>(Boolean.TRUE);
        mEnabledSupplier = () -> true;
        mTextFormatter = spec -> URL_SPEC.equals(spec) ? FORMATTED : null;
        createMediator();
    }

    private void createMediator() {
        mMediator =
                new BraveCollapsedUrlChipMediator(
                        mSizer,
                        mUrlSpecRef::get,
                        mEnabledSupplier,
                        mFullscreenRef::get,
                        mCanShowRef::get,
                        mTextFormatter,
                        mDelegate);
        // Capture the observer registered by the most recent construction.
        verify(mSizer, atLeastOnce()).addObserver(mObserverCaptor.capture());
    }

    private BrowserControlsStateProvider.Observer observer() {
        return mObserverCaptor.getValue();
    }

    /** Simulates Chromium swapping the toolbar's Android view for a composited texture. */
    private void notifyAndroidControlsVisibility(int visibility) {
        observer().onAndroidControlsVisibilityChanged(visibility);
    }

    @Test
    public void testChipHiddenWhileToolbarVisible() {
        notifyOffsetChanged();

        assertFalse(mDelegate.mLastVisible);
        assertEquals(0f, mDelegate.mLastAlpha, 0.001f);
    }

    @Test
    public void testChipShownWhenToolbarScrolledAway() {
        notifyAndroidControlsVisibility(View.INVISIBLE);

        assertTrue(mDelegate.mLastVisible);
        assertEquals(1.0f, mDelegate.mLastAlpha, 0.001f);
        assertEquals(FORMATTED, mDelegate.mLastText);
    }

    @Test
    public void testChipHidesWhenToolbarShownAgain() {
        notifyAndroidControlsVisibility(View.INVISIBLE);
        assertTrue(mDelegate.mLastVisible);

        notifyAndroidControlsVisibility(View.VISIBLE);
        assertFalse(mDelegate.mLastVisible);
        assertEquals(0f, mDelegate.mLastAlpha, 0.001f);
    }

    @Test
    public void testChipShownWhenAndroidControlsGone() {
        notifyAndroidControlsVisibility(View.GONE);
        assertTrue(mDelegate.mLastVisible);
    }

    @Test
    public void testChipHiddenWithoutUrl() {
        mUrlSpecRef.set(null);
        createMediator();

        notifyAndroidControlsVisibility(View.INVISIBLE);

        assertFalse(mDelegate.mLastVisible);
        assertNull(mDelegate.mLastText);
    }

    @Test
    public void testChipHiddenWhenDisabled() {
        mEnabledSupplier = () -> false;
        createMediator();

        notifyAndroidControlsVisibility(View.INVISIBLE);

        assertFalse(mDelegate.mLastVisible);
    }

    @Test
    public void testChipHiddenInPersistentFullscreen() {
        mFullscreenRef.set(Boolean.TRUE);
        notifyAndroidControlsVisibility(View.INVISIBLE);
        assertFalse(mDelegate.mLastVisible);

        mFullscreenRef.set(Boolean.FALSE);
        notifyOffsetChanged();
        assertTrue(mDelegate.mLastVisible);
    }

    @Test
    public void testChipHiddenWhenControlsDelegateHidden() {
        // E.g. the omnibox is focused: the visibility delegate is set to HIDDEN, and the chip
        // must not appear even though the toolbar's Android view is not visible.
        mCanShowRef.set(Boolean.FALSE);
        notifyAndroidControlsVisibility(View.INVISIBLE);
        assertFalse(mDelegate.mLastVisible);

        mCanShowRef.set(Boolean.TRUE);
        notifyOffsetChanged();
        assertTrue(mDelegate.mLastVisible);
    }

    @Test
    public void testTextStableWhileSpecUnchanged() {
        notifyAndroidControlsVisibility(View.INVISIBLE);
        String before = mDelegate.mLastText;
        assertEquals(FORMATTED, before);

        // Same spec again: text must remain identical (no re-format while scrolling).
        notifyOffsetChanged();
        assertEquals(before, mDelegate.mLastText);
    }

    @Test
    public void testUrlSpecChangedUpdatesText() {
        notifyAndroidControlsVisibility(View.INVISIBLE);
        mUrlSpecRef.set("https://bravesoftware.com/home");
        mTextFormatter =
                spec -> spec.equals("https://bravesoftware.com/home") ? "bravesoftware.com" : null;
        createMediator();
        // Restore the hidden state: the new mediator instance starts with the toolbar visible.
        notifyAndroidControlsVisibility(View.INVISIBLE);

        assertTrue(mDelegate.mLastVisible);
        assertEquals("bravesoftware.com", mDelegate.mLastText);

        mUrlSpecRef.set(null);
        mMediator.onUrlSpecChanged();
        assertFalse(mDelegate.mLastVisible);
        assertNull(mDelegate.mLastText);
    }

    @Test
    public void testDestroyRemovesObserverAndHides() {
        mMediator.destroy();
        verify(mSizer).removeObserver(observer());
        assertFalse(mDelegate.mLastVisible);
        assertEquals(0f, mDelegate.mLastAlpha, 0.001f);
        assertNull(mDelegate.mLastText);
    }

    @Test
    public void testStrayCallbackAfterDestroyDoesNotReshow() {
        mMediator.destroy();
        notifyAndroidControlsVisibility(View.INVISIBLE);
        assertFalse(mDelegate.mLastVisible);
    }

    @Test
    public void testIsEligibleUrlSpec() {
        assertTrue(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("https://example.com/page"));
        assertTrue(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("http://example.com/"));
        // The host component (after the userinfo delimiter) is what counts.
        assertTrue(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("https://example.com@evil.com/"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("chrome://version/"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("about:blank"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("file:///tmp/x"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("ftp://example.com/"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("content://com.example/x"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("brave://settings"));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec(""));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec(null));
        assertFalse(BraveCollapsedUrlChipMediator.isEligibleUrlSpec("not a url"));
    }

    @Test
    public void testIsLightContentBackground() {
        assertTrue(
                BraveCollapsedUrlChipMediator.isLightContentBackground(
                        android.graphics.Color.WHITE));
        assertTrue(
                BraveCollapsedUrlChipMediator.isLightContentBackground(
                        android.graphics.Color.TRANSPARENT));
        assertTrue(
                BraveCollapsedUrlChipMediator.isLightContentBackground(0xF2FFFFFF)); // chip light
        assertFalse(
                BraveCollapsedUrlChipMediator.isLightContentBackground(
                        android.graphics.Color.BLACK));
        assertFalse(
                BraveCollapsedUrlChipMediator.isLightContentBackground(0xF21F2026)); // chip dark
        // Translucent black composes over white to ~50% luminance -> not light.
        assertFalse(
                BraveCollapsedUrlChipMediator.isLightContentBackground(
                        android.graphics.Color.argb(128, 0, 0, 0)));
        // Mid gray: system pill shows light on this, chip does the same.
        assertFalse(
                BraveCollapsedUrlChipMediator.isLightContentBackground(
                        android.graphics.Color.rgb(128, 128, 128)));
    }

    @Test
    public void testIsGestureNavigationFollowsSetting() {
        // Fresh read required: nav mode can flip mid-session.
        Context context = ApplicationProvider.getApplicationContext();
        ContentResolver resolver = context.getContentResolver();
        Settings.Secure.putInt(resolver, "navigation_mode", 2); // gesture
        assertTrue(BraveCollapsedUrlChipMediator.isGestureNavigation(resolver));
        // Same resolver, mode flipped mid-session: next call must see it.
        Settings.Secure.putInt(resolver, "navigation_mode", 0); // 3-button
        assertFalse(BraveCollapsedUrlChipMediator.isGestureNavigation(resolver));
        Settings.Secure.putInt(resolver, "navigation_mode", 1); // 2-button
        assertFalse(BraveCollapsedUrlChipMediator.isGestureNavigation(resolver));
        // Key absent (fresh profile): treat as not gesture, never hide-then-stuck.
        Settings.Secure.putString(resolver, "navigation_mode", null);
        assertFalse(BraveCollapsedUrlChipMediator.isGestureNavigation(resolver));
    }

    @Test
    public void testNavBarBottomMarginAddsMinGap() {
        float density2_625 = 2.625f; // emulator-5556: 8dp gap = 21px
        assertEquals(21, BraveCollapsedUrlChipMediator.navBarBottomMarginPx(0, density2_625));
        // Observed device geometry: gesture-mode inset 63px + 21px gap = 84px margin.
        assertEquals(84, BraveCollapsedUrlChipMediator.navBarBottomMarginPx(63, density2_625));
        // Zero inset must still keep the gap (nav bar can be transparent but present).
        assertEquals(8, BraveCollapsedUrlChipMediator.navBarBottomMarginPx(0, 1f));
        assertEquals(63, BraveCollapsedUrlChipMediator.navBarBottomMarginPx(55, 1f));
        // Rounding: 8 * 2.7 = 21.6 -> 22.
        assertEquals(22, BraveCollapsedUrlChipMediator.navBarBottomMarginPx(0, 2.7f));
    }

    private void notifyOffsetChanged() {
        observer().onControlsOffsetChanged(0, 0, false, 60, 0, false, true, false);
    }

    private static class DelegateFake implements BraveCollapsedUrlChipMediator.Delegate {
        private @Nullable String mLastText;
        private boolean mLastVisible;
        private float mLastAlpha;

        @Override
        public void setChipText(@Nullable String text) {
            mLastText = text;
        }

        @Override
        public void setChipVisible(boolean visible) {
            mLastVisible = visible;
        }

        @Override
        public void setChipAlpha(float alpha) {
            mLastAlpha = alpha;
        }
    }
}
