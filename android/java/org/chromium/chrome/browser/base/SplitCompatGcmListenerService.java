/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.base;

import android.content.Context;
import android.os.Bundle;

import org.chromium.base.BundleUtils;

public class SplitCompatGcmListenerService {
    public SplitCompatGcmListenerService(String serviceClassName) {}

    public abstract static class Impl {
        private SplitCompatGcmListenerService mService;

        protected final void setService(SplitCompatGcmListenerService service) {
            mService = service;
        }

        protected final SplitCompatGcmListenerService getService() {
            return mService;
        }

        public void onCreate() {}

        public void onMessageReceived(String from, Bundle data) {}

        public void onMessageSent(String msgId) {}

        public void onSendError(String msgId, Exception error) {}

        public void onDeletedMessages() {}

        public void onNewToken(String token) {}
    }
}
