/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.misc_metrics;

import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

/**
 * This is a handler for mojo connection failure for Privacy Hub Metrics
 */
public class MiscAndroidMetricsConnectionErrorHandler implements ConnectionErrorHandler {
    /**
     *This is a delegate that is implemented in the object where the connection is created
     */
    public interface MiscAndroidMetricsConnectionErrorHandlerDelegate {
        default void initMiscAndroidMetrics() {}
        default void cleanUpMiscAndroidMetrics() {}
    }

    private MiscAndroidMetricsConnectionErrorHandlerDelegate
            mMiscAndroidMetricsConnectionErrorHandlerDelegate;
    private static final Object sLock = new Object();
    private static MiscAndroidMetricsConnectionErrorHandler sInstance;

    public static MiscAndroidMetricsConnectionErrorHandler getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new MiscAndroidMetricsConnectionErrorHandler();
            }
        }
        return sInstance;
    }

    public void setDelegate(MiscAndroidMetricsConnectionErrorHandlerDelegate
                    privacyHubMetricsConnectionErrorHandlerDelegate) {
        mMiscAndroidMetricsConnectionErrorHandlerDelegate =
                privacyHubMetricsConnectionErrorHandlerDelegate;
        assert mMiscAndroidMetricsConnectionErrorHandlerDelegate
                != null : "mMiscAndroidMetricsConnectionErrorHandlerDelegate has to be initialized";
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mMiscAndroidMetricsConnectionErrorHandlerDelegate == null) {
            return;
        }

        mMiscAndroidMetricsConnectionErrorHandlerDelegate.cleanUpMiscAndroidMetrics();
        mMiscAndroidMetricsConnectionErrorHandlerDelegate.initMiscAndroidMetrics();
    }
}
