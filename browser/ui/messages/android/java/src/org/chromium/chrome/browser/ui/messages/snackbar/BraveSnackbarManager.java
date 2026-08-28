/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import android.app.Activity;
import android.view.ViewGroup;

import org.chromium.base.Log;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Brave's extension of SnackbarManager. */
@NullMarked
public class BraveSnackbarManager extends SnackbarManager {
    private static final String TAG = "BraveSnackbarManager";

    // Will be deleted in bytecode. Variable from the parent class will be used instead.
    @SuppressWarnings({"UnusedVariable", "HidingField"})
    protected @Nullable SnackbarView mView;

    // Customizations requested for a snackbar, replayed onto whichever view ends up showing it
    // (see applyCustomizations). They are held here rather than in the view because the view does
    // not survive: SnackbarManager rebuilds it when a snackbar is swiped away, and callers rebuild
    // it by dismissing and re-showing a snackbar to update it. Each slot holds the last requester
    // of that kind, and is keyed by the snackbar so it can never be applied to an unrelated one.
    private @Nullable Snackbar mCustomTextSnackbar;
    private String mCustomTextTitle = "";
    private String mCustomTextPageTitle = "";
    private String mCustomTextUrl = "";

    private @Nullable Snackbar mActionBelowSnackbar;
    private int mActionBelowCloseIconResId;
    private @Nullable String mActionBelowCloseContentDescription;
    private @Nullable Runnable mActionBelowCloseCallback;

    private @Nullable Snackbar mClickableSnackbar;
    private @Nullable Runnable mPendingClickCallback;

    // The single New Tab Takeover notice currently outstanding in this (window-scoped) manager, or
    // null when none is showing or queued. Each NTP tab creates its own BraveNewTabTakeoverInfobar
    // but they share this manager, so remembering the live notice here lets a later NTP detect it
    // and avoid enqueueing a duplicate. It is released from the notice's SnackbarController on
    // every
    // dismissal path, and is naturally gone when this manager (and its activity) is destroyed, so
    // unlike a static flag it cannot leak across sessions or get stuck set.
    private @Nullable Snackbar mNewTabTakeoverInfobar;

    public BraveSnackbarManager(
            Activity activity,
            ViewGroup snackbarParentView,
            @Nullable WindowAndroid windowAndroid,
            @Nullable NonNullObservableSupplier<Integer> additionalBottomMarginPxSupplier,
            @Nullable ModalDialogManager modalDialogManager) {
        super(
                activity,
                snackbarParentView,
                windowAndroid,
                additionalBottomMarginPxSupplier,
                modalDialogManager);
    }

    public BraveSnackbarManager(
            Activity activity,
            ViewGroup snackbarParentView,
            @Nullable WindowAndroid windowAndroid,
            @Nullable NonNullObservableSupplier<Integer> additionalBottomMarginPxSupplier,
            @Nullable ModalDialogManager modalDialogManager,
            NonNullObservableSupplier<Boolean> persistentFullscreenModeSupplier) {
        super(
                activity,
                snackbarParentView,
                windowAndroid,
                additionalBottomMarginPxSupplier,
                modalDialogManager,
                persistentFullscreenModeSupplier);
    }

    /**
     * Applies to {@code view} everything requested for {@code snackbar}, which must be the snackbar
     * the view is showing. Called by {@link BraveSnackbarView} whenever it is built or updated for
     * a snackbar, so a request made while the snackbar was still queued is honoured as soon as it
     * reaches the screen, in whichever view shows it.
     */
    void applyCustomizations(BraveSnackbarView view, Snackbar snackbar) {
        if (snackbar == mCustomTextSnackbar) {
            view.setCustomText(snackbar, mCustomTextTitle, mCustomTextPageTitle, mCustomTextUrl);
        }

        if (snackbar == mActionBelowSnackbar) {
            view.setActionBelowMessage(
                    snackbar,
                    mActionBelowCloseIconResId,
                    mActionBelowCloseContentDescription,
                    mActionBelowCloseCallback);
        }

        if (snackbar == mClickableSnackbar && mPendingClickCallback != null) {
            view.makeClickable(snackbar, mPendingClickCallback);
        }
    }

    /** Applies the customizations requested for {@code snackbar} to the view showing it, if any. */
    private void applyCustomizations(Snackbar snackbar) {
        if (mView instanceof BraveSnackbarView) {
            applyCustomizations((BraveSnackbarView) mView, snackbar);
        }
    }

    /** Returns whether a New Tab Takeover notice is currently outstanding (showing or queued). */
    public boolean hasNewTabTakeoverInfobar() {
        return mNewTabTakeoverInfobar != null;
    }

    /**
     * Shows the given New Tab Takeover notice and remembers it as the single outstanding one (see
     * {@link #hasNewTabTakeoverInfobar()}). The caller must release it via {@link
     * #clearNewTabTakeoverInfobar()} from the notice's controller when it is dismissed.
     */
    public void showNewTabTakeoverInfobar(Snackbar snackbar) {
        mNewTabTakeoverInfobar = snackbar;
        showSnackbar(snackbar);
    }

    /** Releases the remembered New Tab Takeover notice once it has been dismissed. */
    public void clearNewTabTakeoverInfobar() {
        mNewTabTakeoverInfobar = null;
    }

    /**
     * Makes the given snackbar clickable as a whole. The callback runs when that snackbar is
     * tapped, and only while it is the snackbar being shown.
     *
     * @param snackbar The snackbar the callback belongs to.
     * @param clickCallback Callback to execute when the snackbar is tapped.
     */
    public void makeSnackbarClickable(Snackbar snackbar, Runnable clickCallback) {
        if (clickCallback == null) {
            Log.e(TAG, "makeSnackbarClickable: clickCallback is null");
            return;
        }

        mClickableSnackbar = snackbar;
        mPendingClickCallback = clickCallback;
        applyCustomizations(snackbar);
    }

    /**
     * Sets custom text on the given snackbar, with title, page title, and URL.
     *
     * @param snackbar The snackbar the text belongs to.
     * @param title The title text (e.g., "Get back to your most recent tab")
     * @param pageTitle The page title
     * @param url The URL to display
     */
    public void setCustomText(Snackbar snackbar, String title, String pageTitle, String url) {
        mCustomTextSnackbar = snackbar;
        mCustomTextTitle = title;
        mCustomTextPageTitle = pageTitle;
        mCustomTextUrl = url;
        applyCustomizations(snackbar);
    }

    /**
     * Switches the given snackbar to a layout where the action button sits on its own line below
     * the message, optionally with a trailing close button (see {@link
     * BraveSnackbarView#setActionBelowMessage(Snackbar, int, String, Runnable)}). Must be called
     * after {@link #showSnackbar(Snackbar)}. The layout is applied when that snackbar is the one
     * being shown, so a snackbar queued behind a higher priority one does not restyle it.
     *
     * @param snackbar The snackbar the layout belongs to.
     * @param closeIconResId Drawable resource for the close button. Ignored when {@code
     *     onCloseCallback} is null.
     * @param closeContentDescription Accessibility label for the close button, or null.
     * @param onCloseCallback Invoked when the close button is tapped; when null no close button is
     *     added.
     */
    public void setActionBelowMessage(
            Snackbar snackbar,
            int closeIconResId,
            @Nullable String closeContentDescription,
            @Nullable Runnable onCloseCallback) {
        mActionBelowSnackbar = snackbar;
        mActionBelowCloseIconResId = closeIconResId;
        mActionBelowCloseContentDescription = closeContentDescription;
        mActionBelowCloseCallback = onCloseCallback;
        applyCustomizations(snackbar);
    }
}
