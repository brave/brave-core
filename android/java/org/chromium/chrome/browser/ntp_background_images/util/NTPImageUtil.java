/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Build;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.util.ImageUtils;
import org.chromium.components.user_prefs.UserPrefs;

import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.SoftReference;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class NTPImageUtil {
    private static final String TAG = "NTPUtil";

    private static final int BOTTOM_TOOLBAR_HEIGHT = 56;
    private static final String REMOVED_SITES = "removed_sites";

    public static HashMap<String, SoftReference<Bitmap>> imageCache =
            new HashMap<String, SoftReference<Bitmap>>();

    private static SpannableString getBannerText(ChromeActivity chromeActivity, int ntpType,
            View bannerLayout, SponsoredTab sponsoredTab, NewTabPageListener newTabPageListener) {
        String bannerText = "";
        if (ntpType == SponsoredImageUtil.BR_ON_ADS_ON) {
            bannerText = String.format(
                    chromeActivity.getResources().getString(R.string.you_are_earning_tokens),
                    chromeActivity.getResources().getString(R.string.learn_more));
        } else if (ntpType == SponsoredImageUtil.BR_ON_ADS_OFF) {
            bannerText = String.format(
                    chromeActivity.getResources().getString(R.string.earn_tokens_for_viewing),
                    chromeActivity.getResources().getString(R.string.learn_more));
        }
        int learnMoreIndex =
                bannerText.indexOf(chromeActivity.getResources().getString(R.string.learn_more));
        Spanned learnMoreSpanned = BraveRewardsHelper.spannedFromHtmlString(bannerText);
        SpannableString learnMoreTextSS = new SpannableString(learnMoreSpanned.toString());

        ForegroundColorSpan brOffForegroundSpan =
                new ForegroundColorSpan(chromeActivity.getColor(R.color.brave_theme_color));
        // setSpan gives us IndexOutOfBoundsException in case of end or start are > length and in
        // some other cases.
        int length = learnMoreTextSS.length();
        int endIndex = learnMoreIndex
                + chromeActivity.getResources().getString(R.string.learn_more).length();
        if (endIndex < learnMoreIndex || learnMoreIndex >= length || endIndex >= length
                || learnMoreIndex < 0 || endIndex < 0) {
            return learnMoreTextSS;
        }
        try {
            learnMoreTextSS.setSpan(brOffForegroundSpan, learnMoreIndex, endIndex,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        } catch (IndexOutOfBoundsException e) {
            Log.e(TAG,
                    "getBannerText: learnMoreIndex == " + learnMoreIndex
                            + ", endIndex == " + endIndex + ". Error: " + e.getMessage());
        }

        return learnMoreTextSS;
    }

    public static Bitmap getWallpaperBitmap(NTPImage ntpImage, int layoutWidth, int layoutHeight) {
        Context mContext = ContextUtils.getApplicationContext();

        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        options.inSampleSize = ImageUtils.calculateInSampleSize(options, layoutWidth, layoutHeight);
        options.inJustDecodeBounds = false;

        Bitmap imageBitmap = null;
        float centerPointX;
        float centerPointY;

        if (ntpImage instanceof Wallpaper) {
            Wallpaper mWallpaper = (Wallpaper) ntpImage;
            imageBitmap = getBitmapFromImagePath(mWallpaper.getImagePath(), options);
            if (imageBitmap == null) return null;

            centerPointX = mWallpaper.getFocalPointX() == 0 ? (imageBitmap.getWidth() / 2)
                                                            : mWallpaper.getFocalPointX();
            centerPointY = mWallpaper.getFocalPointY() == 0 ? (imageBitmap.getHeight() / 2)
                                                            : mWallpaper.getFocalPointY();
        } else {
            BackgroundImage mBackgroundImage = (BackgroundImage) ntpImage;
            String imagePath = mBackgroundImage.getImagePath();

            // Bundled Background Images
            if (imagePath == null) {
                imageBitmap = BitmapFactory.decodeResource(
                        mContext.getResources(), mBackgroundImage.getImageDrawable(), options);

                centerPointX = mBackgroundImage.getCenterPointX();
                centerPointY = mBackgroundImage.getCenterPointY();
            } else {
                imageBitmap = getBitmapFromImagePath(imagePath, options);
                if (imageBitmap == null) return null;

                centerPointX = mBackgroundImage.getCenterPointX() == 0
                        ? (imageBitmap.getWidth() / 2)
                        : mBackgroundImage.getCenterPointX();
                centerPointY = mBackgroundImage.getCenterPointY() == 0
                        ? (imageBitmap.getHeight() / 2)
                        : mBackgroundImage.getCenterPointY();
            }
        }
        return getCalculatedBitmap(
                imageBitmap, centerPointX, centerPointY, layoutWidth, layoutHeight);
    }

    private static Bitmap getBitmapFromImagePath(String imagePath, BitmapFactory.Options options) {
        Context mContext = ContextUtils.getApplicationContext();
        Bitmap imageBitmap = null;
        Uri imageFileUri = Uri.parse("file://" + imagePath);
        try (InputStream inputStream =
                mContext.getContentResolver().openInputStream(imageFileUri)) {
            imageBitmap = BitmapFactory.decodeStream(inputStream, null, options);
        } catch (IOException exc) {
            Log.e(TAG, "getBitmapFromImagePath: IOException: " + exc.getMessage());
        } catch (IllegalArgumentException exc) {
            Log.e(TAG, "getBitmapFromImagePath: IllegalArgumentException: " + exc.getMessage());
        } catch (OutOfMemoryError exc) {
            Log.e(TAG, "getBitmapFromImagePath: OutOfMemoryError: " + exc.getMessage());
        }
        return imageBitmap;
    }

    public static Bitmap getCalculatedBitmap(Bitmap imageBitmap, float centerPointX,
            float centerPointY, int layoutWidth, int layoutHeight) {
        float imageWidth = imageBitmap.getWidth();
        float imageHeight = imageBitmap.getHeight();

        float centerRatioX = centerPointX / imageWidth;

        float imageWHRatio = imageWidth / imageHeight;
        float imageHWRatio = imageHeight / imageWidth;

        int newImageWidth = (int) (layoutHeight * imageWHRatio);
        int newImageHeight = layoutHeight;

        if (newImageWidth < layoutWidth) {
            // Image is now too small so we need to adjust width and height based on
            // This covers landscape and strange tablet sizes.
            newImageWidth = layoutWidth;
            newImageHeight = (int) (newImageWidth * imageHWRatio);
        }

        int newCenterX = (int) (newImageWidth * centerRatioX);
        int startX = (int) (newCenterX - (layoutWidth / 2));
        if (newCenterX < layoutWidth / 2) {
            // Need to crop starting at 0 to newImageWidth - left aligned image
            startX = 0;
        } else if (newImageWidth - newCenterX < layoutWidth / 2) {
            // Need to crop right side of image - right aligned
            startX = newImageWidth - layoutWidth;
        }

        int startY = (newImageHeight - layoutHeight) / 2;
        if (centerPointY > 0) {
            float centerRatioY = centerPointY / imageHeight;
            newImageWidth = layoutWidth;
            newImageHeight = (int) (layoutWidth * imageHWRatio);

            if (newImageHeight < layoutHeight) {
                newImageHeight = layoutHeight;
                newImageWidth = (int) (newImageHeight * imageWHRatio);
            }

            int newCenterY = (int) (newImageHeight * centerRatioY);
            startY = (int) (newCenterY - (layoutHeight / 2));
            if (newCenterY < layoutHeight / 2) {
                // Need to crop starting at 0 to newImageWidth - left aligned image
                startY = 0;
            } else if (newImageHeight - newCenterY < layoutHeight / 2) {
                // Need to crop right side of image - right aligned
                startY = newImageHeight - layoutHeight;
            }
        }

        Bitmap newBitmap = null;
        Bitmap bitmapWithGradient = null;
        try {
            imageBitmap =
                    Bitmap.createScaledBitmap(imageBitmap, newImageWidth, newImageHeight, true);

            newBitmap = Bitmap.createBitmap(imageBitmap,
                    (startX + layoutWidth) <= imageBitmap.getWidth() ? startX : 0,
                    (startY + (int) layoutHeight) <= imageBitmap.getHeight() ? startY : 0,
                    layoutWidth, (int) layoutHeight);
            bitmapWithGradient = ImageUtils.addGradient(newBitmap);

            if (imageBitmap != null && !imageBitmap.isRecycled()) imageBitmap.recycle();
            if (newBitmap != null && !newBitmap.isRecycled()) newBitmap.recycle();

            return bitmapWithGradient;
        } catch (Exception exc) {
            exc.printStackTrace();
            Log.e(TAG, "getCalculatedBitmap: " + exc.getMessage());
            return null;
        } finally {
            if (imageBitmap != null && !imageBitmap.isRecycled()) imageBitmap.recycle();
            if (newBitmap != null && !newBitmap.isRecycled()) newBitmap.recycle();
        }
    }

    public static Bitmap getTopSiteBitmap(String iconPath) {
        Context mContext = ContextUtils.getApplicationContext();
        InputStream inputStream = null;
        Bitmap topSiteIcon = null;
        try {
            Uri imageFileUri = Uri.parse("file://" + iconPath);
            inputStream = mContext.getContentResolver().openInputStream(imageFileUri);
            topSiteIcon = BitmapFactory.decodeStream(inputStream);
            inputStream.close();
        } catch (IOException exc) {
            exc.printStackTrace();
            Log.e(TAG, "getTopSiteBitmap IOException: " + exc.getMessage());
            topSiteIcon = null;
        } finally {
            try {
                if (inputStream != null) {
                    inputStream.close();
                }
            } catch (IOException exception) {
                exception.printStackTrace();
                Log.e(TAG, "getTopSiteBitmap IOException in finally: " + exception.getMessage());
                topSiteIcon = null;
            }
        }
        return topSiteIcon;
    }

    private static Set<String> getRemovedTopSiteUrls() {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        return mSharedPreferences.getStringSet(REMOVED_SITES, new HashSet<String>());
    }

    public static boolean isInRemovedTopSite(String url) {
        Set<String> urlSet = getRemovedTopSiteUrls();
        if (urlSet.contains(url)) {
            return true;
        }
        return false;
    }

    public static void addToRemovedTopSite(String url) {
        Set<String> urlSet = getRemovedTopSiteUrls();
        if (!urlSet.contains(url)) {
            urlSet.add(url);
        }

        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putStringSet(REMOVED_SITES, urlSet);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldEnableNTPFeature() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return true;
        }
        return false;
    }

    public static NTPImage getNTPImage(NTPBackgroundImagesBridge mNTPBackgroundImagesBridge) {
        NTPImage mWallpaper = mNTPBackgroundImagesBridge.getCurrentWallpaper();
        if (mWallpaper != null) {
            return mWallpaper;
        } else {
            return SponsoredImageUtil.getBackgroundImage();
        }
    }

    public static boolean isReferralEnabled() {
        Profile mProfile = ProfileManager.getLastUsedRegularProfile();
        NTPBackgroundImagesBridge mNTPBackgroundImagesBridge =
                NTPBackgroundImagesBridge.getInstance(mProfile);
        boolean isReferralEnabled =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                                .getInteger(BravePref.NEW_TAB_PAGE_SUPER_REFERRAL_THEMES_OPTION)
                        == 1;
        return mNTPBackgroundImagesBridge.isSuperReferral() && isReferralEnabled;
    }

    public static int getViewHeight(View view) {
        view.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
        return view.getMeasuredHeight();
    }
}
