/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_sponsored_images;

import android.graphics.Bitmap;
import android.support.annotation.Nullable;

import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.ntp_background_images.NTPImage;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;

public class NTPSponsoredImagesBridge {
    private long mNativeNTPSponsoredImagesBridge;
    private final ObserverList<NTPSponsoredImageServiceObserver> mObservers =
            new ObserverList<NTPSponsoredImageServiceObserver>();

    public static class Wallpaper extends NTPImage {
        private String mImagePath;
        private int mFocalPointX;
        private int mFocalPointY;
        private String mLogoPath;
        private String mLogoDestinationUrl;

        private Wallpaper(String imagePath, int focalPointX, int focalPointY,
                          String logoPath, String logoDestinationUrl) {
            mImagePath = imagePath;
            mFocalPointX = focalPointX;
            mFocalPointY = focalPointY;
            mLogoPath = logoPath;
            mLogoDestinationUrl = logoDestinationUrl;
        }

        public String getImagePath() {
            return mImagePath;
        }

        public int getFocalPointX() {
            return mFocalPointX;
        }

        public int getFocalPointY() {
            return mFocalPointY;
        }

        public String getLogoPath() {
            return mLogoPath;
        }

        public String getLogoDestinationUrl() {
            return mLogoDestinationUrl;
        }

        public Bitmap getBitmap() {
            return null;
        }

        public Bitmap getLogoBitmap() {
            return null;
        }
    }

    public abstract static class NTPSponsoredImageServiceObserver {
        public abstract void onUpdated();
    }

    public NTPSponsoredImagesBridge(long nativeNTPSponsoredImagesBridge) {
        ThreadUtils.assertOnUiThread();
        mNativeNTPSponsoredImagesBridge = nativeNTPSponsoredImagesBridge;
    }

    @CalledByNative
    private static NTPSponsoredImagesBridge create(long nativeNTPSponsoredImagesBridge) {
        return new NTPSponsoredImagesBridge(nativeNTPSponsoredImagesBridge);
    }

    /**
     * Destroys this instance so no further calls can be executed.
     */
    @CalledByNative
    public void destroy() {
        mNativeNTPSponsoredImagesBridge = 0;
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

    static public boolean enableSponsoredImages() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
        && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed();
    }

    static public NTPSponsoredImagesBridge getInstance(Profile profile)  {
        return NTPSponsoredImagesBridgeJni.get().getInstance(profile);
    }

    @Nullable
    public Wallpaper getCurrentWallpaper() {
        ThreadUtils.assertOnUiThread();
        if (enableSponsoredImages()) {
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
    public static Wallpaper createWallpaper(
            String imagePath, int focalPointX, int focalPointY,
            String logoPath, String logoDestinationUrl) {
        return new Wallpaper(imagePath, focalPointX, focalPointY,
                             logoPath, logoDestinationUrl);
    }

    @CalledByNative
    public void onUpdated() {
        for (NTPSponsoredImageServiceObserver observer : mObservers) {
            observer.onUpdated();
        }
    }

    @NativeMethods
    interface Natives {
        NTPSponsoredImagesBridge getInstance(Profile profile);
        Wallpaper getCurrentWallpaper(long nativeNTPSponsoredImagesBridge,
                                      NTPSponsoredImagesBridge caller);
        void registerPageView(long nativeNTPSponsoredImagesBridge,
                              NTPSponsoredImagesBridge caller);
    }
}
