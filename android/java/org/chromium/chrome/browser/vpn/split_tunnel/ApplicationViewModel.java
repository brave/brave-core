/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.split_tunnel;

import android.Manifest;
import android.app.Activity;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ApplicationViewModel extends ViewModel {
    MutableLiveData<List<ApplicationDataModel>> excludedApplicationDataLiveData;
    MutableLiveData<List<ApplicationDataModel>> applicationDataLiveData;
    MutableLiveData<List<ApplicationDataModel>> systemApplicationDataLiveData;

    public ApplicationViewModel() {
        excludedApplicationDataLiveData = new MutableLiveData<>();
        applicationDataLiveData = new MutableLiveData<>();
        systemApplicationDataLiveData = new MutableLiveData<>();
    }

    public MutableLiveData<List<ApplicationDataModel>> getExcludedApplicationDataLiveData() {
        return excludedApplicationDataLiveData;
    }

    public MutableLiveData<List<ApplicationDataModel>> getApplicationDataMutableLiveData() {
        return applicationDataLiveData;
    }
    public MutableLiveData<List<ApplicationDataModel>> getSystemApplicationDataMutableLiveData() {
        return systemApplicationDataLiveData;
    }

    public void getApplications(Activity activity) {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        executor.execute(() -> {
            List<ApplicationDataModel> excludedApplicationDataModels = new ArrayList<>();
            List<ApplicationDataModel> applicationDataModels = new ArrayList<>();
            List<ApplicationDataModel> systemApplicationDataModels = new ArrayList<>();

            Set<String> excludedPackages = BraveVpnPrefUtils.getExcludedPackages();
            PackageManager pm = activity.getPackageManager();
            List<PackageInfo> packageInfos =
                    activity.getPackageManager().getPackagesHoldingPermissions(
                            new String[] {Manifest.permission.INTERNET},
                            PackageManager.GET_META_DATA);
            for (PackageInfo packageInfo : packageInfos) {
                String packageName = packageInfo.packageName;
                ApplicationInfo appInfo = packageInfo.applicationInfo;
                ApplicationDataModel applicationDataModel = new ApplicationDataModel(
                        appInfo.loadIcon(pm), appInfo.loadLabel(pm).toString(), packageName,
                        isSystemApp(appInfo));
                if (excludedPackages != null && excludedPackages.contains(appInfo.packageName)) {
                    excludedApplicationDataModels.add(applicationDataModel);
                } else if (isSystemApp(appInfo)) {
                    systemApplicationDataModels.add(applicationDataModel);
                } else {
                    applicationDataModels.add(applicationDataModel);
                }
            }
            excludedApplicationDataLiveData.postValue(excludedApplicationDataModels);
            systemApplicationDataLiveData.postValue(systemApplicationDataModels);
            applicationDataLiveData.postValue(applicationDataModels);
        });
    }

    private boolean isSystemApp(ApplicationInfo applicationInfo) {
        return (applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0;
    }
}
