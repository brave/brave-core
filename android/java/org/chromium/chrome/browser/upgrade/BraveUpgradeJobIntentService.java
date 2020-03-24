/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import android.content.Context;
import android.content.Intent;
import java.util.Locale;
import android.content.SharedPreferences;
import android.support.v4.app.JobIntentService;

import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.Log;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.settings.website.WebsitePreferenceBridge;
import org.chromium.content_public.browser.BrowserStartupController;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class BraveUpgradeJobIntentService extends JobIntentService {
    private static final String TAG = "BraveUpgradeJobIntentService";
    private static final String SHIELDS_CONFIG_FILENAME = "shields_config.dat";
    private static final String SHIELDS_CONFIG_MIGRATED_FILENAME = "shields_config_migrated.dat";
    private static final int JOB_ID = 1;

    // For old Brave stats
    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";

    // For Desktop always mode
    private static final int CONTENT_SETTINGS_TYPE_DESKTOP_VIEW = 62;
    // For play video in background option
    private static final int CONTENT_SETTINGS_TYPE_PLAY_VIDEO_IN_BACKGROUND = 63;
    // For YT links play video in Brave
    private static final int CONTENT_SETTINGS_TYPE_PLAY_YT_VIDEO_IN_BROWSER = 64;

    // Old search engines settings
    private static final String PREF_STANDARD_SEARCH_ENGINE = "brave_standard_search_engine";
    private static final String PREF_STANDARD_SEARCH_ENGINE_KEYWORD = "brave_standard_search_engine_keyword";
    private static final String PREF_PRIVATE_SEARCH_ENGINE = "brave_private_search_engine";
    private static final String PREF_PRIVATE_SEARCH_ENGINE_KEYWORD = "brave_private_search_engine_keyword";
    private static final String DSE_NAME = "Google";
    private static final String DSE_KEYWORD = "google.com";

    // Old tabs bottom toolbar settings
    private static final String BOTTOM_TOOLBAR_ENABLED_KEY = "bottom_toolbar_enabled";

    // To detect update from tabs
    private static final String PREF_STATS_PREFERENCES_NAME = "StatsPreferences";
    private static final String PREF_WEEK_OF_INSTALLATION_NAME = "WeekOfInstallation";

    public static void startMigrationIfNecessary(Context context) {
        if (BraveUpgradeJobIntentService.needToMigratePreferences()) {
            // Migrate bottom toolbar settings
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY,
                sharedPreferences.getBoolean(BOTTOM_TOOLBAR_ENABLED_KEY, true));
            sharedPreferencesEditor.apply();
        }
        // Start migration in any case as we can have only partial data
        // to migrate available
        BraveUpgradeJobIntentService.enqueueWork(context, new Intent());
    }

    private static boolean needToMigratePreferences() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        boolean migrated = sharedPreferences.getBoolean(BraveHelper.PREF_TABS_SETTINGS_MIGRATED, false);
        if (migrated) {
            // Everything was already migrated
            return false;
        }

        // Detect whether it is update from tabs
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        SharedPreferences prefStatsFromTabs = ContextUtils.getApplicationContext()
                .getSharedPreferences(PREF_STATS_PREFERENCES_NAME, 0);
        boolean updateFormTabs = prefStatsFromTabs.contains(PREF_WEEK_OF_INSTALLATION_NAME);
        if (!updateFormTabs) {
            // We assume that everything was migrated in that case
            sharedPreferencesEditor.putBoolean(BraveHelper.PREF_TABS_SETTINGS_MIGRATED, true);
            sharedPreferencesEditor.apply();

            return false;
        }

        return true;
    }

    private static void enqueueWork(Context context, Intent work) {
        enqueueWork(context, BraveUpgradeJobIntentService.class, JOB_ID, work);
    }

    private byte[] readLocalFile(File path) {
        byte[] buffer = null;

        try {
            if (!path.exists()) {
                return null;
            }
            FileInputStream inputStream = new FileInputStream(path.getAbsolutePath());
            buffer = new byte[inputStream.available()];
            int n = -1;
            int bytesOffset = 0;
            byte[] tempBuffer = new byte[1024];
            while ((n = inputStream.read(tempBuffer)) != -1) {
                System.arraycopy(tempBuffer, 0, buffer, bytesOffset, n);
                bytesOffset += n;
            }
            inputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return buffer;
    }

    private static boolean hasSubdomain(String host) {
        int count = 0;
        for (int i = 0; i < host.length(); ++i) {
            if (host.charAt(i) == '.') {
                ++count;
                if (count >= 2) {
                  return true;
                }
            }
        }
        return false;
    }

    private boolean migrateShieldsConfig() {
        Context context = ContextUtils.getApplicationContext();
        File path = new File(context.getApplicationInfo().dataDir, SHIELDS_CONFIG_FILENAME);
        if (!path.exists()) {
            Log.e(TAG, "Can't locate Brave shields config file: " + path.getPath());
            return false;
        }

        byte buffer[] = null;
        buffer = readLocalFile(path);
        if (buffer == null) {
            Log.e(TAG, "Brave shields config file is empty");
            return false;
        }

        String[] array = new String(buffer).split(";");
        for (int i = 0; i < array.length; ++i) {
            if (array[i].equals("")) {
                continue;
            }

            String host = "";
            String settings = "";
            int splitterIndex = array[i].indexOf(",");
            if (splitterIndex == -1) {
                continue;
            }

            host = array[i].substring(0, splitterIndex);
            if (array[i].length() > splitterIndex + 1) {
                settings = array[i].substring(splitterIndex + 1);
            }
            if (host.length() == 0 || settings.length() == 0) {
                continue;
            }

            // The shields_config.dat file doesn't include the scheme,
            // but it's required when setting shields preferences
            migrateShieldsSettingsForHost("http://" + host, settings);
            migrateShieldsSettingsForHost("https://" + host, settings);

            // The shields_config.dat file strips "www." from
            // hostnames, so include it back (with scheme!) to avoid
            // missing sites
            if (!hasSubdomain(host)) {
                migrateShieldsSettingsForHost("http://www." + host, settings);
                migrateShieldsSettingsForHost("https://www." + host, settings);
            }
        }

        return true;
    }

    private boolean migrateShieldsSettingsForHost(String host, String settings) {
        String[] array = new String(settings).split(",");
        if (array.length < 6) {
            Log.e(TAG, "Brave shields host settings invalid for host " + host);
            return false;
        }

        Profile profile = Profile.getLastUsedProfile();
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS, array[0].equals("1"),
                false);
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS_TRACKERS, array[1].equals("1"),
                false);
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES,
                array[2].equals("1"), false);
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS, array[3].equals("1"),
                false);
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES, array[4].equals("1"),
                false);
        BraveShieldsContentSettings.setShields(profile, host,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                array[5].equals("1"), false);

        return true;
    }

    private boolean migrateTotalStatsAndPreferences() {
        boolean needToRestart = false;
        if (!BraveUpgradeJobIntentService.needToMigratePreferences()) {
            return needToRestart;
        }
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        // Total stats migration
        long trackersBlockedCount = sharedPreferences.getLong(PREF_TRACKERS_BLOCKED_COUNT, 0);
        long adsBlockedCount = sharedPreferences.getLong(PREF_ADS_BLOCKED_COUNT, 0);
        long httpsUpgradesCount = sharedPreferences.getLong(PREF_HTTPS_UPGRADES_COUNT, 0);
        Profile profile = Profile.getLastUsedProfile();
        if (trackersBlockedCount > 0) {
            BravePrefServiceBridge.getInstance().setOldTrackersBlockedCount(profile, trackersBlockedCount);
        }
        if (adsBlockedCount > 0) {
            BravePrefServiceBridge.getInstance().setOldAdsBlockedCount(profile, adsBlockedCount);
        }
        if (httpsUpgradesCount > 0) {
            BravePrefServiceBridge.getInstance().setOldHttpsUpgradesCount(profile, httpsUpgradesCount);
        }
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_TRACKERS_BLOCKED_COUNT, 0);
        sharedPreferencesEditor.putLong(PREF_ADS_BLOCKED_COUNT, 0);
        sharedPreferencesEditor.putLong(PREF_HTTPS_UPGRADES_COUNT, 0);

        // Background video playback migration
        if (BravePrefServiceBridge.getInstance().getBackgroundVideoPlaybackEnabled() != BravePrefServiceBridge.getInstance()
                .GetBooleanForContentSetting(CONTENT_SETTINGS_TYPE_PLAY_VIDEO_IN_BACKGROUND)) {
            needToRestart = true;
            BravePrefServiceBridge.getInstance().setBackgroundVideoPlaybackEnabled(BravePrefServiceBridge.getInstance()
                    .GetBooleanForContentSetting(CONTENT_SETTINGS_TYPE_PLAY_VIDEO_IN_BACKGROUND));
        }
        // Play YT links in Brave option
        BravePrefServiceBridge.getInstance().setPlayYTVideoInBrowserEnabled(
            BravePrefServiceBridge.getInstance().GetBooleanForContentSetting(CONTENT_SETTINGS_TYPE_PLAY_YT_VIDEO_IN_BROWSER));
        // View pages in Desktop mode option
        BravePrefServiceBridge.getInstance().setDesktopModeEnabled(
            BravePrefServiceBridge.getInstance().GetBooleanForContentSetting(CONTENT_SETTINGS_TYPE_DESKTOP_VIEW));

        // Background image settings settings
        if (sharedPreferences.contains(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES)) {
            BravePrefServiceBridge.getInstance().setBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE,
                    sharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true));
            BravePrefServiceBridge.getInstance().setBoolean(BravePref.NTP_SHOW_BRANDED_BACKGROUND_IMAGE,
                    sharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_SPONSORED_IMAGES, true));
        }

        // Migrate search engines settings
        sharedPreferencesEditor.putString(BraveHelper.PRIVATE_DSE_SHORTNAME,
            sharedPreferences.getString(PREF_PRIVATE_SEARCH_ENGINE, DSE_NAME));
        sharedPreferencesEditor.putString(BraveHelper.STANDARD_DSE_SHORTNAME,
            sharedPreferences.getString(PREF_STANDARD_SEARCH_ENGINE, DSE_NAME));

        sharedPreferencesEditor.putBoolean(BraveHelper.PREF_TABS_SETTINGS_MIGRATED, true);
        sharedPreferencesEditor.apply();
        return needToRestart;
    }

    private void removeNTPFiles() {
        String countryCode = Locale.getDefault().getCountry();

        File dataDirPath = new File(PathUtils.getDataDirectory());
        if (null == dataDirPath) {
            return;
        }
        File[] fileList = dataDirPath.listFiles();

        for (File file : fileList) {
            String filePath = file.getPath();
            String sFileName = filePath.substring(filePath.lastIndexOf(File.separator)+1);
            if (sFileName.startsWith(countryCode + "_")) {
                file.delete();
            }
        }
    }

    @Override
    protected void onHandleWork(Intent intent) {
        // Kick off the migration task only after the browser has
        // completed startup, as migration requires a profile.
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                    .addStartupCompletedObserver(new BrowserStartupController.StartupCallback() {
                        @Override
                        public void onSuccess() {
                            try {
                                removeNTPFiles();
                            } catch(Exception exc) {
                                Log.e("NTP", "On app upgrade : " + exc.getMessage());
                            }
                            boolean needToRestart = migrateTotalStatsAndPreferences();
                            if (!migrateShieldsConfig()) {
                                Log.e(TAG, "Failed to migrate Brave shields config settings");
                                finalizeMigration(needToRestart);
                                return;
                            }

                            Log.i(TAG, "Successfully migrated Brave shields config settings");

                            // Rename the migrated file so we don't
                            // try to import it again
                            String dataDir = ContextUtils.getApplicationContext()
                                                     .getApplicationInfo()
                                                     .dataDir;
                            File oldName = new File(dataDir, SHIELDS_CONFIG_FILENAME);
                            File migratedName = new File(dataDir, SHIELDS_CONFIG_MIGRATED_FILENAME);
                            if (!oldName.renameTo(migratedName)) {
                                Log.e(TAG, "Failed to rename migrated Brave shields config file");
                                finalizeMigration(needToRestart);
                                return;
                            }
                            finalizeMigration(needToRestart);
                        }

                        @Override
                        public void onFailure() {
                            Log.e(TAG,
                                    "Failed to migrate Brave shields config settings: "
                                            + "BrowserStartupController.StartupCallback failed");
                        }
                    });
        });
    }

    private void finalizeMigration(boolean needToRestart) {
        if (!needToRestart) return;
        // Restart the app to apply migrated options
        PostTask.postTask(UiThreadTaskTraits.DEFAULT, () -> {
            BraveRelaunchUtils.restart();
        });
    }
}
