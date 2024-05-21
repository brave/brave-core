/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.multiwindow;

import android.app.Activity;
import android.content.Intent;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

import java.util.List;

public class BraveMultiWindowUtils extends MultiWindowUtils {

    private static final String TAG = "MultiWindowUtils";

    public BraveMultiWindowUtils() {
        super();
    }

    public boolean shouldShowEnableWindow(Activity activity) {
        return super.isOpenInOtherWindowSupported(activity)
                || super.canEnterMultiWindowMode(activity);
    }

    public static boolean shouldShowManageWindowsMenu() {
        return shouldEnableMultiWindows() && MultiWindowUtils.shouldShowManageWindowsMenu();
    }

    @Override
    public boolean isOpenInOtherWindowSupported(Activity activity) {
        return shouldEnableMultiWindows() && super.isOpenInOtherWindowSupported(activity);
    }

    @Override
    public boolean isMoveToOtherWindowSupported(
            Activity activity, TabModelSelector tabModelSelector) {
        return shouldEnableMultiWindows()
                && super.isMoveToOtherWindowSupported(activity, tabModelSelector);
    }

    @Override
    public boolean canEnterMultiWindowMode(Activity activity) {
        return shouldEnableMultiWindows() && super.canEnterMultiWindowMode(activity);
    }

    public static boolean shouldEnableMultiWindows() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.ENABLE_MULTI_WINDOWS, false);
    }

    public static void updateEnableMultiWindows(boolean isEnable) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.ENABLE_MULTI_WINDOWS, isEnable);
    }

    public static boolean isCheckUpgradeEnableMultiWindows() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.ENABLE_MULTI_WINDOWS_UPGRADE, false);
    }

    public static void setCheckUpgradeEnableMultiWindows(boolean isUpgradeCheck) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.ENABLE_MULTI_WINDOWS_UPGRADE, isUpgradeCheck);
    }

    public static void mergeWindows(Activity activity) {
        try {
            MultiInstanceManager multiInstanceManager =
                    BraveActivity.getBraveActivityFromTaskId(activity.getTaskId())
                            .getMultiInstanceManager();
            if (multiInstanceManager != null) {
                multiInstanceManager.handleMenuOrKeyboardAction(
                        org.chromium.chrome.R.id.move_to_other_window_menu_id, false);
                if (activity != null) {
                    Intent intent = new Intent(activity, ChromeTabbedActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
                    intent.setAction(Intent.ACTION_VIEW);
                    activity.startActivity(intent);
                }
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "get BraveActivity exception", e);
        }
    }

    public static void closeWindows() {
        try {
            MultiInstanceManager multiInstanceManager =
                    BraveActivity.getBraveActivity().getMultiInstanceManager();
            if (multiInstanceManager != null) {
                if (multiInstanceManager instanceof MultiInstanceManagerApi31) {
                    MultiInstanceManagerApi31 multiInstanceManagerApi31 =
                            ((MultiInstanceManagerApi31) multiInstanceManager);
                    List<InstanceInfo> allInstances = multiInstanceManagerApi31.getInstanceInfo();
                    if (allInstances != null && allInstances.size() > 1) {
                        for (int i = 1; i < allInstances.size(); i++) {
                            multiInstanceManagerApi31.closeInstance(
                                    allInstances.get(i).instanceId, allInstances.get(i).taskId);
                        }
                    }
                }
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "get BraveActivity exception", e);
        }
    }
}
