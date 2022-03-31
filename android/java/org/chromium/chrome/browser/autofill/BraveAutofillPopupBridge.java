/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.app.Activity;
import android.util.DisplayMetrics;
import android.view.View;

import androidx.annotation.NonNull;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.ui.base.WindowAndroid;

@JNINamespace("autofill")
public class BraveAutofillPopupBridge extends AutofillPopupBridge {
    public BraveAutofillPopupBridge(@NonNull View anchorView, long nativeAutofillPopupViewAndroid,
            @NonNull WindowAndroid windowAndroid) {
        super(anchorView, nativeAutofillPopupViewAndroid, windowAndroid);

        Activity activity = windowAndroid != null ? windowAndroid.getActivity().get() : null;
        if (activity instanceof BraveActivity) {
            DisplayMetrics metrics = activity.getResources().getDisplayMetrics();
            float ratio = metrics.widthPixels > metrics.heightPixels
                    ? ((float) metrics.widthPixels / (float) metrics.heightPixels)
                    : 1.0f;
            float newAnchorViewY = ((BraveActivity) activity).getToolbarBottom() * ratio;
            if (newAnchorViewY > anchorView.getY()) {
                anchorView.setY(newAnchorViewY);
            }
        }
    }
}
