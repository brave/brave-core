/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.onboarding.OnboardingActivity;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayout;
import org.chromium.chrome.browser.util.BraveReferrer;
import org.chromium.chrome.browser.util.UrlConstants;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.ui.widget.Toast;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity extends ChromeActivity {
    public static final int SITE_BANNER_REQUEST_CODE = 33;
    public static final String ADD_FUNDS_URL = "chrome://rewards/#add-funds";
    public static final String REWARDS_SETTINGS_URL = "chrome://rewards/";
    public static final String REWARDS_AC_SETTINGS_URL = "chrome://rewards/contribute";
    public static final String REWARDS_LEARN_MORE_URL = "https://brave.com/faq-rewards/#unclaimed-funds";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";

    /**
     * Settings for sending local notification reminders.
     */
    public static final String CHANNEL_ID = "com.brave.browser";
    public static final String ANDROID_SETUPWIZARD_PACKAGE_NAME = "com.google.android.setupwizard";
    public static final String ANDROID_PACKAGE_NAME = "android";
    public static final String BRAVE_BLOG_URL = "http://www.brave.com/blog";

    // Sync worker
    public BraveSyncWorker mBraveSyncWorker;

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        nativeRestartStatsUpdater();
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();

        // Disable NTP suggestions
        PrefServiceBridge.getInstance().setBoolean(Pref.NTP_ARTICLES_SECTION_ENABLED, false);
        PrefServiceBridge.getInstance().setBoolean(Pref.NTP_ARTICLES_LIST_VISIBLE, false);
    }

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        final TabImpl currentTab = (TabImpl) getActivityTab();
        // Handle items replaced by Brave.
        if (id == R.id.info_menu_id && currentTab != null) {
            ShareDelegate shareDelegate = (ShareDelegate) getShareDelegateSupplier().get();
            shareDelegate.share(currentTab, false);
            return true;
        }

        if (super.onMenuOrKeyboardAction(id, fromMenu)) {
            return true;
        }

        // Handle items added by Brave.
        if (currentTab == null) {
            return false;
        } else if (id == R.id.exit_id) {
            ApplicationLifetime.terminate(false);
        } else if (id == R.id.set_default_browser) {
            handleBraveSetDefaultBrowserDialog();
        } else if (id == R.id.brave_rewards_id) {
            openNewOrSelectExistingTab(REWARDS_SETTINGS_URL);
        } else {
            return false;
        }

        return true;
    }

    @Override
    public void initializeState() {
        super.initializeState();
        if (isNoRestoreState()) {
            CommandLine.getInstance().appendSwitch(ChromeSwitches.NO_RESTORE_STATE);
        }

        BraveSearchEngineUtils.initializeBraveSearchEngineStates(getTabModelSelector());
    }

    @Override
    public void onResume() {
        super.onResume();

        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set proper active DSE whenever brave returns to foreground.
        // If active tab is private, set private DSE as an active DSE.
        BraveSearchEngineUtils.updateActiveDSE(tab.isIncognito());
    }

    @Override
    public void onPause() {
        super.onPause();

        Tab tab = getActivityTab();
        if (tab == null)
            return;

        // Set normal DSE as an active DSE when brave goes in background
        // because currently set DSE is used by outside of brave(ex, brave search widget).
        if (tab.isIncognito()) {
            BraveSearchEngineUtils.updateActiveDSE(false);
        }
    }

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        BraveReferrer.getInstance().initReferrer(this);
        createNotificationChannel();
        setupBraveSetDefaultBrowserNotification();
    }

    @Override
    protected void initializeStartupMetrics() {
        super.initializeStartupMetrics();

        // Disable FRE for arm64 builds where ChromeActivity is the one that
        // triggers FRE instead of ChromeLauncherActivity on arm32 build.
        BraveHelper.DisableFREDRP();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        int appOpenCount = ContextUtils.getAppSharedPreferences().getInt(BackgroundImagesPreferences.PREF_APP_OPEN_COUNT, 0);
        BackgroundImagesPreferences.setOnPreferenceValue(BackgroundImagesPreferences.PREF_APP_OPEN_COUNT , appOpenCount + 1);

        Context app = ContextUtils.getApplicationContext();
        if (null != app && (this instanceof ChromeTabbedActivity)) {
            mBraveSyncWorker = new BraveSyncWorker(app);
        }

        OnboardingActivity onboardingActivity = null;
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof OnboardingActivity)) continue;

            onboardingActivity = (OnboardingActivity) ref;
        }

        if (onboardingActivity == null) {
            OnboardingPrefManager.getInstance().showOnboarding(this, false);
        }
    }

    @Override
    public void addOrEditBookmark(final Tab tabToBookmark) {
        long tempBookmarkId = BookmarkBridge.getUserBookmarkIdForTab(tabToBookmark);
        final boolean bCreateBookmark = (BookmarkId.INVALID_ID == tempBookmarkId);

        super.addOrEditBookmark(tabToBookmark);

        final long bookmarkId = BookmarkBridge.getUserBookmarkIdForTab(tabToBookmark);
        final BookmarkModel bookmarkModel = new BookmarkModel();

        bookmarkModel.finishLoadingBookmarkModel(() -> {
            // Gives up the bookmarking if the tab is being destroyed.
            BookmarkId newBookmarkId = new BookmarkId(bookmarkId, BookmarkType.NORMAL);
            if (!((TabImpl)tabToBookmark).isClosing() && ((TabImpl)tabToBookmark).isInitialized()) {
                if (null != mBraveSyncWorker && null != newBookmarkId) {
                    mBraveSyncWorker.CreateUpdateBookmark(bCreateBookmark, bookmarkModel.getBookmarkById(newBookmarkId));
                    bookmarkModel.destroy();
                }
            }
            bookmarkModel.destroy();
        });
    }

    private void createNotificationChannel() {
        Context context = ContextUtils.getApplicationContext();
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = "Brave Browser";
            String description = "Notification channel for Brave Browser";
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
            channel.setDescription(description);
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    private void setupBraveSetDefaultBrowserNotification() {
        // Post task to IO thread because isBraveSetAsDefaultBrowser may cause
        // sqlite file IO operation underneath
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () ->
        {
            Context context = ContextUtils.getApplicationContext();
            if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)) {
                // Don't ask again
                return;
            }
            Intent intent = new Intent(context, BraveSetDefaultBrowserNotificationService.class);
            context.sendBroadcast(intent);
        });
    }

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    private void handleBraveSetDefaultBrowserDialog() {
        /* (Albert Wang): Default app settings didn't get added until API 24
         * https://developer.android.com/reference/android/provider/Settings#ACTION_MANAGE_DEFAULT_APPS_SETTINGS
         */
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        boolean supportsDefault = Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
        ResolveInfo resolveInfo = getPackageManager().resolveActivity(
                browserIntent, supportsDefault ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        Context context = ContextUtils.getApplicationContext();
        if (BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)) {
            Toast toast = Toast.makeText(
                    context, R.string.brave_already_set_as_default_browser, Toast.LENGTH_LONG);
            toast.show();
            return;
        }
        if (supportsDefault) {
            if (resolveInfo.activityInfo.packageName.equals(ANDROID_SETUPWIZARD_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(ANDROID_PACKAGE_NAME)) {
                LayoutInflater inflater = getLayoutInflater();
                View layout = inflater.inflate(R.layout.brave_set_default_browser_dialog,
                        (ViewGroup) findViewById(R.id.brave_set_default_browser_toast_container));

                Toast toast = new Toast(context);
                toast.setDuration(Toast.LENGTH_LONG);
                toast.setView(layout);
                toast.setGravity(Gravity.TOP, 0, 40);
                toast.show();
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(BRAVE_BLOG_URL));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Intent intent = new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            }
        } else {
            if (resolveInfo.activityInfo.packageName.equals(ANDROID_SETUPWIZARD_PACKAGE_NAME)
                    || resolveInfo.activityInfo.packageName.equals(ANDROID_PACKAGE_NAME)) {
                // (Albert Wang): From what I've experimented on 6.0,
                // default browser popup is in the middle of the screen for
                // these versions. So we shouldn't show the toast.
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(BRAVE_BLOG_URL));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(intent);
            } else {
                Toast toast = Toast.makeText(
                        context, R.string.brave_default_browser_go_to_settings, Toast.LENGTH_LONG);
                toast.show();
                return;
            }
        }
    }

    public void OnRewardsPanelDismiss() {
        BraveToolbarLayout layout = (BraveToolbarLayout)findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.onRewardsPanelDismiss();
        }
    }

    public Tab selectExistingTab(String url) {
        Tab tab = getActivityTab();
        if (tab != null && tab.getUrl().equals(url)) {
            return tab;
        }

        TabModel tabModel = getCurrentTabModel();
        int tabIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        // Find if tab exists
        if (tabIndex != TabModel.INVALID_TAB_INDEX){
            tab = tabModel.getTabAt(tabIndex);
            // Moving tab forward
            tabModel.moveTab(tab.getId(), tabModel.getCount());
            tabModel.setIndex(
                    TabModelUtils.getTabIndexById(tabModel, tab.getId()),
                    TabSelectionType.FROM_USER);
            return tab;
        } else {
            return null;
        }
    }

    public Tab openNewOrSelectExistingTab(String url) {
        TabModel tabModel = getCurrentTabModel();
        int tabRewardsIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        Tab tab = selectExistingTab(url);
        if (tab != null) {
            return tab;
        } else { // Open a new tab
            return getTabCreator(false).launchUrl(url, TabLaunchType.FROM_CHROME_UI);
        }
    }

    private native void nativeRestartStatsUpdater();

    static public ChromeTabbedActivity getChromeTabbedActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof ChromeTabbedActivity)) continue;

            return (ChromeTabbedActivity)ref;
        }

        return null;
    }

    static public BraveActivity getBraveActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof BraveActivity)) continue;

            return (BraveActivity)ref;
        }

        return null;
    }
}
