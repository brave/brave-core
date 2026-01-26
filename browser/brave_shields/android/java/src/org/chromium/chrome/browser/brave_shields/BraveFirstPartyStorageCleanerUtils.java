/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_shields;

import android.app.ActivityManager;
import android.content.Context;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;

import java.util.List;

@JNINamespace("brave_shields")
@NullMarked
public class BraveFirstPartyStorageCleanerUtils {
    private static final String TAG = "FPSCleanerUtils";

    public static void cleanupTLDFirstPartyStorage(Tab tab) {
        BraveFirstPartyStorageCleanerUtilsJni.get().cleanupTLDFirstPartyStorage(tab);
    }

    public static void triggerCurrentAppStateNotification(Profile profile) {
        BraveFirstPartyStorageCleanerUtilsJni.get().triggerCurrentAppStateNotification(profile);
    }

    @CalledByNative
    public static boolean isAppInTaskStack(String packageName) {
        try {
            ActivityManager am =
                    (ActivityManager)
                            ContextUtils.getApplicationContext()
                                    .getSystemService(Context.ACTIVITY_SERVICE);

            List<ActivityManager.RunningTaskInfo> tasks = am.getRunningTasks(10);
            for (ActivityManager.RunningTaskInfo task : tasks) {
                if (task.baseActivity != null
                        && packageName.equals(task.baseActivity.getPackageName())) {
                    return true;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to check task stack", e);
        }
        return false;
    }

    @CalledByNative
    public static String getCurrentPackageName() {
        return ContextUtils.getApplicationContext().getPackageName();
    }

    @NativeMethods
    interface Natives {
        void cleanupTLDFirstPartyStorage(Tab tab);

        void triggerCurrentAppStateNotification(Profile profile);
    }
}
