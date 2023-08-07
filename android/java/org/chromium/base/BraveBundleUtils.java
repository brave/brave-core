/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import android.annotation.SuppressLint;
import android.content.pm.ApplicationInfo;
import android.os.Build;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import org.chromium.base.annotations.CalledByNative;

import java.util.Arrays;

/**
 * Brave's helpers for BundleUtils
 */
public class BraveBundleUtils {
    public static String getSplitSuffixByCurrentAbi() {
        assert Build.SUPPORTED_ABIS.length > 0;
        String currentAbi = Build.SUPPORTED_ABIS[0];
        switch (currentAbi) {
            case "arm64-v8a":
                return "config.arm64_v8a";
            case "armeabi-v7a":
                return "config.armeabi_v7a";
            case "x86_64":
                return "config.x86_64";
            case "x86":
                return "config.x86";
            default:
                assert false;
                return "config.arm64_v8a";
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public static Boolean doesSplitExist(String splitName) {
        ApplicationInfo appInfo = ContextUtils.getApplicationContext().getApplicationInfo();
        if (appInfo.splitNames == null) {
            return false;
        }
        int idx = Arrays.binarySearch(appInfo.splitNames, splitName);
        return idx >= 0;
    }

    /* Returns absolute path to a native library in a feature module. */
    @CalledByNative
    @Nullable
    @SuppressLint({"NewApi"})
    public static String getNativeLibraryPathTrySplitAbi(String libraryName, String splitName) {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.O) {
            // Fallback, as BundleUtils.getSplitApkPath works starting from Oreo
            return BundleUtils.getNativeLibraryPath(libraryName, splitName);
        }

        // When bundle is built with abi split dimension, there is a separate split apk
        // with a native lib, and if it is available, we must load library from there.
        // Adjust split name with abi name and check whether it exists.
        // This is required at least for VR module, but may also be required for others.
        String splitNameWithAbi = splitName + "." + getSplitSuffixByCurrentAbi();
        if (doesSplitExist(splitNameWithAbi)) {
            return BundleUtils.getNativeLibraryPath(libraryName, splitNameWithAbi);
        } else {
            return BundleUtils.getNativeLibraryPath(libraryName, splitName);
        }
    }
}
