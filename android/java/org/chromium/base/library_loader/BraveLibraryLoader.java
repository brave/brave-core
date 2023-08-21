/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base.library_loader;

import android.annotation.SuppressLint;
import android.content.pm.ApplicationInfo;
import android.os.Build;

import androidx.annotation.RequiresApi;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.BundleUtils;
import org.chromium.build.NativeLibraries;

import java.util.Arrays;

/**
 * Class to override upstream's LibraryLoader.loadWithSystemLinkerAlreadyLocked
 * See also BraveLibraryLoaderClassAdapter
 */
public class BraveLibraryLoader extends LibraryLoader {
    @VisibleForTesting
    public BraveLibraryLoader() {
        super();
    }

    public static LibraryLoader sInstance = new BraveLibraryLoader();
    public static LibraryLoader getInstance() {
        return sInstance;
    }

    public void preloadAlreadyLocked(String packageName, boolean inZygote) {
        assert false : "preloadAlreadyLocked should be redirected to parent in bytecode!";
    }

    public void loadWithSystemLinkerAlreadyLocked(ApplicationInfo appInfo, boolean inZygote) {
        setEnvForNative();
        preloadAlreadyLocked(appInfo.packageName, inZygote);
        loadLibsfromSplits(appInfo, inZygote);
    }

    @SuppressLint({"UnsafeDynamicallyLoadedCode", "NewApi"})
    private void loadLibsfromSplits(ApplicationInfo appInfo, boolean inZygote) {
        assert !inZygote || android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q;

        String splitName = getSplitNameByCurrentAbi();
        if (inZygote && android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            String apkPath = getSplitApkPath(appInfo, splitName);
            for (String library : NativeLibraries.LIBRARIES) {
                System.load(getNativeLibraryPath(library, apkPath, appInfo));
            }
        } else {
            for (String library : NativeLibraries.LIBRARIES) {
                System.load(BundleUtils.getNativeLibraryPath(library, splitName));
            }
        }
    }

    private String getSplitNameByCurrentAbi() {
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
    private static String getSplitApkPath(ApplicationInfo appInfo, String splitName) {
        String[] splitNames = appInfo.splitNames;
        if (splitNames == null) {
            return null;
        }
        int idx = Arrays.binarySearch(splitNames, splitName);
        return idx < 0 ? null : appInfo.splitSourceDirs[idx];
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    private String getNativeLibraryPath(
            String libraryName, String apkPath, ApplicationInfo appInfo) {
        try {
            String primaryCpuAbi =
                    (String) appInfo.getClass().getField("primaryCpuAbi").get(appInfo);
            return apkPath + "!/lib/" + primaryCpuAbi + "/" + System.mapLibraryName(libraryName);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
