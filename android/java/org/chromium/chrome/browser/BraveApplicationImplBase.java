/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser;

import android.app.Activity;
import android.app.Application;
import android.content.Intent;
import android.os.Bundle;

import com.wireguard.android.backend.GoBackend;

import org.chromium.base.JavaUtils;
import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.base.SplitCompatApplication;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.components.safe_browsing.BraveSafeBrowsingApiHandler;
import org.chromium.components.safe_browsing.SafeBrowsingApiBridge;
import org.chromium.mojo.bindings.BadMessageException;
import org.chromium.mojo.bindings.ExceptionHandler;

@NullMarked
public class BraveApplicationImplBase extends SplitCompatApplication.Impl {
    private static final String TAG = "BraveApp";

    @Override
    public void onCreate() {
        super.onCreate();
        if (SplitCompatApplication.isBrowserProcess()) {
            // Handle Mojo BadMessageException gracefully by closing the pipe instead of
            // crashing. This matches the C++ behavior in Connector::DispatchMessage
            // (mojo/public/cpp/bindings/lib/connector.cc) where a failed Accept() calls
            // HandleError() to reset the pipe — no crash, just clean pipe closure.
            // Upstream Chromium introduced BadMessageException in Java bindings in 146
            // (crbug.com/469861566,
            // https://github.com/chromium/chromium/commit/c04b8552deeef) but left the
            // default handler as a rethrow, causing crashes in race conditions during
            // teardown. When upstream finishes their TODO in Connector.java they will
            // likely handle BadMessageException directly in the catch block rather than
            // via this delegate, at which point this handler becomes a harmless no-op
            // and can be removed.
            ExceptionHandler.DefaultExceptionHandler.getInstance()
                    .setDelegate(
                            (Throwable e) -> {
                                if (e instanceof BadMessageException) {
                                    Log.w(TAG, "Mojo BadMessageException, closing pipe", e);
                                    return false;
                                }
                                throw JavaUtils.throwUnchecked(e);
                            });
            GoBackend.setAlwaysOnCallback(
                    new GoBackend.AlwaysOnCallback() {
                        @Override
                        public void alwaysOnTriggered() {
                            BraveVpnProfileUtils.getInstance().startVpn(getApplication());
                        }
                    });
            // Set a handler for SafeBrowsing. It has to be done only once for a process lifetime.
            SafeBrowsingApiBridge.setSafeBrowsingApiHandler(
                    BraveSafeBrowsingApiHandler.getInstance());

            // Fix ClassNotFoundException crash in Play Core's in-app review flow.
            //
            // When the app is installed from Google Play as split APKs (AAB), each split
            // gets its own ClassLoader. PlayCoreDialogWrapperActivity (from core-common)
            // lives in base.apk, but the Parcelable it deserializes from its Intent extras
            // (com.google.android.play.core.review.zzc, a ResultReceiver subclass from the
            // review library) lives in split_chrome.apk. When the activity calls
            // getIntent().getParcelableExtra("result_receiver") in onCreate(), the Bundle
            // uses the base split's ClassLoader which cannot see chrome split classes,
            // causing BadParcelableException -> ClassNotFoundException.
            //
            // Chromium has the same class of bug for its own activities and fixes it in
            // ChromeBaseAppCompatActivity.attachBaseContext() via
            // BundleUtils.checkContextClassLoader() (see https://crbug.com/346709145).
            // However, PlayCoreDialogWrapperActivity extends plain android.app.Activity,
            // not ChromeBaseAppCompatActivity, so it never receives that fix.
            //
            // This callback fires before onCreate() and sets the Intent Bundle's
            // ClassLoader to the Application's ClassLoader (which points to
            // split_chrome.apk and can resolve all app classes). This is not reproducible
            // with local APK installs — only with split APK delivery (Play Store or
            // bundletool --local-testing).
            getApplication()
                    .registerActivityLifecycleCallbacks(
                            new Application.ActivityLifecycleCallbacks() {
                                @Override
                                public void onActivityPreCreated(
                                        Activity activity, @Nullable Bundle savedInstanceState) {
                                    if (activity.getClass()
                                            .getName()
                                            .equals(
                                                    "com.google.android.play.core.common"
                                                            + ".PlayCoreDialogWrapperActivity")) {
                                        Intent intent = activity.getIntent();
                                        if (intent != null) {
                                            intent.setExtrasClassLoader(
                                                    getApplication().getClassLoader());
                                        }
                                    }
                                }

                                @Override
                                public void onActivityCreated(
                                        Activity activity, @Nullable Bundle savedInstanceState) {}

                                @Override
                                public void onActivityStarted(Activity activity) {}

                                @Override
                                public void onActivityResumed(Activity activity) {}

                                @Override
                                public void onActivityPaused(Activity activity) {}

                                @Override
                                public void onActivityStopped(Activity activity) {}

                                @Override
                                public void onActivitySaveInstanceState(
                                        Activity activity, Bundle outState) {}

                                @Override
                                public void onActivityDestroyed(Activity activity) {}
                            });
        }
    }
}
