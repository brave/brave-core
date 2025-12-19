/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tor;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;

/**
 * JNI Bridge for Tor service functionality.
 * Connects Java TorService to C++ ProxyConfigServiceTor.
 */
@JNINamespace("tor::android")
@NullMarked
public class TorServiceBridge {
    private static final String TAG = "TorServiceBridge";

    private static TorServiceBridge sInstance;
    private long mNativePtr;

    private TorServiceBridge() {
    }

    /**
     * Get singleton instance
     */
    public static synchronized TorServiceBridge getInstance() {
        if (sInstance == null) {
            sInstance = new TorServiceBridge();
        }
        return sInstance;
    }

    /**
     * Initialize the native Tor service bridge.
     * Should be called when Tor starts.
     */
    public void initialize() {
        if (mNativePtr == 0) {
            mNativePtr = TorServiceBridgeJni.get().init(TorServiceBridge.this);
            Log.i(TAG, "Tor service bridge initialized");
        }
    }

    /**
     * Destroy the native bridge.
     * Should be called when Tor stops.
     */
    public void destroy() {
        if (mNativePtr != 0) {
            TorServiceBridgeJni.get().destroy(mNativePtr);
            mNativePtr = 0;
            Log.i(TAG, "Tor service bridge destroyed");
        }
    }

    /**
     * Update the proxy URI used by Tor profiles.
     * 
     * @param proxyUri The SOCKS5 proxy URI (e.g., "socks5://127.0.0.1:9050")
     */
    public void updateProxyUri(String proxyUri) {
        if (mNativePtr != 0) {
            TorServiceBridgeJni.get().updateProxyUri(mNativePtr, proxyUri);
            Log.d(TAG, "Proxy URI updated: " + proxyUri);
        }
    }

    /**
     * Request a new Tor circuit (new identity).
     * 
     * @param url The URL for which to request a new circuit
     */
    public void setNewTorCircuit(String url) {
        if (mNativePtr != 0) {
            TorServiceBridgeJni.get().setNewTorCircuit(mNativePtr, url);
        }
    }

    /**
     * Check if Tor is currently enabled.
     * 
     * @return true if Tor is enabled
     */
    public boolean isTorEnabled() {
        if (mNativePtr != 0) {
            return TorServiceBridgeJni.get().isTorEnabled(mNativePtr);
        }
        return false;
    }

    /**
     * Called from native when Tor connection state changes.
     */
    @CalledByNative
    private void onTorConnectionStateChanged(int state) {
        Log.d(TAG, "Tor connection state changed: " + state);

        TorService.TorConnectionState connectionState;
        switch (state) {
            case 0:
                connectionState = TorService.TorConnectionState.DISCONNECTED;
                break;
            case 1:
                connectionState = TorService.TorConnectionState.CONNECTING;
                break;
            case 2:
                connectionState = TorService.TorConnectionState.CONNECTED;
                break;
            default:
                connectionState = TorService.TorConnectionState.DISCONNECTED;
        }

        // Notify TorService of state change
        // TorService will propagate this to its listeners
    }

    /**
     * Called from native with log messages from Tor.
     */
    @CalledByNative
    private void onTorLogMessage(String message) {
        Log.d(TAG, "Tor: " + message);
    }

    @NativeMethods
    public interface Natives {
        long init(TorServiceBridge caller);

        void destroy(long nativePtr);

        void updateProxyUri(long nativePtr, String proxyUri);

        void setNewTorCircuit(long nativePtr, String url);

        boolean isTorEnabled(long nativePtr);
    }
}
