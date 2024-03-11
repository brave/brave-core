/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.os.CancellationSignal;
import android.service.autofill.AutofillService;
import android.service.autofill.FillCallback;
import android.service.autofill.FillRequest;
import android.service.autofill.SaveCallback;
import android.service.autofill.SaveRequest;

import androidx.annotation.NonNull;

import com.google.android.gms.gcm.TaskParams;

import org.chromium.chrome.browser.init.MinimalBrowserStartupUtils;

public class BraveAutofillService extends AutofillService {

    @Override
    public void onFillRequest(
            @NonNull FillRequest request,
            @NonNull CancellationSignal cancellationSignal,
            @NonNull FillCallback callback) {
        BraveAutofillBackgroundServiceImpl backgroundServiceImpl =
                new BraveAutofillBackgroundServiceImpl(request, callback);
        backgroundServiceImpl.onRunTask(new TaskParams(MinimalBrowserStartupUtils.TASK_TAG));
    }

    @Override
    public void onSaveRequest(@NonNull SaveRequest request, @NonNull SaveCallback callback) {
        BraveAutofillBackgroundServiceImpl backgroundServiceImpl =
                new BraveAutofillBackgroundServiceImpl(request, callback);
        backgroundServiceImpl.onRunTask(new TaskParams(MinimalBrowserStartupUtils.TASK_TAG));
    }
}
