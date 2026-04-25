/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.helpers;

import android.os.Build;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import java.lang.ref.WeakReference;

/**
 * Helper class for back press event handling via {@link OnBackInvokedDispatcher}.
 * This should only be used to resolve Android 13+ back press issue
 * {@link https://github.com/brave/brave-browser/issues/27787}.
 */
public final class Api33AndPlusBackPressHelper {
    private final WeakReference<FragmentActivity> mActivity;

    @RequiresApi(api = Build.VERSION_CODES.TIRAMISU)
    public static void create(LifecycleOwner lifecycleOwner, FragmentActivity activity,
            OnBackInvokedCallback handler) {
        new Api33AndPlusBackPressHelper(lifecycleOwner, activity, handler);
    }

    private Api33AndPlusBackPressHelper(LifecycleOwner lifecycleOwner, FragmentActivity activity,
            OnBackInvokedCallback handler) {
        mActivity = new WeakReference<>(activity);
        if (isActive() && Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            getRef().getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    OnBackInvokedDispatcher.PRIORITY_DEFAULT, handler);
            lifecycleOwner.getLifecycle().addObserver(new DefaultLifecycleObserver() {
                @Override
                public void onDestroy(@NonNull LifecycleOwner owner) {
                    if (isAvailable()) {
                        getRef().getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(
                                handler);
                    }
                }
            });
        }
    }

    private FragmentActivity getRef() {
        return mActivity.get();
    }

    private boolean isAvailable() {
        return mActivity != null && mActivity.get() != null;
    }

    private boolean isActive() {
        return isAvailable() && !mActivity.get().isFinishing();
    }
}
