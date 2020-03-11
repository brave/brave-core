/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_sponsored_images;


import android.support.annotation.Nullable;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.sponsored.NTPImage;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;

public class NTPSponsoredImagesBridge {
    private final Profile mProfile;
    private long mNativeNTPSponsoredImagesBridge;
    private final ObserverList<NTPSponsoredImageServiceObserver> mObservers =
            new ObserverList<NTPSponsoredImageServiceObserver>();

    public static class Wallpaper extends NTPImage {
        private Bitmap mBitmap;
        private int mFocalPointX;
        private int mFocalPointY;

        private Wallpaper(Bitmap bitmap, int focalPointX, int focalPointY) {
            mBitmap = bitmap;
            mFocalPointX = focalPointX;
            mFocalPointY = focalPointY;
        }

        public Bitmap getBitmap() {
            return mBitmap;
        }

        public int getFocalPointX() {
            return mFocalPointX;
        }

        public int getFocalPointY() {
            return mFocalPointY;
        }
    }

    public abstract static class NTPSponsoredImageServiceObserver {
        public abstract void onUpdated();
    }

    public NTPSponsoredImagesBridge(Profile profile) {
        ThreadUtils.assertOnUiThread();
        mProfile = profile;
        mNativeNTPSponsoredImagesBridge =
            NTPSponsoredImagesBridgeJni.get().init(
                    NTPSponsoredImagesBridge.this, profile);
    }

    /**
     * Destroys this instance so no further calls can be executed.
     */
    public void destroy() {
        if (mNativeNTPSponsoredImagesBridge != 0) {
            NTPSponsoredImagesBridgeJni.get().destroy(
                    mNativeNTPSponsoredImagesBridge,
                    NTPSponsoredImagesBridge.this);
            mNativeNTPSponsoredImagesBridge = 0;
        }
        mObservers.clear();
    }

    /**
     * @param observer The observer to be added.
     */
    public void addObserver(NTPSponsoredImageServiceObserver observer) {
        mObservers.addObserver(observer);
    }

    /**
     * @param observer The observer to be removed.
     */
    public void removeObserver(NTPSponsoredImageServiceObserver observer) {
        mObservers.removeObserver(observer);
    }

    static public boolean enableSponsoredImages(Profile profile) {
        // return BraveAdsNativeHelper.nativeIsLocaleValid(profile)
        //        && BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()
        //        && ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS);
        return true;
    }

    @Nullable
    public Wallpaper getCurrentWallpaper() {
        ThreadUtils.assertOnUiThread();
        if (enableSponsoredImages(mProfile)) {
            return NTPSponsoredImagesBridgeJni.get().getCurrentWallpaper(
                mNativeNTPSponsoredImagesBridge, NTPSponsoredImagesBridge.this);
        } else {
            return null;
        }
    }

    public void registerPageView() {
        NTPSponsoredImagesBridgeJni.get().registerPageView(
                mNativeNTPSponsoredImagesBridge, NTPSponsoredImagesBridge.this);
    }

    @CalledByNative
    public static Wallpaper createWallpaper(Bitmap bitmap, int focalPointX, int focalPointY) {
        return new Wallpaper(bitmap, focalPointX, focalPointY);
    }

    @CalledByNative
    public void onUpdated() {
        for (NTPSponsoredImageServiceObserver observer : mObservers) {
            observer.onUpdated();
        }
    }

    @NativeMethods
    interface Natives {
        long init(NTPSponsoredImagesBridge caller, Profile profile);
        void destroy(long nativeNTPSponsoredImagesBridge,
                     NTPSponsoredImagesBridge caller);
        Wallpaper getCurrentWallpaper(long nativeNTPSponsoredImagesBridge,
                                      NTPSponsoredImagesBridge caller);
        void registerPageView(long nativeNTPSponsoredImagesBridge,
                              NTPSponsoredImagesBridge caller);
    }
}
