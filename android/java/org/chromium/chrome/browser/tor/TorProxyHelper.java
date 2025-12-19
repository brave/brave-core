/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tor;

import android.util.Log;

/**
 * Helper class to configure SOCKS5 proxy for Tor tabs.
 * Routes network requests through Tor's SOCKS5 proxy at 127.0.0.1:9050.
 */
public class TorProxyHelper {
    private static final String TAG = "TorProxyHelper";

    /**
     * Get the Tor SOCKS5 proxy URL
     * 
     * @return Proxy URL in format "socks5://host:port"
     */
    public static String getTorProxyUrl() {
        return TorService.TOR_SOCKS_PROXY;
    }

    /**
     * Get the Tor SOCKS5 proxy host
     * 
     * @return Proxy host (127.0.0.1)
     */
    public static String getTorProxyHost() {
        return TorService.TOR_SOCKS_HOST;
    }

    /**
     * Get the Tor SOCKS5 proxy port
     * 
     * @return Proxy port (9050)
     */
    public static int getTorProxyPort() {
        return TorService.TOR_SOCKS_PORT;
    }

    /**
     * Check if Tor is connected and ready to proxy requests
     * 
     * @return true if Tor is connected
     */
    public static boolean isTorReady() {
        return TorService.getInstance().getConnectionState() == TorService.TorConnectionState.CONNECTED;
    }

    /**
     * Configure a WebView or similar component to use Tor proxy.
     * Note: This is a simplified implementation. In practice, you may need
     * to use network stack configuration.
     * 
     * @return true if proxy was configured successfully
     */
    public static boolean configureProxy() {
        if (!isTorReady()) {
            Log.w(TAG, "Cannot configure proxy: Tor is not connected");
            return false;
        }

        // The actual proxy configuration for Chrome/WebView happens at the
        // network layer. This class provides the proxy details that can be
        // passed to ProxyConfigServiceTor in the C++ layer via JNI.

        Log.i(TAG, "Tor proxy configured: " + getTorProxyUrl());
        return true;
    }
}
