/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.base;

import android.content.Context;
import android.content.Intent;

import androidx.core.app.JobIntentService;

import org.chromium.base.BundleUtils;

/**
 * JobIntentService base class which will call through to the given {@link Impl}. This class must be
 * present in the base module, while the Impl can be in the chrome module.
 */
public class SplitCompatJobIntentService extends JobIntentService {
    private String mServiceClassName;
    private String mSplitName;
    private Impl mImpl;

    public SplitCompatJobIntentService(String serviceClassName) {
        mServiceClassName = serviceClassName;
    }

    public SplitCompatJobIntentService(String serviceClassName, String splitName) {
        mServiceClassName = serviceClassName;
        mSplitName = splitName;
    }

    @Override
    protected void attachBaseContext(Context context) {
        // Make sure specified split is installed, otherwise fall back to chrome split.
        if (mSplitName != null && BundleUtils.isIsolatedSplitInstalled(context, mSplitName)) {
            context = BundleUtils.createIsolatedSplitContext(context, mSplitName);
        } else {
            context = SplitCompatApplication.createChromeContext(context);
        }
        mImpl = (Impl) BundleUtils.newInstance(context, mServiceClassName);
        mImpl.setService(this);
        super.attachBaseContext(context);
    }

    @Override
    protected void onHandleWork(Intent intent) {
        mImpl.onHandleWork(intent);
    }

    /**
     * Holds the implementation of service logic. Will be called by {@link
     * SplitCompatJobIntentService}.
     */
    public abstract static class Impl {
        private SplitCompatJobIntentService mService;

        private void setService(SplitCompatJobIntentService service) {
            mService = service;
        }

        protected final JobIntentService getService() {
            return mService;
        }

        public static void enqueueWork(Context context, Class<?> cls, int jobId, Intent work) {
            JobIntentService.enqueueWork(context, cls, jobId, work);
        }

        protected abstract void onHandleWork(Intent intent);
    }
}
