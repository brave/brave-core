/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.referrer;

import android.content.Context;
import android.net.Uri;
import android.os.RemoteException;

import androidx.annotation.VisibleForTesting;

import com.android.installreferrer.api.InstallReferrerClient;
import com.android.installreferrer.api.InstallReferrerClient.InstallReferrerResponse;
import com.android.installreferrer.api.InstallReferrerStateListener;
import com.android.installreferrer.api.ReferrerDetails;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.PackageUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

@JNINamespace("android_brave_referrer")
public class BraveReferrer implements InstallReferrerStateListener {
    private static final String TAG = "BraveReferrer";
    private static final String APP_CHROME_DIR = "app_chrome";
    private static final String PROMO_CODE_FILE_NAME = "promoCode";
    private static final String BRAVE_REFERRER_RECEIVED = "brave_referrer_received";
    private static final String PLAY_STORE_AD_REFERRAL_CODE = "UAC001";
    private static final String GOOGLE_SEARCH_AD_REFERRAL_CODE = "UAC002";
    private static final String PLAY_STORE_AD_GBRAID_REFERRAL_CODE = "UAC003";
    private static final String PLAY_STORE_AD_GCLID_GBRAID_REFERRAL_CODE = "UAC004";

    private String mPromoCodeFilePath;
    private InstallReferrerClient mReferrerClient;

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
        private final Context mContext;
        private final BraveReferrer mBraveReferrer;

        public InitReferrerRunnable(Context context, BraveReferrer braveReferrer) {
            mContext = context;
            mBraveReferrer = braveReferrer;
        }

        @Override
        public void run() {
            if (ChromeSharedPreferences.getInstance().readBoolean(BRAVE_REFERRER_RECEIVED, false)
                    || !PackageUtils.isFirstInstall(mContext)) {
                onReferrerReady();
                return;
            }
            mPromoCodeFilePath =
                    mContext.getApplicationInfo().dataDir
                            + File.separator
                            + APP_CHROME_DIR
                            + File.separator
                            + PROMO_CODE_FILE_NAME;
            mReferrerClient = InstallReferrerClient.newBuilder(mContext).build();
            try {
                mReferrerClient.startConnection(mBraveReferrer);
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
        private final String mUrpc;

        public SaveReferrerRunnable(String urpc) {
            mUrpc = urpc;
        }

        @Override
        public void run() {
            FileOutputStream outputStreamWriter = null;
            try {
                File promoCodeFile = new File(mPromoCodeFilePath);
                outputStreamWriter = new FileOutputStream(promoCodeFile);
                outputStreamWriter.write(mUrpc.getBytes());
            } catch (IOException e) {
                Log.e(
                        TAG,
                        "Could not write to file (" + mPromoCodeFilePath + "): " + e.getMessage());
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
                    ReferrerDetails response = mReferrerClient.getInstallReferrer();
                    String referrer = response.getInstallReferrer();
                    Uri uri = Uri.parse("http://www.stub.co/?" + referrer);
                    // Get and save user referal program code
                    String urpc = getReferralCode(uri, referrer);
                    if (urpc != null && !urpc.isEmpty()) {
                        urpcEmtpy = false;
                        PostTask.postTask(
                                TaskTraits.BEST_EFFORT_MAY_BLOCK, new SaveReferrerRunnable(urpc));
                    }
                    mReferrerClient.endConnection();
                    // Set flag to not repeat this procedure
                    ChromeSharedPreferences.getInstance()
                            .writeBoolean(BRAVE_REFERRER_RECEIVED, true);
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

    private static boolean isNullOrEmpty(String value) {
        return value == null || value.isEmpty();
    }

    private static String getReferralCode(Uri uri, String referrer) {
        String urpc = uri.getQueryParameter("urpc");
        if (!isNullOrEmpty(urpc)) {
            return urpc;
        }

        String gclid = uri.getQueryParameter("gclid");
        String gbraid = uri.getQueryParameter("gbraid");
        boolean hasGclid = !isNullOrEmpty(gclid);
        boolean hasGbraid = !isNullOrEmpty(gbraid);
        if (hasGclid && hasGbraid) {
            return PLAY_STORE_AD_GCLID_GBRAID_REFERRAL_CODE;
        } else if (hasGbraid) {
            return PLAY_STORE_AD_GBRAID_REFERRAL_CODE;
        } else if (hasGclid) {
            return PLAY_STORE_AD_REFERRAL_CODE;
        }

        if (!isNullOrEmpty(referrer) && referrer.contains("gclid")) {
            // This detection was found empirically. Unfortunately we are not able to get any more
            // info from the referrer at the moment.
            return GOOGLE_SEARCH_AD_REFERRAL_CODE;
        }
        return null;
    }

    @VisibleForTesting
    static String getReferralCodeForTesting(String referrer) {
        if (isNullOrEmpty(referrer)) {
            return null;
        }
        return getReferralCode(Uri.parse("http://www.stub.co/?" + referrer), referrer);
    }

    @Override
    public void onInstallReferrerServiceDisconnected() {
        onReferrerReady();
        Log.e(TAG, "Install referrer service was disconnected");
    }

    private void onReferrerReady() {
        PostTask.postTask(TaskTraits.UI_BEST_EFFORT,
                () -> { BraveReferrerJni.get().onReferrerReady(mNativeBraveReferrer); });
    }

    @NativeMethods
    interface Natives {
        void onReferrerReady(long nativeBraveReferrer);
    }
}
