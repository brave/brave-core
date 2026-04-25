/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.base;

import android.content.Context;
import android.os.CancellationSignal;
import android.service.autofill.AutofillService;
import android.service.autofill.FillCallback;
import android.service.autofill.FillRequest;
import android.service.autofill.SaveCallback;
import android.service.autofill.SaveRequest;

import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;

/**
 * AutofillService base class which will call through to the given {@link Impl}. This class must be
 * present in the base module, while the Impl can be in the chrome module.
 */
@NullMarked
public class SplitCompatAutofillService extends AutofillService {
    private final String mServiceClassName;
    private Impl mImpl;

    public SplitCompatAutofillService(String serviceClassName) {
        mServiceClassName = serviceClassName;
    }

    @Override
    protected void attachBaseContext(Context baseContext) {
        mImpl =
                (Impl)
                        SplitCompatUtils.loadClassAndAdjustContextChrome(
                                baseContext, mServiceClassName);
        mImpl.setService(this);
        super.attachBaseContext(baseContext);
    }

    @Override
    public void onFillRequest(
            FillRequest request, CancellationSignal cancellationSignal, FillCallback callback) {
        mImpl.onFillRequest(request, cancellationSignal, callback);
    }

    @Override
    public void onSaveRequest(SaveRequest request, SaveCallback callback) {
        mImpl.onSaveRequest(request, callback);
    }

    @Override
    public void onConnected() {
        super.onConnected();
        mImpl.onConnected();
    }

    @Override
    public void onDisconnected() {
        super.onDisconnected();
        mImpl.onDisconnected();
    }

    /**
     * Holds the implementation of autofill service logic. Will be called by {@link
     * SplitCompatAutofillService}.
     */
    public abstract static class Impl {
        private SplitCompatAutofillService mService;

        @Initializer
        protected final void setService(SplitCompatAutofillService service) {
            mService = service;
        }

        protected final AutofillService getService() {
            return mService;
        }

        public abstract void onFillRequest(
                FillRequest request, CancellationSignal cancellationSignal, FillCallback callback);

        public abstract void onSaveRequest(SaveRequest request, SaveCallback callback);

        public void onConnected() {}

        public void onDisconnected() {}
    }
}
