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
public class PrivacyHubMetricsConnectionErrorHandler implements ConnectionErrorHandler {
    /**
     * This is a delegate that is implemented in the object where the connection is created
     */
    public interface PrivacyHubMetricsConnectionErrorHandlerDelegate {
        default void initPrivacyHubMetrics() {}
        default void cleanUpPrivacyHubMetrics() {}
    }

    private PrivacyHubMetricsConnectionErrorHandlerDelegate
            mPrivacyHubMetricsConnectionErrorHandlerDelegate;
    private static final Object sLock = new Object();
    private static volatile PrivacyHubMetricsConnectionErrorHandler sInstance;

    public static PrivacyHubMetricsConnectionErrorHandler getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new PrivacyHubMetricsConnectionErrorHandler();
            }
        }
        return sInstance;
    }

    public void setDelegate(PrivacyHubMetricsConnectionErrorHandlerDelegate
                    privacyHubMetricsConnectionErrorHandlerDelegate) {
        mPrivacyHubMetricsConnectionErrorHandlerDelegate =
                privacyHubMetricsConnectionErrorHandlerDelegate;
        assert mPrivacyHubMetricsConnectionErrorHandlerDelegate
                != null : "mPrivacyHubMetricsConnectionErrorHandlerDelegate has to be initialized";
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mPrivacyHubMetricsConnectionErrorHandlerDelegate == null) {
            return;
        }

        mPrivacyHubMetricsConnectionErrorHandlerDelegate.cleanUpPrivacyHubMetrics();
        mPrivacyHubMetricsConnectionErrorHandlerDelegate.initPrivacyHubMetrics();
    }
}
