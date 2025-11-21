/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveUrlConstants;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabClosureParams;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.chrome.browser.ui.messages.snackbar.BraveSnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;

@NullMarked
public class BraveRecentTabsSnackbarHelper {
    private static final String TAG = "RecentTabsSnackbar";
    private static final int SNACKBAR_DISMISS_DELAY_MS = 8000; // 8 seconds
    private static final int SNACKBAR_SHOW_DELAY_MS = 500; // Wait for view hierarchy to settle

    private @Nullable Tab mLastTab;
    // Generates a fallback icon (colored rounded rectangle with letter) when no favicon is
    // available
    // from FaviconHelper. This ensures the snackbar always has a visual identifier for the tab.
    private @Nullable RoundedIconGenerator mIconGenerator;
    private final FaviconHelper mFaviconHelper;
    private @Nullable Snackbar mCurrentSnackbar;
    private @Nullable BraveSnackbarManager mSnackbarManager;
    private @Nullable SnackbarController mSnackbarController;
    // Prevents duplicate action calls when the snackbar is clicked. Since the entire snackbar
    // is clickable via makeSnackbarClickable(), this flag ensures switchToLastTabAndDismiss()
    // is only called once per user interaction, even if the click handler fires multiple times.
    private boolean mUserClickedSnackbar;
    // Prevents onDismissNoAction from cleaning up resources when the snackbar is programmatically
    // dismissed and re-shown to update the favicon image. Only cleanup when actually dismissed.
    private boolean mIsUpdatingSnackbar;
    // Prevents use-after-destroy bugs. Async callbacks (like favicon loading) may fire after
    // destroy()
    // is called, so this flag ensures we don't access resources or update UI after cleanup.
    private boolean mDestroyed;
    // Store text values to restore after favicon updates
    private String mSnackbarTitle = "";
    private String mSnackbarPageTitle = "";
    private String mSnackbarUrl = "";

    public BraveRecentTabsSnackbarHelper() {
        mFaviconHelper = new FaviconHelper();
    }

    /**
     * Shows a snackbar with information about the last opened tab. The snackbar allows the user to
     * switch back to that tab.
     */
    public void showSnackbar(BraveActivity activity) {
        if (activity == null) {
            Log.e(TAG, "showSnackbar: BraveActivity is null");
            return;
        }

        // Get BraveSnackbarManager from WindowAndroid
        Tab currentTab = activity.getActivityTab();
        if (currentTab == null) {
            Log.e(TAG, "showSnackbar: currentTab is null");
            return;
        }

        WindowAndroid windowAndroid = currentTab.getWindowAndroid();
        if (windowAndroid == null) {
            Log.e(TAG, "showSnackbar: windowAndroid is null");
            return;
        }

        mSnackbarManager = (BraveSnackbarManager) SnackbarManagerProvider.from(windowAndroid);
        if (mSnackbarManager == null) {
            Log.e(TAG, "showSnackbar: BraveSnackbarManager is null");
            return;
        }

        // Get the last opened tab
        mLastTab = getLastOpenedTab(activity, currentTab);
        if (mLastTab == null) {
            Log.e(TAG, "showSnackbar: getLastOpenedTab returned null");
            return;
        }

        // Get tab URL and title
        GURL url = mLastTab.getUrl();
        String tabTitle = mLastTab.getTitle();

        if (url == null || url.isEmpty()) {
            Log.e(TAG, "showSnackbar: url is null or empty");
            return;
        }

        // Create snackbar controller
        mSnackbarController =
                new SnackbarController() {
                    @Override
                    public void onAction(@Nullable Object actionData) {
                        switchToLastTabAndDismiss(activity);
                    }

                    @Override
                    public void onDismissNoAction(@Nullable Object actionData) {
                        // User dismissed without action or snackbar auto-dismissed - just dismiss,
                        // stay on NTP
                        // (updating dismisses/re-shows the snackbar, which triggers this callback)
                        if (!mIsUpdatingSnackbar) {
                            // Clear references - snackbar is already dismissed by the system
                            mCurrentSnackbar = null;
                            mSnackbarController = null;
                            // Cleanup resources
                            destroy();
                        }
                    }
                };

        // Create snackbar with title and tab information
        String title = activity.getString(org.chromium.chrome.R.string.recent_tabs_dialog_title);
        // Format URL for display (remove protocol and www if present)
        String displayUrl = url.getSpec();
        String scheme = url.getScheme();
        String httpPrefix = UrlConstants.HTTP_SCHEME + "://";
        String httpsPrefix = UrlConstants.HTTPS_SCHEME + "://";
        if (UrlConstants.HTTP_SCHEME.equals(scheme)) {
            // Remove "http://" prefix
            if (displayUrl.startsWith(httpPrefix)) {
                displayUrl = displayUrl.substring(httpPrefix.length());
            }
        } else if (UrlConstants.HTTPS_SCHEME.equals(scheme)) {
            // Remove "https://" prefix
            if (displayUrl.startsWith(httpsPrefix)) {
                displayUrl = displayUrl.substring(httpsPrefix.length());
            }
        }

        // Remove www. prefix if present
        String wwwPrefix = BraveUrlConstants.WWW_PREFIX + ".";
        if (displayUrl.startsWith(wwwPrefix)) {
            displayUrl = displayUrl.substring(wwwPrefix.length());
        }

        // Remove trailing slash if present
        if (displayUrl.endsWith("/")) {
            displayUrl = displayUrl.substring(0, displayUrl.length() - 1);
        }

        // Store text values for later restoration after favicon updates
        mSnackbarTitle = title != null ? title : "";
        mSnackbarPageTitle = tabTitle != null ? tabTitle : "";
        mSnackbarUrl = displayUrl != null ? displayUrl : "";

        // Use a placeholder message - will be replaced with custom formatted text
        String message = tabTitle != null && !tabTitle.isEmpty() ? tabTitle : displayUrl;

        // Create snackbar without action button - entire snackbar will be clickable
        mCurrentSnackbar =
                Snackbar.make(
                                message,
                                mSnackbarController,
                                Snackbar.TYPE_NOTIFICATION,
                                Snackbar.UMA_UNKNOWN)
                        .setDuration(SNACKBAR_DISMISS_DELAY_MS);

        // Delay showing snackbar to let view hierarchy settle first
        // This gives MessageContainer time to be properly positioned before we adjust it
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    try {
                        if (mCurrentSnackbar == null || mSnackbarManager == null) {
                            return;
                        }

                        mSnackbarManager.showSnackbar(mCurrentSnackbar);

                        // Set custom formatted text with title, page title, and URL
                        mSnackbarManager.setCustomText(
                                mSnackbarTitle, mSnackbarPageTitle, mSnackbarUrl);

                        // Make entire snackbar clickable
                        mSnackbarManager.makeSnackbarClickable(
                                () -> {
                                    if (!mUserClickedSnackbar && mSnackbarController != null) {
                                        mUserClickedSnackbar = true;
                                        mSnackbarController.onAction(null);
                                    }
                                });

                        // Fetch favicon and set profile image (asynchronously)
                        fetchFaviconAndSetProfileImage(url, activity, currentTab);
                    } catch (Exception e) {
                        Log.e(TAG, "showSnackbar: Failed to show snackbar: " + e);
                        // If snackbar fails to show, don't switch tabs - let user see the NTP
                    }
                },
                SNACKBAR_SHOW_DELAY_MS); // Wait for view hierarchy to settle as sometimes snackbar
        // pop ups under bottom bar otherwise
    }

    /** Generates a fallback icon and sets it as the profile image on the snackbar. */
    private void setFallbackIcon(GURL url, BraveActivity activity) {
        if (mIconGenerator == null) {
            mIconGenerator = FaviconUtils.createRoundedRectangleIconGenerator(activity);
        }
        Bitmap generatedIcon = mIconGenerator.generateIconForUrl(url.getSpec());
        if (generatedIcon != null) {
            setProfileImageFromBitmap(generatedIcon, activity);
        }
    }

    /** Fetches the favicon for the given URL and sets it as the profile image on the snackbar. */
    private void fetchFaviconAndSetProfileImage(GURL url, BraveActivity activity, Tab currentTab) {
        if (mCurrentSnackbar == null || url == null || url.isEmpty() || activity == null) {
            return;
        }

        if (currentTab == null || currentTab.getWebContents() == null) {
            setFallbackIcon(url, activity);
            return;
        }

        Profile profile = Profile.fromWebContents(currentTab.getWebContents());
        if (profile == null) {
            setFallbackIcon(url, activity);
            return;
        }

        WeakReference<BraveRecentTabsSnackbarHelper> helperRef = new WeakReference<>(this);
        FaviconHelper.FaviconImageCallback imageCallback =
                (bitmap, iconUrl) -> {
                    BraveRecentTabsSnackbarHelper helper = helperRef.get();
                    if (helper == null || helper.mDestroyed || helper.mCurrentSnackbar == null) {
                        return;
                    }

                    // If no favicon found, fallback to generated icon
                    if (bitmap == null) {
                        helper.setFallbackIcon(url, activity);
                        return;
                    }
                    helper.setProfileImageFromBitmap(bitmap, activity);
                };

        // 0 is a max bitmap size for download
        mFaviconHelper.getLocalFaviconImageForURL(profile, url, 0, imageCallback);
    }

    /** Sets the profile image on the snackbar from a bitmap. */
    private void setProfileImageFromBitmap(Bitmap bitmap, BraveActivity activity) {
        if (mCurrentSnackbar == null
                || bitmap == null
                || mSnackbarManager == null
                || mSnackbarController == null) {
            return;
        }

        // Check if snackbar is still showing before updating
        if (!mSnackbarManager.isShowing()) {
            return;
        }

        Drawable drawable = new BitmapDrawable(activity.getResources(), bitmap);
        mCurrentSnackbar.setProfileImage(drawable);

        // If snackbar is already shown, we need to update it by dismissing and re-showing
        // But only if snackbar is still showing and valid
        // IMPORTANT: Set flag to prevent onDismissNoAction from triggering during update
        try {
            mIsUpdatingSnackbar = true;
            mSnackbarManager.dismissSnackbars(mSnackbarController);
            mSnackbarManager.showSnackbar(mCurrentSnackbar);
            // Restore custom text after re-showing
            if (!mSnackbarTitle.isEmpty()) {
                mSnackbarManager.setCustomText(mSnackbarTitle, mSnackbarPageTitle, mSnackbarUrl);
            }
        } finally {
            mIsUpdatingSnackbar = false;
        }
    }

    /**
     * Gets the last opened tab before the app was backgrounded by comparing URLs with the stored
     * BRAVE_LAST_ACTIVE_TAB_URL preference.
     */
    private @Nullable Tab getLastOpenedTab(BraveActivity activity, Tab currentTab) {
        if (activity == null) {
            Log.e(TAG, "getLastOpenedTab: BraveActivity is null");
            return null;
        }

        TabModelSelector selector = activity.getTabModelSelectorSupplier().get();
        if (selector == null) {
            Log.e(TAG, "getLastOpenedTab: TabModelSelector is null");
            return null;
        }

        // Get tabs from regular (non-incognito) model
        TabModel tabModel = selector.getModel(false);
        if (tabModel == null) {
            Log.e(TAG, "getLastOpenedTab: TabModel is null");
            return null;
        }

        int count = tabModel.getCount();

        // Get the last active tab URL from preferences
        String lastActiveTabUrl =
                ChromeSharedPreferences.getInstance()
                        .readString(BravePreferenceKeys.BRAVE_LAST_ACTIVE_TAB_URL, null);

        if (lastActiveTabUrl == null || lastActiveTabUrl.isEmpty()) {
            Log.e(TAG, "getLastOpenedTab: lastActiveTabUrl is null or empty");
            return null;
        }

        // Find tab by URL
        for (int i = 0; i < count; i++) {
            Tab tab = tabModel.getTabAt(i);
            if (tab == null) {
                continue;
            }

            GURL url = tab.getUrl();

            // Skip current tab (NTP)
            if (currentTab != null && tab.getId() == currentTab.getId()) {
                continue;
            }

            if (url != null && url.getSpec().equals(lastActiveTabUrl)) {
                // Found the tab by URL - clear the stored URL after using it
                ChromeSharedPreferences.getInstance()
                        .writeString(BravePreferenceKeys.BRAVE_LAST_ACTIVE_TAB_URL, null);
                return tab;
            }
        }

        // Clear invalid stored URL
        Log.e(TAG, "getLastOpenedTab: No matching tab found, clearing stored URL");
        ChromeSharedPreferences.getInstance()
                .writeString(BravePreferenceKeys.BRAVE_LAST_ACTIVE_TAB_URL, null);
        return null;
    }

    /** Switches to the last tab and dismisses the snackbar. */
    private void switchToLastTabAndDismiss(BraveActivity activity) {
        // Dismiss snackbar first to ensure it's removed before tab switch
        // Use dismissAllSnackbars as a more reliable way to dismiss
        if (mSnackbarManager != null) {
            mSnackbarManager.dismissAllSnackbars();
        }
        // Clear references
        mCurrentSnackbar = null;
        mSnackbarController = null;
        // Cleanup resources
        destroy();
        // Then switch tabs
        switchToLastTab(activity);
    }

    /** Switches to the last tab and closes the current NTP tab. */
    private void switchToLastTab(BraveActivity activity) {
        if (mLastTab == null) {
            Log.e(TAG, "switchToLastTab: mLastTab is null");
            return;
        }

        if (activity == null) {
            Log.e(TAG, "switchToLastTab: BraveActivity is null");
            return;
        }

        TabModelSelector selector = activity.getTabModelSelectorSupplier().get();
        if (selector == null) {
            Log.e(TAG, "switchToLastTab: TabModelSelector is null");
            return;
        }

        TabModel tabModel = selector.getModel(false);
        if (tabModel == null) {
            Log.e(TAG, "switchToLastTab: TabModel is null");
            return;
        }

        // Get current tab (should be NTP) BEFORE switching
        Tab ntpTab = activity.getActivityTab();
        if (ntpTab == null) {
            Log.e(TAG, "switchToLastTab: ntpTab is null");
            return;
        }

        // Store NTP tab ID before switching (since getActivityTab() will change after switch)
        int ntpTabId = ntpTab.getId();
        int lastTabIndex = tabModel.indexOf(mLastTab);

        // Switch to the last tab
        tabModel.setIndex(lastTabIndex, TabSelectionType.FROM_USER);

        // If NTP tab is the same as the last tab, nothing to close
        if (ntpTabId == mLastTab.getId()) {
            return;
        }

        // Close the NTP tab (it might have moved in the model after switching)
        Tab tabToClose = tabModel.getTabById(ntpTabId);
        if (tabToClose != null) {
            // Use TabClosureParams to prevent the "Undo close tab" snackbar
            tabModel.getTabRemover()
                    .closeTabs(
                            TabClosureParams.closeTab(tabToClose).allowUndo(false).build(), false);
        } else {
            Log.e(TAG, "switchToLastTab: tabToClose is null, NTP tab not found");
        }
    }

    /**
     * Cleans up resources held by this helper. Should be called when the helper is no longer
     * needed.
     */
    public void destroy() {
        if (mDestroyed) {
            return;
        }
        mDestroyed = true;
        if (mFaviconHelper != null) {
            mFaviconHelper.destroy();
        }
    }
}
