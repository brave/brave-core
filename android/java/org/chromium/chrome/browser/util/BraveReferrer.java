/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.RemoteException;

import com.android.installreferrer.api.InstallReferrerClient;
import com.android.installreferrer.api.InstallReferrerClient.InstallReferrerResponse;
import com.android.installreferrer.api.InstallReferrerStateListener;
import com.android.installreferrer.api.ReferrerDetails;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

@JNINamespace("android_brave_referrer")
public class BraveReferrer implements InstallReferrerStateListener {
    private static final String TAG = "BraveReferrer";
    private static final String APP_CHROME_DIR = "app_chrome";
    private static final String PROMO_CODE_FILE_NAME = "promoCode";
    private static final String BRAVE_REFERRER_RECEIVED = "brave_referrer_received";

    private String promoCodeFilePath;
    private InstallReferrerClient referrerClient;

    private long mNativeBraveReferrer;

    private BraveReferrer(long nativeBraveReferrer) {
        mNativeBraveReferrer = nativeBraveReferrer;
    }

    @CalledByNative
    private static BraveReferrer create(long nativeBraveReferrer) {
        return new BraveReferrer(nativeBraveReferrer);
    }

    @CalledByNative
    private void destroy() {
        assert mNativeBraveReferrer != 0;
        mNativeBraveReferrer = 0;
    }

    private class InitReferrerRunnable implements Runnable {
        private Context mContext;
        private BraveReferrer mBraveReferrer;
        public InitReferrerRunnable(Context context, BraveReferrer braveReferrer) {
            mContext = context;
            mBraveReferrer = braveReferrer;
        }

        @Override
        public void run() {
            SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
            if (sharedPref.getBoolean(BRAVE_REFERRER_RECEIVED, false)
                    || !PackageUtils.isFirstInstall(mContext)) {
                onReferrerReady();
                return;
            }
            promoCodeFilePath = mContext.getApplicationInfo().dataDir + File.separator
                    + APP_CHROME_DIR + File.separator + PROMO_CODE_FILE_NAME;
            referrerClient = InstallReferrerClient.newBuilder(mContext).build();
            try {
                referrerClient.startConnection(mBraveReferrer);
            } catch (Exception e) {
                Log.e(TAG, "Unable to start connection for referrer client: " + e);
                onReferrerReady();
            }
        }
    }

    @CalledByNative
    public void initReferrer() {
        // On some devices InstallReferrerClient.startConnection causes file IO,
        // so run it in IO task
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK,
                new InitReferrerRunnable(ContextUtils.getApplicationContext(), this));
    }

    private class SaveReferrerRunnable implements Runnable {
        private String mUrpc;
        public SaveReferrerRunnable(String urpc) {
            mUrpc = urpc;
        }

        @Override
        public void run() {
            FileOutputStream outputStreamWriter = null;
            try {
                File promoCodeFile = new File(promoCodeFilePath);
                outputStreamWriter = new FileOutputStream(promoCodeFile);
                outputStreamWriter.write(mUrpc.getBytes());
            } catch (IOException e) {
                Log.e(TAG,
                        "Could not write to file (" + promoCodeFilePath + "): " + e.getMessage());
            } finally {
                try {
                    if (outputStreamWriter != null) outputStreamWriter.close();
                } catch (IOException exception) {
                }
            }
            onReferrerReady();
        }
    }

    @Override
    public void onInstallReferrerSetupFinished(int responseCode) {
        boolean urpcEmtpy = true;
        switch (responseCode) {
            case InstallReferrerResponse.OK:
                try {
                    ReferrerDetails response = referrerClient.getInstallReferrer();
                    String referrer = response.getInstallReferrer();
                    Uri uri = Uri.parse("http://www.stub.co/?" + referrer);
                    // Get and save user referal program code
                    String urpc = uri.getQueryParameter("urpc");
                    if (urpc != null && !urpc.isEmpty()) {
                        urpcEmtpy = false;
                        PostTask.postTask(
                                TaskTraits.BEST_EFFORT_MAY_BLOCK, new SaveReferrerRunnable(urpc));
                    }
                    referrerClient.endConnection();
                    // Set flag to not repeat this procedure
                    SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
                    SharedPreferences.Editor editor = sharedPref.edit();
                    editor.putBoolean(BRAVE_REFERRER_RECEIVED, true);
                    editor.apply();
                } catch (RemoteException e) {
                    Log.e(TAG, "Could not get referral: " + e.getMessage());
                }
                break;
            case InstallReferrerResponse.FEATURE_NOT_SUPPORTED:
                Log.e(TAG, "API not available on the current Play Store app");
                break;
            case InstallReferrerResponse.SERVICE_UNAVAILABLE:
                Log.e(TAG, "Connection couldn't be established");
                break;
            default:
                Log.e(TAG, "Other error: " + responseCode);
                break;
        }
        if (urpcEmtpy) {
            onReferrerReady();
        }
    }

    @Override
    public void onInstallReferrerServiceDisconnected() {
        onReferrerReady();
        Log.e(TAG, "Install referrer service was disconnected");
    }

    private void onReferrerReady() {
        PostTask.postTask(UiThreadTaskTraits.BEST_EFFORT,
                () -> { BraveReferrerJni.get().onReferrerReady(mNativeBraveReferrer); });
    }

    @NativeMethods
    interface Natives {
        void onReferrerReady(long nativeBraveReferrer);
    }
}
