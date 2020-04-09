/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import android.graphics.Bitmap;
import android.support.annotation.Nullable;
import java.util.List;
import java.util.ArrayList;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.ntp_background_images.NTPImage;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp_background_images.NewTabPageListener;

public class NTPBackgroundImagesBridge {
    private long mNativeNTPBackgroundImagesBridge;
    private final ObserverList<NTPBackgroundImageServiceObserver> mObservers =
            new ObserverList<NTPBackgroundImageServiceObserver>();
    private static List<TopSite> mTopSites = new ArrayList<>();
    private static NewTabPageListener mNewTabPageListener;

    public static class Wallpaper extends NTPImage {
        private String mImagePath;
        private int mFocalPointX;
        private int mFocalPointY;
        private String mLogoPath;
        private String mLogoDestinationUrl;
        private String mThemeName;
        private boolean mIsSponsored;

        private Wallpaper(String imagePath, int focalPointX, int focalPointY,
                          String logoPath, String logoDestinationUrl , String themeName, 
                          boolean isSponsored) {
            mImagePath = imagePath;
            mFocalPointX = focalPointX;
            mFocalPointY = focalPointY;
            mLogoPath = logoPath;
            mLogoDestinationUrl = logoDestinationUrl;
            mThemeName = themeName;
            mIsSponsored = isSponsored;
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

        public String getThemeName() {
            return mThemeName;
        }

        public boolean isSponsored() {
            return mIsSponsored;
        }
    }

    public static class TopSite {
        private String mName;
        private String mDestinationUrl;
        private String mBackgroundColor;
        private String mImagePath;

        private TopSite(String name, String destinationUrl, String backgroundColor, String imagePath) {
            mName = name;
            mDestinationUrl = destinationUrl;
            mBackgroundColor = backgroundColor;
            mImagePath = imagePath;
        }

        public String getName() {
            return mName;
        }        

        public String getDestinationUrl() {
            return mDestinationUrl;
        }

        public String getBackgroundColor() {
            return mBackgroundColor;
        }

        public String getImagePath() {
            return mImagePath;
        }
    }

    public abstract static class NTPBackgroundImageServiceObserver {
        public abstract void onUpdated();
    }

    public NTPBackgroundImagesBridge(long nativeNTPBackgroundImagesBridge) {
        ThreadUtils.assertOnUiThread();
        mNativeNTPBackgroundImagesBridge = nativeNTPBackgroundImagesBridge;
    }

    @CalledByNative
    private static NTPBackgroundImagesBridge create(long nativeNTPBackgroundImagesBridge) {
        return new NTPBackgroundImagesBridge(nativeNTPBackgroundImagesBridge);
    }

    /**
     * Destroys this instance so no further calls can be executed.
     */
    @CalledByNative
    public void destroy() {
        mNativeNTPBackgroundImagesBridge = 0;
        mObservers.clear();
    }

    /**
     * @param observer The observer to be added.
     */
    public void addObserver(NTPBackgroundImageServiceObserver observer) {
        mObservers.addObserver(observer);
    }

    /**
     * @param observer The observer to be removed.
     */
    public void removeObserver(NTPBackgroundImageServiceObserver observer) {
        mObservers.removeObserver(observer);
    }

    static public boolean enableSponsoredImages() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
        && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed();
    }

    static public NTPBackgroundImagesBridge getInstance(Profile profile)  {
        return NTPBackgroundImagesBridgeJni.get().getInstance(profile);
    }

    @Nullable
    public Wallpaper getCurrentWallpaper() {
        ThreadUtils.assertOnUiThread();
        if (enableSponsoredImages()) {
            return NTPBackgroundImagesBridgeJni.get().getCurrentWallpaper(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
        } else {
            return null;
        }
    }

    public void registerPageView() {
        NTPBackgroundImagesBridgeJni.get().registerPageView(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
    }

    public boolean isSuperReferral() {
        return NTPBackgroundImagesBridgeJni.get().isSuperReferral(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
    }

    public String getSuperReferralThemeName() {
        return NTPBackgroundImagesBridgeJni.get().getSuperReferralThemeName(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
    }

    public String getSuperReferralCode() {
        return NTPBackgroundImagesBridgeJni.get().getSuperReferralCode(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
    }

    public void getTopSites() {
        mTopSites.clear();
        NTPBackgroundImagesBridgeJni.get().getTopSites(
                mNativeNTPBackgroundImagesBridge, NTPBackgroundImagesBridge.this);
    }

    public void setNewTabPageListener(NewTabPageListener newTabPageListener) {
        mNewTabPageListener = newTabPageListener;
    }

    @CalledByNative
    public static void loadTopSitesData(String name, String destinationUrl, String backgroundColor, String imagePath) {
        mTopSites.add(new TopSite(name, destinationUrl, backgroundColor, imagePath));
    }

    @CalledByNative
    public static void topSitesLoaded() {
        mNewTabPageListener.updateTopSites(mTopSites);
    }

    @CalledByNative
    public static Wallpaper createWallpaper(
            String imagePath, int focalPointX, int focalPointY,
            String logoPath, String logoDestinationUrl, 
            String themeName, boolean isSponsored) {
        return new Wallpaper(imagePath, focalPointX, focalPointY,
                             logoPath, logoDestinationUrl, 
                             themeName, isSponsored);
    }

    @CalledByNative
    public void onUpdated() {
        for (NTPBackgroundImageServiceObserver observer : mObservers) {
            observer.onUpdated();
        }
    }

    @NativeMethods
    interface Natives {
        NTPBackgroundImagesBridge getInstance(Profile profile);
        Wallpaper getCurrentWallpaper(long nativeNTPBackgroundImagesBridge,
                                      NTPBackgroundImagesBridge caller);
        void registerPageView(long nativeNTPBackgroundImagesBridge,
                              NTPBackgroundImagesBridge caller);
        void getTopSites(long nativeNTPBackgroundImagesBridge,
                              NTPBackgroundImagesBridge caller);
        boolean isSuperReferral(long nativeNTPBackgroundImagesBridge,
                              NTPBackgroundImagesBridge caller);
        String getSuperReferralThemeName(long nativeNTPBackgroundImagesBridge,
                              NTPBackgroundImagesBridge caller);
        String getSuperReferralCode(long nativeNTPBackgroundImagesBridge,
                              NTPBackgroundImagesBridge caller);
    }
}
