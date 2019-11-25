/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import android.content.Context;
import android.content.Intent;
import android.support.v4.app.JobIntentService;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.task.PostTask;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
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

    public static void startMigrationIfNecessary(Context context) {
        // If the Brave shields config file exists, migrate it
        File path = new File(context.getApplicationInfo().dataDir, SHIELDS_CONFIG_FILENAME);
        if (path.exists()) {
            BraveUpgradeJobIntentService.enqueueWork(context, new Intent());
        }
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

    @Override
    protected void onHandleWork(Intent intent) {
        // Kick off the migration task only after the browser has
        // completed startup, as migration requires a profile.
        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
            BrowserStartupController.get(LibraryProcessType.PROCESS_BROWSER)
                    .addStartupCompletedObserver(new BrowserStartupController.StartupCallback() {
                        @Override
                        public void onSuccess() {
                            if (!migrateShieldsConfig()) {
                                Log.e(TAG, "Failed to migrate Brave shields config settings");
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
                                return;
                            }
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
}
