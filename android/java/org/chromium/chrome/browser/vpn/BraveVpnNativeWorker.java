/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.os.Environment;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Semaphore;

@JNINamespace("chrome::android")
public class BraveVpnNativeWorker {
    private long mNativeBraveVpnNativeWorker;
    private static final Object mLock = new Object();
    private static BraveVpnNativeWorker mInstance;

    public static Semaphore mutex;

    private List<BraveVpnObserver> mObservers;

    // private String finalResponse = "";
    public static ArrayList<byte[]> tempStorage = new ArrayList<byte[]>();
    public static ByteArrayOutputStream output;
    // public static int currentOffset;
    public static CompletableFuture<Long> contentLength = new CompletableFuture<>();
    public static CompletableFuture<Integer> responseLength = new CompletableFuture<>();
    public static int readLength;
    public static CompletableFuture<String> url = new CompletableFuture<>();
    // public static String url;
    // public static long contentLength;
    public static String itemId = "";
    public static byte[] finalData;
    public static PipedInputStream pipedInputStream;
    private PipedOutputStream pipedOutputStream;
    // private static File file =
    //         new
    //         File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
    //                 "index.mp4");
    // private static FileOutputStream fos;

    public static BraveVpnNativeWorker getInstance() {
        synchronized (mLock) {
            if (mInstance == null) {
                mInstance = new BraveVpnNativeWorker();
                mInstance.init();
            }
        }
        mutex = new Semaphore(1);
        return mInstance;
    }

    private BraveVpnNativeWorker() {
        mObservers = new ArrayList<BraveVpnObserver>();
    }

    private void init() {
        if (mNativeBraveVpnNativeWorker == 0) {
            BraveVpnNativeWorkerJni.get().init(this);
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveVpnNativeWorker != 0) {
            BraveVpnNativeWorkerJni.get().destroy(mNativeBraveVpnNativeWorker, this);
            mNativeBraveVpnNativeWorker = 0;
        }
    }

    public void addObserver(BraveVpnObserver observer) {
        synchronized (mLock) {
            mObservers.add(observer);
        }
    }

    public void removeObserver(BraveVpnObserver observer) {
        synchronized (mLock) {
            mObservers.remove(observer);
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveVpnNativeWorker == 0;
        mNativeBraveVpnNativeWorker = nativePtr;
    }

    @CalledByNative
    public void onGetAllServerRegions(String jsonServerRegions, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetAllServerRegions(jsonServerRegions, isSuccess);
        }
    }

    @CalledByNative
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetTimezonesForRegions(jsonTimezones, isSuccess);
        }
    }

    @CalledByNative
    public void onGetHostnamesForRegion(String jsonHostnames, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetHostnamesForRegion(jsonHostnames, isSuccess);
        }
    }

    @CalledByNative
    public void onGetWireguardProfileCredentials(
            String jsonWireguardProfileCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetWireguardProfileCredentials(jsonWireguardProfileCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyCredentials(String jsonVerifyCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onVerifyCredentials(jsonVerifyCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onInvalidateCredentials(String jsonInvalidateCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onInvalidateCredentials(jsonInvalidateCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetSubscriberCredential(subscriberCredential, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onVerifyPurchaseToken(jsonResponse, isSuccess);
        }
    }

    @CalledByNative
    public void onResponseStarted(String url, long contentLength) {
        Log.e("data_source", "onResponseStarted : " + url);
        // if (output != null) {
        //     try {
        //         output.reset();
        //     } catch (Exception e) {
        //     }
        // } else {
        //     output = new ByteArrayOutputStream();
        // }
        // try {
        //     pipedInputStream = new PipedInputStream();
        //     pipedOutputStream = new PipedOutputStream(pipedInputStream);
        // } catch (IOException e) {
        //     e.printStackTrace();
        // }
        // this.url = url;
        // this.contentLength = contentLength;
        // this.url.complete(url);
        // this.contentLength.complete(contentLength);
        for (BraveVpnObserver observer : mObservers) {
            observer.onResponseStarted(url, contentLength);
        }
        // mutex.release();
        Log.e("data_source", "release mutex : ");
    }

    @CalledByNative
    synchronized public void onDataReceived(byte[] response) {
        Log.e("data_source", "onDataReceived : response : " + response.length);
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK,
                ()
                        -> {
                                // try {
                                //     output.write(response);
                                //     if (output.toByteArray().length > readLength) {
                                //         responseLength.complete(output.toByteArray().length);
                                //     }
                                //     // tempStorage.add(response);
                                // } catch (Exception e) {
                                //     Log.e("data_source", e.getMessage());
                                // }
                                // try {
                                //     pipedOutputStream.write(response);
                                // } catch (IOException e) {
                                //     e.printStackTrace();
                                // }
                        });
        for (BraveVpnObserver observer : mObservers) {
            observer.onDataReceived(response);
        }
    }

    @CalledByNative
    public void onDataCompleted() {
        Log.e("data_source", "onDataCompleted : file.getAbsolutePath() : ");
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            try {
                // finalData = output.toByteArray();
                // writeToFile(finalData);
                // output.close();
            } catch (Exception e) {
                Log.e("data_source", e.getMessage());
            }
        });
        for (BraveVpnObserver observer : mObservers) {
            observer.onDataCompleted();
        }
    }

    public void writeToFile(byte[] data) throws IOException {
        File outputDir = ContextUtils.getApplicationContext().getCacheDir();
        File outputFile = File.createTempFile("file", ".temp", outputDir);
        Log.e("data_source", "File location : " + outputFile.getAbsolutePath());
        FileOutputStream out = new FileOutputStream(new File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "index.mp4"));
        out.write(data);
        out.close();
    }

    synchronized public byte[] getLatestData() {
        return output.toByteArray();
    }

    public void getAllServerRegions() {
        BraveVpnNativeWorkerJni.get().getAllServerRegions(mNativeBraveVpnNativeWorker);
    }

    public void getTimezonesForRegions() {
        BraveVpnNativeWorkerJni.get().getTimezonesForRegions(mNativeBraveVpnNativeWorker);
    }

    public void getHostnamesForRegion(String region) {
        BraveVpnNativeWorkerJni.get().getHostnamesForRegion(mNativeBraveVpnNativeWorker, region);
    }

    public void getWireguardProfileCredentials(
            String subscriberCredential, String publicKey, String hostname) {
        BraveVpnNativeWorkerJni.get().getWireguardProfileCredentials(
                mNativeBraveVpnNativeWorker, subscriberCredential, publicKey, hostname);
    }

    public void verifyCredentials(
            String hostname, String clientId, String subscriberCredential, String apiAuthToken) {
        BraveVpnNativeWorkerJni.get().verifyCredentials(mNativeBraveVpnNativeWorker, hostname,
                clientId, subscriberCredential, apiAuthToken);
    }

    public void invalidateCredentials(
            String hostname, String clientId, String subscriberCredential, String apiAuthToken) {
        BraveVpnNativeWorkerJni.get().invalidateCredentials(mNativeBraveVpnNativeWorker, hostname,
                clientId, subscriberCredential, apiAuthToken);
    }

    public void getSubscriberCredential(String productType, String productId,
            String validationMethod, String purchaseToken, String packageName) {
        BraveVpnNativeWorkerJni.get().getSubscriberCredential(mNativeBraveVpnNativeWorker,
                productType, productId, validationMethod, purchaseToken, packageName);
    }

    public void verifyPurchaseToken(
            String purchaseToken, String productId, String productType, String packageName) {
        BraveVpnNativeWorkerJni.get().verifyPurchaseToken(
                mNativeBraveVpnNativeWorker, purchaseToken, productId, productType, packageName);
    }

    // Desktop purchase methods
    public void reloadPurchasedState() {
        BraveVpnNativeWorkerJni.get().reloadPurchasedState(mNativeBraveVpnNativeWorker);
    }

    public boolean isPurchasedUser() {
        return BraveVpnNativeWorkerJni.get().isPurchasedUser(mNativeBraveVpnNativeWorker);
    }

    public void getSubscriberCredentialV12() {
        BraveVpnNativeWorkerJni.get().getSubscriberCredentialV12(mNativeBraveVpnNativeWorker);
    }

    public void reportBackgroundP3A(long sessionStartTimeMs, long sessionEndTimeMs) {
        BraveVpnNativeWorkerJni.get().reportBackgroundP3A(
                mNativeBraveVpnNativeWorker, sessionStartTimeMs, sessionEndTimeMs);
    }

    public void reportForegroundP3A() {
        BraveVpnNativeWorkerJni.get().reportForegroundP3A(mNativeBraveVpnNativeWorker);
    }

    public void queryPrompt(String url, String method) {
        Log.e("custom", "queryPrompt : ");
        BraveVpnNativeWorkerJni.get().queryPrompt(mNativeBraveVpnNativeWorker, url, method);
    }

    @NativeMethods
    interface Natives {
        void init(BraveVpnNativeWorker caller);
        void destroy(long nativeBraveVpnNativeWorker, BraveVpnNativeWorker caller);
        void getAllServerRegions(long nativeBraveVpnNativeWorker);
        void getTimezonesForRegions(long nativeBraveVpnNativeWorker);
        void getHostnamesForRegion(long nativeBraveVpnNativeWorker, String region);
        void getWireguardProfileCredentials(long nativeBraveVpnNativeWorker,
                String subscriberCredential, String publicKey, String hostname);
        void verifyCredentials(long nativeBraveVpnNativeWorker, String hostname, String clientId,
                String subscriberCredential, String apiAuthToken);
        void invalidateCredentials(long nativeBraveVpnNativeWorker, String hostname,
                String clientId, String subscriberCredential, String apiAuthToken);
        void getSubscriberCredential(long nativeBraveVpnNativeWorker, String productType,
                String productId, String validationMethod, String purchaseToken,
                String packageName);
        void verifyPurchaseToken(long nativeBraveVpnNativeWorker, String purchaseToken,
                String productId, String productType, String packageName);
        void reloadPurchasedState(long nativeBraveVpnNativeWorker);
        boolean isPurchasedUser(long nativeBraveVpnNativeWorker);
        void getSubscriberCredentialV12(long nativeBraveVpnNativeWorker);
        void reportBackgroundP3A(
                long nativeBraveVpnNativeWorker, long sessionStartTimeMs, long sessionEndTimeMs);
        void reportForegroundP3A(long nativeBraveVpnNativeWorker);
        void queryPrompt(long nativeBraveVpnNativeWorker, String url, String method);
    }
}
