/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;

import java.time.LocalDateTime;
import java.time.ZoneOffset;

@JNINamespace("chrome::android")
public class BraveSyncWorker {
    private static final String TAG = "SYNC";

    private Context mContext;
    private String mDebug = "true";

    private long mNativeBraveSyncWorker;

    private static BraveSyncWorker sBraveSyncWorker;
    private static boolean sInitialized;

    public static BraveSyncWorker get() {
        if (!sInitialized) {
            sBraveSyncWorker = new BraveSyncWorker();
            sInitialized = true;
        }
        return sBraveSyncWorker;
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveSyncWorker == 0;
        mNativeBraveSyncWorker = nativePtr;
    }

    private void init() {
        if (mNativeBraveSyncWorker == 0) {
            BraveSyncWorkerJni.get().init(BraveSyncWorker.this);
        }
    }

    /**
     * A finalizer is required to ensure that the native object associated with this descriptor gets
     * torn down, otherwise there would be a memory leak.
     */
    @SuppressWarnings("Finalize")
    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveSyncWorker != 0) {
            BraveSyncWorkerJni.get().destroy(mNativeBraveSyncWorker);
            mNativeBraveSyncWorker = 0;
        }
    }

    public BraveSyncWorker() {
        mContext = ContextUtils.getApplicationContext();
        init();
    }

    public String getPureWords() {
        return BraveSyncWorkerJni.get().getSyncCodeWords(mNativeBraveSyncWorker);
    }

    public String getTimeLimitedWordsFromPure(String pureWords) {
        return BraveSyncWorkerJni.get().getTimeLimitedWordsFromPure(pureWords);
    }

    public void saveCodephrase(String codephrase) {
        BraveSyncWorkerJni.get().saveCodeWords(mNativeBraveSyncWorker, codephrase);
    }

    public String getSeedHexFromWords(String codephrase) {
        return BraveSyncWorkerJni.get().getSeedHexFromWords(codephrase);
    }

    public String getWordsFromSeedHex(String seedHex) {
        return BraveSyncWorkerJni.get().getWordsFromSeedHex(seedHex);
    }

    public String getQrDataJson(String seedHex) {
        return BraveSyncWorkerJni.get().getQrDataJson(seedHex);
    }

    public int getQrCodeValidationResult(String jsonQr) {
        return BraveSyncWorkerJni.get().getQrCodeValidationResult(jsonQr);
    }

    public String getSeedHexFromQrJson(String jsonQr) {
        return BraveSyncWorkerJni.get().getSeedHexFromQrJson(jsonQr);
    }

    public int getWordsValidationResult(String timeLimitedWords) {
        return BraveSyncWorkerJni.get().getWordsValidationResult(timeLimitedWords);
    }

    public String getPureWordsFromTimeLimited(String timeLimitedWords) {
        return BraveSyncWorkerJni.get().getPureWordsFromTimeLimited(timeLimitedWords);
    }

    public LocalDateTime getNotAfterFromFromTimeLimitedWords(String timeLimitedWords) {
        long unixTime =
                BraveSyncWorkerJni.get().getNotAfterFromFromTimeLimitedWords(timeLimitedWords);
        LocalDateTime notAfter = LocalDateTime.ofEpochSecond(unixTime, 0, ZoneOffset.UTC);
        return notAfter;
    }

    public String getFormattedTimeDelta(long seconds) {
        return BraveSyncWorkerJni.get().getFormattedTimeDelta(seconds);
    }

    public void requestSync() {
        BraveSyncWorkerJni.get().requestSync(mNativeBraveSyncWorker);
    }

    public boolean isInitialSyncFeatureSetupComplete() {
        return BraveSyncWorkerJni.get().isInitialSyncFeatureSetupComplete(mNativeBraveSyncWorker);
    }

    public void finalizeSyncSetup() {
        BraveSyncWorkerJni.get().finalizeSyncSetup(mNativeBraveSyncWorker);
    }

    public void resetSync() {
        BraveSyncWorkerJni.get().resetSync(mNativeBraveSyncWorker);
    }

    @CalledByNative
    private static void onPermanentlyDeleteAccountResult(Callback<String> callback, String result) {
        callback.onResult(result);
    }

    public void permanentlyDeleteAccount(Callback<String> callback) {
        BraveSyncWorkerJni.get().permanentlyDeleteAccount(mNativeBraveSyncWorker, callback);
    }

    public void clearAccountDeletedNoticePending() {
        BraveSyncWorkerJni.get().clearAccountDeletedNoticePending(mNativeBraveSyncWorker);
    }

    public boolean isAccountDeletedNoticePending() {
        return BraveSyncWorkerJni.get().isAccountDeletedNoticePending(mNativeBraveSyncWorker);
    }

    @CalledByNative
    private static void onJoinSyncChainResult(Callback<Boolean> callback, Boolean result) {
        callback.onResult(result);
    }

    public void setJoinSyncChainCallback(Callback<Boolean> callback) {
        BraveSyncWorkerJni.get().setJoinSyncChainCallback(mNativeBraveSyncWorker, callback);
    }

    public int getWordsCount(String words) {
        return BraveSyncWorkerJni.get().getWordsCount(words);
    }

    @NativeMethods
    interface Natives {
        void init(BraveSyncWorker caller);

        void destroy(long nativeBraveSyncWorker);

        String getSyncCodeWords(long nativeBraveSyncWorker);

        void requestSync(long nativeBraveSyncWorker);

        String getSeedHexFromWords(String passphrase);

        String getWordsFromSeedHex(String seedHex);

        String getQrDataJson(String seedHex);

        int getQrCodeValidationResult(String jsonQr);

        String getSeedHexFromQrJson(String jsonQr);

        int getWordsValidationResult(String timeLimitedWords);

        String getPureWordsFromTimeLimited(String timeLimitedWords);

        String getTimeLimitedWordsFromPure(String pureWords);

        long getNotAfterFromFromTimeLimitedWords(String pureWords);

        String getFormattedTimeDelta(long seconds);

        void saveCodeWords(long nativeBraveSyncWorker, String passphrase);

        int getWordsCount(String words);

        void finalizeSyncSetup(long nativeBraveSyncWorker);

        boolean isInitialSyncFeatureSetupComplete(long nativeBraveSyncWorker);

        void resetSync(long nativeBraveSyncWorker);

        void permanentlyDeleteAccount(long nativeBraveSyncWorker, Callback<String> callback);
        void clearAccountDeletedNoticePending(long nativeBraveSyncWorker);
        boolean isAccountDeletedNoticePending(long nativeBraveSyncWorker);
        void setJoinSyncChainCallback(long nativeBraveSyncWorker, Callback<Boolean> callback);
    }
}
