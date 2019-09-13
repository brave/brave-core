/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import org.chromium.base.ThreadUtils;

public class BravePrefServiceBridge extends PrefServiceBridge {
    private BravePrefServiceBridge() {
        super();
    }

    private static BravePrefServiceBridge sInstance;

    public static BravePrefServiceBridge getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new BravePrefServiceBridge();
        }
        return sInstance;
    }

    /**
     * @param whether HTTPSE should be enabled.
     */
    public void setHTTPSEEnabled(boolean enabled) {
        nativeSetHTTPSEEnabled(enabled);
    }

    /**
     * @param whether AdBlock should be enabled.
     */
    public void setAdBlockEnabled(boolean enabled) {
        nativeSetAdBlockEnabled(enabled);
    }

    /**
     * @param whether Fingerprinting Protection should be enabled.
     */
    public void setFingerprintingProtectionEnabled(boolean enabled) {
        nativeSetFingerprintingProtectionEnabled(enabled);
    }

    private native void nativeSetHTTPSEEnabled(boolean enabled);
    private native void nativeSetAdBlockEnabled(boolean enabled);
    private native void nativeSetFingerprintingProtectionEnabled(boolean enabled);
}
