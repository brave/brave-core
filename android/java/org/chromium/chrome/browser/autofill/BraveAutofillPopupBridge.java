/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.app.Activity;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewTreeObserver;

import androidx.annotation.NonNull;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.ui.base.WindowAndroid;

@JNINamespace("autofill")
public class BraveAutofillPopupBridge
        extends AutofillPopupBridge implements ViewTreeObserver.OnPreDrawListener {
    private View mView;
    private BraveActivity mActivity;
    private ViewTreeObserver mViewTreeObserver;

    public BraveAutofillPopupBridge(@NonNull View anchorView, long nativeAutofillPopupViewAndroid,
            @NonNull WindowAndroid windowAndroid) {
        super(anchorView, nativeAutofillPopupViewAndroid, windowAndroid);

        Activity activity = windowAndroid != null ? windowAndroid.getActivity().get() : null;
        assert activity instanceof BraveActivity : "Wrong activity type!";
        if (activity instanceof BraveActivity) {
            mActivity = (BraveActivity) activity;
        }

        mView = anchorView;
        mViewTreeObserver = mView.getViewTreeObserver();
        mViewTreeObserver.addOnPreDrawListener(this);
    }

    // ViewTreeObserver.OnPreDrawListener implementation.
    @Override
    public boolean onPreDraw() {
        if (mView.isShown() && mActivity != null && !mActivity.isViewBelowToolbar(mView)) {
            mView.setY(mActivity.getToolbarBottom());
        }

        // We need to make an adjustment only once.
        if (mViewTreeObserver != null && mViewTreeObserver.isAlive()) {
            mViewTreeObserver.removeOnPreDrawListener(this);
        }

        return true;
    }
}
