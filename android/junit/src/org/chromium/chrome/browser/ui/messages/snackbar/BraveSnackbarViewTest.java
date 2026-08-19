/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import android.app.Activity;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;

/**
 * Unit tests for the custom layouts of {@link BraveSnackbarView}. A single view is reused for every
 * snackbar of a window, so each layout must be re-applied for the snackbar that asked for it and
 * reverted for every other one.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveSnackbarViewTest {
    private static final String NOTICE_MESSAGE = "New Tab Takeover";
    private static final String NOTICE_ACTION = "Learn more and opt out choices";
    private static final String RECENT_TAB_MESSAGE = "Recent tab";
    private static final String RECENT_TAB_TITLE = "Get back to your most recent tab";
    private static final String RECENT_TAB_PAGE_TITLE = "Brave Software";
    private static final String RECENT_TAB_URL = "brave.com";

    private Activity mActivity;
    private FrameLayout mParentView;
    private BraveSnackbarManager mManager;

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        // The snackbar layout resolves Chrome styles and semantic colors.
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mParentView = new FrameLayout(mActivity);
        mManager =
                new BraveSnackbarManager(
                        mActivity,
                        mParentView,
                        /* windowAndroid= */ null,
                        /* additionalBottomMarginPxSupplier= */ null,
                        /* modalDialogManager= */ null);
    }

    @After
    public void tearDown() {
        mManager.destroy();
    }

    /**
     * Creates the view the way {@link SnackbarManager} does, without showing it: these tests
     * exercise the layout the view builds, not its animations.
     */
    private BraveSnackbarView createView(Snackbar snackbar) {
        return new BraveSnackbarView(
                mActivity,
                mManager,
                snackbar,
                mParentView,
                /* windowAndroid= */ null,
                ObservableSuppliers.alwaysZero(),
                ObservableSuppliers.alwaysFalse());
    }

    private static Snackbar makeSnackbar(String message, int type) {
        return Snackbar.make(message, new SnackbarController() {}, type, Snackbar.UMA_UNKNOWN);
    }

    /**
     * Returns the first descendant {@link TextView} showing {@code text}, or null if there is none.
     */
    private static @Nullable TextView findViewWithText(ViewGroup viewGroup, String text) {
        for (int i = 0; i < viewGroup.getChildCount(); i++) {
            View child = viewGroup.getChildAt(i);
            if (child instanceof TextView && TextUtils.equals(((TextView) child).getText(), text)) {
                return (TextView) child;
            }
            if (child instanceof ViewGroup) {
                TextView found = findViewWithText((ViewGroup) child, text);
                if (found != null) {
                    return found;
                }
            }
        }
        return null;
    }

    /**
     * The action-below-message layout must survive another snackbar taking over the view and
     * restructuring it for custom text. This used to throw ("The specified child already has a
     * parent") because the action button was still nested in the custom-text content row when it
     * was moved back into a freshly built action row.
     */
    @Test
    public void actionBelowMessageReappliedAfterCustomTextSnackbar() {
        Snackbar notice =
                makeSnackbar(NOTICE_MESSAGE, Snackbar.TYPE_PERSISTENT)
                        .setAction(NOTICE_ACTION, /* actionData= */ null);
        BraveSnackbarView view = createView(notice);
        view.setActionBelowMessage(
                android.R.drawable.ic_menu_close_clear_cancel, "Close", () -> {});

        // A notification snackbar takes the view over and restructures it for custom text.
        Snackbar recentTab = makeSnackbar(RECENT_TAB_MESSAGE, Snackbar.TYPE_NOTIFICATION);
        view.update(recentTab);
        view.setCustomText(RECENT_TAB_TITLE, RECENT_TAB_PAGE_TITLE, RECENT_TAB_URL);

        // Dismissing it hands the view back to the notice.
        view.update(notice);

        LinearLayout snackbarLayout = (LinearLayout) view.getViewForTesting();
        assertEquals(LinearLayout.VERTICAL, snackbarLayout.getOrientation());
        // The action button is back on its own row below the message.
        TextView actionButton = findViewWithText(snackbarLayout, NOTICE_ACTION);
        assertNotNull(actionButton);
        assertSame(snackbarLayout, actionButton.getParent().getParent());
        // Nothing of the custom-text layout is left behind.
        assertNull(findViewWithText(snackbarLayout, RECENT_TAB_TITLE));
    }

    /** The custom-text layout must not leak into an unrelated snackbar. */
    @Test
    public void customTextRevertedForAnotherSnackbar() {
        Snackbar recentTab = makeSnackbar(RECENT_TAB_MESSAGE, Snackbar.TYPE_NOTIFICATION);
        BraveSnackbarView view = createView(recentTab);
        view.setCustomText(RECENT_TAB_TITLE, RECENT_TAB_PAGE_TITLE, RECENT_TAB_URL);

        LinearLayout snackbarLayout = (LinearLayout) view.getViewForTesting();
        assertEquals(LinearLayout.VERTICAL, snackbarLayout.getOrientation());
        assertNotNull(findViewWithText(snackbarLayout, RECENT_TAB_TITLE));

        Snackbar notice = makeSnackbar(NOTICE_MESSAGE, Snackbar.TYPE_NOTIFICATION);
        view.update(notice);

        assertEquals(LinearLayout.HORIZONTAL, snackbarLayout.getOrientation());
        assertNull(findViewWithText(snackbarLayout, RECENT_TAB_TITLE));
        // The snackbar's own views are direct children of the snackbar layout again.
        TextView messageView = findViewWithText(snackbarLayout, NOTICE_MESSAGE);
        assertNotNull(messageView);
        assertSame(snackbarLayout, messageView.getParent());
    }

    /** The custom-text layout is re-applied when its own snackbar comes back. */
    @Test
    public void customTextReappliedForItsOwnSnackbar() {
        Snackbar recentTab = makeSnackbar(RECENT_TAB_MESSAGE, Snackbar.TYPE_NOTIFICATION);
        BraveSnackbarView view = createView(recentTab);
        view.setCustomText(RECENT_TAB_TITLE, RECENT_TAB_PAGE_TITLE, RECENT_TAB_URL);

        LinearLayout snackbarLayout = (LinearLayout) view.getViewForTesting();
        view.update(makeSnackbar(NOTICE_MESSAGE, Snackbar.TYPE_NOTIFICATION));
        view.update(recentTab);

        assertEquals(LinearLayout.VERTICAL, snackbarLayout.getOrientation());
        assertNotNull(findViewWithText(snackbarLayout, RECENT_TAB_TITLE));
    }
}
