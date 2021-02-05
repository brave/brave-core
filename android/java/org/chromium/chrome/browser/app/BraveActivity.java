/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app;

import android.app.Activity;
import android.app.AlertDialog;
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
import android.text.TextUtils;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import org.json.JSONException;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ApplicationLifetime;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveSyncReflectionUtils;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.CrossPromotionalModalDialogFragment;
import org.chromium.chrome.browser.DeprecateBAPModalDialogFragment;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.SetDefaultBrowserActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.dependency_injection.ChromeActivityComponent;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.informers.BraveAndroidSyncDisabledInformer;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.onboarding.OnboardingActivity;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.P3aOnboardingActivity;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.rate.RateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.settings.BraveRewardsPreferences;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.share.ShareDelegateImpl.ShareOrigin;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayout;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.chrome.browser.util.BraveReferrer;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.widget.Toast;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity<C extends ChromeActivityComponent>
        extends ChromeActivity implements BraveRewardsObserver {
    public static final int SITE_BANNER_REQUEST_CODE = 33;
    public static final int VERIFY_WALLET_ACTIVITY_REQUEST_CODE = 34;
    public static final int USER_WALLET_ACTIVITY_REQUEST_CODE = 35;
    public static final String ADD_FUNDS_URL = "chrome://rewards/#add-funds";
    public static final String REWARDS_SETTINGS_URL = "chrome://rewards/";
    public static final String BRAVE_REWARDS_SETTINGS_URL = "brave://rewards/";
    public static final String REWARDS_AC_SETTINGS_URL = "chrome://rewards/contribute";
    public static final String REWARDS_LEARN_MORE_URL = "https://brave.com/faq-rewards/#unclaimed-funds";
    public static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";
    public static final String P3A_URL = "https://brave.com/p3a";
    public static final String BRAVE_PRIVACY_POLICY = "https://brave.com/privacy/#rewards";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    public static final String OPEN_URL = "open_url";

    public static final String BRAVE_PRODUCTION_PACKAGE_NAME = "com.brave.browser";
    public static final String BRAVE_BETA_PACKAGE_NAME = "com.brave.browser_beta";
    public static final String BRAVE_NIGHTLY_PACKAGE_NAME = "com.brave.browser_nightly";

    private static final int DAYS_3 = 3;
    private static final int DAYS_4 = 4;
    private static final int DAYS_5 = 5;
    private static final int DAYS_12 = 12;

    /**
     * Settings for sending local notification reminders.
     */
    public static final String CHANNEL_ID = "com.brave.browser";
    public static final String ANDROID_SETUPWIZARD_PACKAGE_NAME = "com.google.android.setupwizard";
    public static final String ANDROID_PACKAGE_NAME = "android";
    public static final String BRAVE_BLOG_URL = "http://www.brave.com/blog";

    private static final String JAPAN_COUNTRY_CODE = "JP";

    // Explicitly declare this variable to avoid build errors.
    // It will be removed in asm and parent variable will be used instead.
    protected ObservableSupplier<Profile> mTabModelProfileSupplier;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private static final List<String> yandexRegions =
            Arrays.asList("AM", "AZ", "BY", "KG", "KZ", "MD", "RU", "TJ", "TM", "UZ");

    public BraveActivity() {
        // Disable key checker to avoid asserts on Brave keys in debug
        SharedPreferencesManager.getInstance().disableKeyCheckerForTesting();
    }

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        nativeRestartStatsUpdater();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            if (mBraveRewardsNativeWorker == null)
                mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
            mBraveRewardsNativeWorker.AddObserver(this);
        }
    }

    @Override
    public void onPauseWithNative() {
        super.onPauseWithNative();
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        final TabImpl currentTab = (TabImpl) getActivityTab();
        // Handle items replaced by Brave.
        if (id == R.id.info_menu_id && currentTab != null) {
            ShareDelegate shareDelegate = (ShareDelegate) getShareDelegateSupplier().get();
            shareDelegate.share(currentTab, false, ShareOrigin.OVERFLOW_MENU);
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

        if (BraveRewardsHelper.hasRewardsEnvChange()) {
            BravePrefServiceBridge.getInstance().resetPromotionLastFetchStamp();
            BraveRewardsHelper.setRewardsEnvChange(false);
        }

        int appOpenCount = SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);
        SharedPreferencesManager.getInstance().writeInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT, appOpenCount + 1);

        if (PackageUtils.isFirstInstall(this) && appOpenCount == 0) {
            checkForYandexSE();
        }

        //set bg ads to off for existing and new installations
        setBgBraveAdsDefaultOff();

        Context app = ContextUtils.getApplicationContext();
        if (null != app
                && BraveReflectionUtil.EqualTypes(this.getClass(), ChromeTabbedActivity.class)) {
            // Trigger BraveSyncWorker CTOR to make migration from sync v1 if sync is enabled
            BraveSyncReflectionUtils.getSyncWorker();
        }

        checkForNotificationData();

        if (!RateUtils.getInstance(this).getPrefRateEnabled()) {
            RateUtils.getInstance(this).setPrefRateEnabled(true);
            RateUtils.getInstance(this).setNextRateDateAndCount();
        }

        if (RateUtils.getInstance(this).shouldShowRateDialog())
            showBraveRateDialog();

        // TODO commenting out below code as we may use it in next release

        // if (PackageUtils.isFirstInstall(this)
        //         &&
        //         SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
        //         == 1) {
        //     Calendar calender = Calendar.getInstance();
        //     calender.setTime(new Date());
        //     calender.add(Calendar.DATE, DAYS_4);
        //     OnboardingPrefManager.getInstance().setNextOnboardingDate(
        //         calender.getTimeInMillis());
        // }

        // OnboardingActivity onboardingActivity = null;
        // for (Activity ref : ApplicationStatus.getRunningActivities()) {
        //     if (!(ref instanceof OnboardingActivity)) continue;

        //     onboardingActivity = (OnboardingActivity) ref;
        // }

        // if (onboardingActivity == null
        //         && OnboardingPrefManager.getInstance().showOnboardingForSkip(this)) {
        //     OnboardingPrefManager.getInstance().showOnboarding(this);
        //     OnboardingPrefManager.getInstance().setOnboardingShownForSkip(true);
        // }

        if (SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT) == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_12);
            OnboardingPrefManager.getInstance().setNextCrossPromoModalDate(
                calender.getTimeInMillis());
        }

        if (OnboardingPrefManager.getInstance().showCrossPromoModal()) {
            showCrossPromotionalDialog();
            OnboardingPrefManager.getInstance().setCrossPromoModalShown(true);
        }
        BraveSyncReflectionUtils.showInformers();
        BraveAndroidSyncDisabledInformer.showInformers();

        if (!PackageUtils.isFirstInstall(this)
                && !OnboardingPrefManager.getInstance().isP3AEnabledForExistingUsers()) {
            BravePrefServiceBridge.getInstance().setP3AEnabled(true);
            OnboardingPrefManager.getInstance().setP3AEnabledForExistingUsers(true);
        }

        if (BraveConfig.P3A_ENABLED
                && !OnboardingPrefManager.getInstance().isP3aOnboardingShown()) {
            Intent p3aOnboardingIntent = new Intent(this, P3aOnboardingActivity.class);
            p3aOnboardingIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(p3aOnboardingIntent);
        }

        if (!OnboardingPrefManager.getInstance().isOneTimeNotificationStarted()
                && PackageUtils.isFirstInstall(this)) {
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_3);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_24);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_6);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_10);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_30);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_35);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_1);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_2);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_3);
            OnboardingPrefManager.getInstance().setOneTimeNotificationStarted(true);
        }
        if (!TextUtils.isEmpty(BinanceWidgetManager.getInstance().getBinanceAccountBalance())) {
            try {
                BinanceWidgetManager.binanceAccountBalance = new BinanceAccountBalance(
                        BinanceWidgetManager.getInstance().getBinanceAccountBalance());
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
        }

        if (PackageUtils.isFirstInstall(this)
                && SharedPreferencesManager.getInstance().readInt(
                           BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_4);
            BraveRewardsHelper.setNextRewardsOnboardingModalDate(calender.getTimeInMillis());
        }
        if (BraveRewardsHelper.shouldShowRewardsOnboardingModalOnDay4()) {
            BraveRewardsHelper.setShowBraveRewardsOnboardingModal(true);
            openRewardsPanel();
            BraveRewardsHelper.setRewardsOnboardingModalShown(true);
        }

        if (SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_5);
            OnboardingPrefManager.getInstance().setNextSetDefaultBrowserModalDate(
                    calender.getTimeInMillis());
        }
        checkSetDefaultBrowserModal();
        if (mBraveRewardsNativeWorker != null
                && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            mBraveRewardsNativeWorker.StartProcess();
        }
    }

    @Override
    public void OnRewardsParameters(int errorCode) {
        if (errorCode == BraveRewardsNativeWorker.LEDGER_OK && mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.GetWalletBalance() != null
                && mBraveRewardsNativeWorker.GetWalletBalance().getTotal() > 0) {
            checkForDeprecateBAPDialog();
        }
    }

    @Override
    public void OnStartProcess() {
        mBraveRewardsNativeWorker.GetRewardsParameters();
    }

    private void checkForDeprecateBAPDialog() {
        String countryCode = Locale.getDefault().getCountry();
        if (countryCode.equals(JAPAN_COUNTRY_CODE) && !isRewardsPanelOpened()
                && System.currentTimeMillis() > BraveRewardsHelper.getNextBAPModalDate()) {
            Calendar toDayCalendar = Calendar.getInstance();
            Date todayDate = toDayCalendar.getTime();

            Calendar march6Calendar = Calendar.getInstance();
            march6Calendar.set(Calendar.DAY_OF_MONTH, 6);
            march6Calendar.set(Calendar.YEAR, 2021);
            march6Calendar.set(Calendar.MONTH, 2);
            march6Calendar.set(Calendar.HOUR_OF_DAY, 0);
            march6Calendar.set(Calendar.MINUTE, 0);
            march6Calendar.set(Calendar.SECOND, 0);
            march6Calendar.set(Calendar.MILLISECOND, 0);
            Date march6Date = march6Calendar.getTime();

            Calendar march13Calendar = Calendar.getInstance();
            march13Calendar.set(Calendar.DAY_OF_MONTH, 13);
            march13Calendar.set(Calendar.YEAR, 2021);
            march13Calendar.set(Calendar.MONTH, 2);
            march13Calendar.set(Calendar.HOUR_OF_DAY, 0);
            march13Calendar.set(Calendar.MINUTE, 0);
            march13Calendar.set(Calendar.SECOND, 0);
            march13Calendar.set(Calendar.MILLISECOND, 0);
            Date march13Date = march13Calendar.getTime();

            boolean shouldSetNextDate = false;
            if (todayDate.compareTo(march6Date) < 0) {
                showRewardsTooltip();
                shouldSetNextDate = true;
            } else if (todayDate.compareTo(march6Date) > 0
                    && todayDate.compareTo(march13Date) < 0) {
                showDeprecateBAPDialog();
                shouldSetNextDate = true;
            }
            if (shouldSetNextDate) {
                Calendar calender = toDayCalendar;
                calender.add(Calendar.DATE, DAYS_3);
                BraveRewardsHelper.setNextBAPModalDate(calender.getTimeInMillis());
            }
        }
    }

    private void checkSetDefaultBrowserModal() {
        boolean shouldShowDefaultBrowserModal =
                (OnboardingPrefManager.getInstance().getNextSetDefaultBrowserModalDate() > 0
                        && System.currentTimeMillis()
                                > OnboardingPrefManager.getInstance()
                                          .getNextSetDefaultBrowserModalDate());
        boolean shouldShowDefaultBrowserModalAfterP3A =
                OnboardingPrefManager.getInstance().shouldShowDefaultBrowserModalAfterP3A();
        if (!BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(this)
                && (shouldShowDefaultBrowserModalAfterP3A || shouldShowDefaultBrowserModal)) {
            Intent setDefaultBrowserIntent = new Intent(this, SetDefaultBrowserActivity.class);
            setDefaultBrowserIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(setDefaultBrowserIntent);
            if (shouldShowDefaultBrowserModal) {
                OnboardingPrefManager.getInstance().setNextSetDefaultBrowserModalDate(0);
            }
            if (shouldShowDefaultBrowserModalAfterP3A) {
                OnboardingPrefManager.getInstance().setShowDefaultBrowserModalAfterP3A(false);
            }
        }
    }

    private void checkForYandexSE() {
        String countryCode = Locale.getDefault().getCountry();
        if (yandexRegions.contains(countryCode)) {
            TemplateUrl yandexTemplateUrl =
                    BraveSearchEngineUtils.getTemplateUrlByShortName(OnboardingPrefManager.YANDEX);
            if (yandexTemplateUrl != null) {
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, false);
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, true);
            }
        }
    }

    private void checkForNotificationData() {
        Intent notifIntent = getIntent();
        if (notifIntent != null && notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE) != null) {
            Log.e("NTP", notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE));
            String notificationType = notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
            switch (notificationType) {
            case RetentionNotificationUtil.HOUR_3:
            case RetentionNotificationUtil.HOUR_24:
            case RetentionNotificationUtil.EVERY_SUNDAY:
                checkForBraveStats();
                break;
            case RetentionNotificationUtil.DAY_6:
            case RetentionNotificationUtil.BRAVE_STATS_ADS_TRACKERS:
            case RetentionNotificationUtil.BRAVE_STATS_DATA:
            case RetentionNotificationUtil.BRAVE_STATS_TIME:
                if (getActivityTab() != null
                    && getActivityTab().getUrlString() != null
                    && !UrlUtilities.isNTPUrl(getActivityTab().getUrlString())) {
                    getTabCreator(false).launchUrl(UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                }
                break;
            case RetentionNotificationUtil.DAY_10:
            case RetentionNotificationUtil.DAY_30:
            case RetentionNotificationUtil.DAY_35:
                openRewardsPanel();
                break;
            }
        }
    }

    public void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            if (getActivityTab() != null && getActivityTab().getUrlString() != null
                    && !UrlUtilities.isNTPUrl(getActivityTab().getUrlString())) {
                OnboardingPrefManager.getInstance().setFromNotification(true);
                if (getTabCreator(false) != null) {
                    getTabCreator(false).launchUrl(
                            UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                }
            } else {
                showOnboardingV2(false);
            }
        }
    }

    public void showOnboardingV2(boolean fromStats) {
        try {
            OnboardingPrefManager.getInstance().setNewOnboardingShown(true);
            FragmentManager fm = getSupportFragmentManager();
            HighlightDialogFragment fragment = (HighlightDialogFragment) fm
                                               .findFragmentByTag(HighlightDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = fm.beginTransaction();

            if (fragment != null) {
                transaction.remove(fragment);
            }

            fragment = new HighlightDialogFragment();
            Bundle fragmentBundle = new Bundle();
            fragmentBundle.putBoolean(OnboardingPrefManager.FROM_STATS, fromStats);
            fragment.setArguments(fragmentBundle);
            transaction.add(fragment, HighlightDialogFragment.TAG_FRAGMENT);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("HighlightDialogFragment", e.getMessage());
        }
    }

    public void hideRewardsOnboardingIcon() {
        BraveToolbarLayout layout = (BraveToolbarLayout)findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.hideRewardsOnboardingIcon();
        }
    }

    public void showRewardsTooltip() {
        BraveToolbarLayout layout = (BraveToolbarLayout) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.showRewardsTooltip();
        }
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
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
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

    public void handleBraveSetDefaultBrowserDialog() {
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

                Toast toast = new Toast(context, layout);
                toast.setDuration(Toast.LENGTH_LONG);
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

    public void dismissRewardsPanel() {
        BraveToolbarLayout layout = (BraveToolbarLayout)findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.dismissRewardsPanel();
        }
    }

    public void dismissShieldsTooltip() {
        BraveToolbarLayout layout = (BraveToolbarLayout)findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.dismissShieldsTooltip();
        }
    }

    public void openRewardsPanel() {
        BraveToolbarLayout layout = (BraveToolbarLayout)findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            layout.openRewardsPanel();
        }
    }

    public boolean isRewardsPanelOpened() {
        BraveToolbarLayout layout = (BraveToolbarLayout) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            return layout.isRewardsPanelOpened();
        }
        return false;
    }

    public boolean isShieldsTooltipShown() {
        BraveToolbarLayout layout = (BraveToolbarLayout) findViewById(R.id.toolbar);
        assert layout != null;
        if (layout != null) {
            return layout.isShieldsTooltipShown();
        }
        return false;
    }

    public Tab selectExistingTab(String url) {
        Tab tab = getActivityTab();
        if (tab != null && tab.getUrlString().equals(url)) {
            return tab;
        }

        TabModel tabModel = getCurrentTabModel();
        int tabIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        // Find if tab exists
        if (tabIndex != TabModel.INVALID_TAB_INDEX) {
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

    private void showBraveRateDialog() {
        RateDialogFragment mRateDialogFragment = new RateDialogFragment();
        mRateDialogFragment.setCancelable(false);
        mRateDialogFragment.show(getSupportFragmentManager(), "RateDialogFragment");
    }

    private void showCrossPromotionalDialog() {
        CrossPromotionalModalDialogFragment mCrossPromotionalModalDialogFragment = new CrossPromotionalModalDialogFragment();
        mCrossPromotionalModalDialogFragment.setCancelable(false);
        mCrossPromotionalModalDialogFragment.show(getSupportFragmentManager(), "CrossPromotionalModalDialogFragment");
    }

    public void showDeprecateBAPDialog() {
        DeprecateBAPModalDialogFragment mDeprecateBAPModalDialogFragment =
                new DeprecateBAPModalDialogFragment();
        mDeprecateBAPModalDialogFragment.setCancelable(false);
        mDeprecateBAPModalDialogFragment.show(
                getSupportFragmentManager(), "DeprecateBAPModalDialogFragment");
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

    @Override
    public void onActivityResult (int requestCode, int resultCode,
                                  Intent data) {
        if (resultCode == RESULT_OK &&
                (requestCode == VERIFY_WALLET_ACTIVITY_REQUEST_CODE ||
                 requestCode == USER_WALLET_ACTIVITY_REQUEST_CODE ||
                 requestCode == SITE_BANNER_REQUEST_CODE) ) {
            dismissRewardsPanel();
            String open_url = data.getStringExtra(BraveActivity.OPEN_URL);
            if (! TextUtils.isEmpty(open_url)) {
                openNewOrSelectExistingTab(open_url);
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    /**
     * Disable background ads on Android. Issue #8641.
     */
    private void setBgBraveAdsDefaultOff() {
        SharedPreferences sharedPreferences =
            ContextUtils.getAppSharedPreferences();
        boolean exists = sharedPreferences.contains(
                             BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET);
        if (!exists) {
            SharedPreferences.Editor sharedPreferencesEditor =
                sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(
                BraveRewardsPreferences.PREF_ADS_SWITCH, false);
            sharedPreferencesEditor.putBoolean(
                BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET, true);
            sharedPreferencesEditor.apply();
        }
    }

    @Override
    public void performPreInflationStartup() {
        BraveDbUtil dbUtil = BraveDbUtil.getInstance();
        if (dbUtil.dbOperationRequested()) {
            AlertDialog dialog = new AlertDialog.Builder(this)
            .setMessage(dbUtil.performDbExportOnStart() ? "Exporting database, please wait..."
                        : "Importing database, please wait...")
            .setCancelable(false)
            .create();
            dialog.setCanceledOnTouchOutside(false);
            if (dbUtil.performDbExportOnStart()) {
                dbUtil.setPerformDbExportOnStart(false);
                dbUtil.ExportRewardsDb(dialog);
            } else if (dbUtil.performDbImportOnStart() && !dbUtil.dbImportFile().isEmpty()) {
                dbUtil.setPerformDbImportOnStart(false);
                dbUtil.ImportRewardsDb(dialog, dbUtil.dbImportFile());
            }
            dbUtil.cleanUpDbOperationRequest();
        }
        super.performPreInflationStartup();
    }

    @Override
    protected @LaunchIntentDispatcher.Action int maybeDispatchLaunchIntent(
        Intent intent, Bundle savedInstanceState) {
        boolean notificationUpdate = IntentUtils.safeGetBooleanExtra(
                                         intent, BravePreferenceKeys.BRAVE_UPDATE_EXTRA_PARAM, false);
        if (notificationUpdate) {
            SetUpdatePreferences();
        }

        return super.maybeDispatchLaunchIntent(intent, savedInstanceState);
    }

    private void SetUpdatePreferences() {
        Calendar currentTime = Calendar.getInstance();
        long milliSeconds = currentTime.getTimeInMillis();

        SharedPreferences sharedPref =
            getApplicationContext().getSharedPreferences(
                BravePreferenceKeys.BRAVE_NOTIFICATION_PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();

        editor.putLong(BravePreferenceKeys.BRAVE_MILLISECONDS_NAME, milliSeconds);
        editor.apply();
    }
}
