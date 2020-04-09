/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import android.os.Bundle;
import android.os.Build;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import android.view.ViewGroup;
import android.view.View;
import android.view.Gravity;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Button;
import android.os.Handler;
import android.net.Uri;
import java.io.IOException;

import org.chromium.chrome.R;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.ntp_background_images.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.SponsoredImageUtil;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.util.ImageUtils;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;

import static org.chromium.ui.base.ViewUtils.dpToPx;

public class NTPUtil {
	private static final int BOTTOM_TOOLBAR_HEIGHT = 56;

    public static void openImageCredit(String url) {
        ChromeTabbedActivity chromeTabbedActivity = BraveRewardsHelper.getChromeTabbedActivity();
        if(chromeTabbedActivity != null) {
            LoadUrlParams loadUrlParams = new LoadUrlParams(url);
            chromeTabbedActivity.getActivityTab().loadUrl(loadUrlParams);
        }
    }

    public static void turnOnAds() {
        BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedProfile());
    }

    public static void updateOrientedUI(Context context, ViewGroup view) {
        LinearLayout parentLayout= (LinearLayout)view.findViewById(R.id.parent_layout);
        ViewGroup mainLayout = view.findViewById(R.id.ntp_main_layout);
        ViewGroup imageCreditLayout = view.findViewById(R.id.image_credit_layout);

        ImageView sponsoredLogo = (ImageView)view.findViewById(R.id.sponsored_logo);
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(dpToPx(context,170), dpToPx(context,170));

        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);

        if (ConfigurationUtils.isLandscape(context) && BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)) {
            // In landscape
            parentLayout.removeView(mainLayout);
            parentLayout.removeView(imageCreditLayout);

            if (isTablet) {
                parentLayout.addView(mainLayout);
                parentLayout.addView(imageCreditLayout);

                parentLayout.setOrientation(LinearLayout.VERTICAL);

                LinearLayout.LayoutParams mainLayoutLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,0);
                mainLayoutLayoutParams.weight = 1f;
                mainLayout.setLayoutParams(mainLayoutLayoutParams);

                LinearLayout.LayoutParams imageCreditLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
                imageCreditLayout.setLayoutParams(imageCreditLayoutParams);

                layoutParams.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                sponsoredLogo.setLayoutParams(layoutParams);

            } else {
                parentLayout.addView(imageCreditLayout);
                parentLayout.addView(mainLayout);

                parentLayout.setOrientation(LinearLayout.HORIZONTAL);

                LinearLayout.LayoutParams mainLayoutLayoutParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT);
                mainLayoutLayoutParams.weight = 0.6f;
                mainLayout.setLayoutParams(mainLayoutLayoutParams);

                LinearLayout.LayoutParams imageCreditLayoutParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT);
                imageCreditLayoutParams.weight = 0.4f;
                imageCreditLayout.setLayoutParams(imageCreditLayoutParams);

                layoutParams.setMargins(dpToPx(context,32), 0, 0, 0);
                layoutParams.gravity = Gravity.BOTTOM | Gravity.START;
                sponsoredLogo.setLayoutParams(layoutParams);
            }
        } else {
            // In portrait
            parentLayout.removeView(mainLayout);
            parentLayout.removeView(imageCreditLayout);

            parentLayout.addView(mainLayout);
            parentLayout.addView(imageCreditLayout);

            parentLayout.setOrientation(LinearLayout.VERTICAL);

            LinearLayout.LayoutParams mainLayoutLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,0);
            mainLayoutLayoutParams.weight = 1f;
            mainLayout.setLayoutParams(mainLayoutLayoutParams);

            LinearLayout.LayoutParams imageCreditLayoutParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            imageCreditLayout.setLayoutParams(imageCreditLayoutParams);

            layoutParams.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
            sponsoredLogo.setLayoutParams(layoutParams);
        }
    }

    public static int checkForNonDistruptiveBanner(NTPImage ntpImage, SponsoredTab sponsoredTab) {
        BraveRewardsNativeWorker mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (sponsoredTab.shouldShowBanner()) {
            if (BraveRewardsPanelPopup.isBraveRewardsEnabled()) {
                if (BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedProfile())) {
                    if (ntpImage instanceof NTPBackgroundImagesBridge.Wallpaper) {
                        return SponsoredImageUtil.BR_ON_ADS_ON;
                    }
                } else if(BraveAdsNativeHelper.nativeIsLocaleValid(Profile.getLastUsedProfile())) {
                    if (ntpImage instanceof NTPBackgroundImagesBridge.Wallpaper) {
                        return SponsoredImageUtil.BR_ON_ADS_OFF ;
                    } else {
                        return SponsoredImageUtil.BR_ON_ADS_OFF_BG_IMAGE;
                    }
                }
            } else {
                if (ntpImage instanceof NTPBackgroundImagesBridge.Wallpaper && !mBraveRewardsNativeWorker.IsCreateWalletInProcess()) {
                    return SponsoredImageUtil.BR_OFF;
                }
            }
        }
        return SponsoredImageUtil.BR_INVALID_OPTION;
    }

    public static void showNonDistruptiveBanner(ChromeActivity chromeActivity, View view, int ntpType, SponsoredTab sponsoredTab, NewTabPageListener newTabPageListener) {
        ViewGroup nonDistruptiveBannerLayout = (ViewGroup) view.findViewById(R.id.non_distruptive_banner);
        nonDistruptiveBannerLayout.setVisibility(View.GONE);

        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                BackgroundImagesPreferences.setOnPreferenceValue(BackgroundImagesPreferences.PREF_SHOW_NON_DISTRUPTIVE_BANNER,false);

                boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(chromeActivity);
                if(isTablet || (!isTablet && ConfigurationUtils.isLandscape(chromeActivity))) {
                    FrameLayout.LayoutParams nonDistruptiveBannerLayoutParams = new FrameLayout.LayoutParams(dpToPx(chromeActivity, 400), FrameLayout.LayoutParams.WRAP_CONTENT);
                    nonDistruptiveBannerLayoutParams.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                    nonDistruptiveBannerLayout.setLayoutParams(nonDistruptiveBannerLayoutParams);
                } else {
                    FrameLayout.LayoutParams nonDistruptiveBannerLayoutParams = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
                    nonDistruptiveBannerLayoutParams.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                    nonDistruptiveBannerLayout.setLayoutParams(nonDistruptiveBannerLayoutParams);
                }
                nonDistruptiveBannerLayout.setVisibility(View.VISIBLE);

                TextView bannerHeader = nonDistruptiveBannerLayout.findViewById(R.id.ntp_banner_header);
                TextView bannerText = nonDistruptiveBannerLayout.findViewById(R.id.ntp_banner_text);
                TextView learnMoreText = nonDistruptiveBannerLayout.findViewById(R.id.ntp_banner_learn_more_text);
                Button turnOnAdsButton = nonDistruptiveBannerLayout.findViewById(R.id.btn_turn_on_ads);
                ImageView bannerClose = nonDistruptiveBannerLayout.findViewById(R.id.ntp_banner_close);
                bannerClose.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        nonDistruptiveBannerLayout.setVisibility(View.GONE);
                        sponsoredTab.updateBannerPref();
                    }
                });

                learnMoreText.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        nonDistruptiveBannerLayout.setVisibility(View.GONE);

                        RewardsBottomSheetDialogFragment rewardsBottomSheetDialogFragment = RewardsBottomSheetDialogFragment.newInstance();
                        Bundle bundle = new Bundle();
                        bundle.putInt(SponsoredImageUtil.NTP_TYPE, ntpType);
                        rewardsBottomSheetDialogFragment.setArguments(bundle);
                        rewardsBottomSheetDialogFragment.setNewTabPageListener(newTabPageListener);
                        rewardsBottomSheetDialogFragment.show(chromeActivity.getSupportFragmentManager(), "rewards_bottom_sheet_dialog_fragment");
                        rewardsBottomSheetDialogFragment.setCancelable(false);

                        sponsoredTab.updateBannerPref();
                    }
                });

                turnOnAdsButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        NTPUtil.turnOnAds();
                        nonDistruptiveBannerLayout.setVisibility(View.GONE);

                        sponsoredTab.updateBannerPref();
                    }
                });

                switch(ntpType) {
                    case SponsoredImageUtil.BR_OFF:
                        bannerText.setText(chromeActivity.getResources().getString(R.string.get_paid_to_see_image));
                        learnMoreText.setVisibility(View.VISIBLE);
                        break;
                    case SponsoredImageUtil.BR_ON_ADS_OFF:
                        bannerText.setText(chromeActivity.getResources().getString(R.string.get_paid_to_see_image));
                        learnMoreText.setVisibility(View.VISIBLE);
                        break;
                    case SponsoredImageUtil.BR_ON_ADS_OFF_BG_IMAGE:
                        bannerText.setText(chromeActivity.getResources().getString(R.string.you_can_support_creators));
                        turnOnAdsButton.setVisibility(View.VISIBLE);
                        break;
                    case SponsoredImageUtil.BR_ON_ADS_ON:
                        bannerText.setText(chromeActivity.getResources().getString(R.string.you_are_getting_paid));
                        learnMoreText.setVisibility(View.VISIBLE);
                        break;
                }
            }
        }, 1500);
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

        if (ntpImage instanceof NTPBackgroundImagesBridge.Wallpaper) {
            NTPBackgroundImagesBridge.Wallpaper mWallpaper = (NTPBackgroundImagesBridge.Wallpaper) ntpImage;
            InputStream inputStream = null;
            try {
                Uri imageFileUri = Uri.parse("file://"+mWallpaper.getImagePath());
                inputStream = mContext.getContentResolver().openInputStream(imageFileUri);
                imageBitmap = BitmapFactory.decodeStream(inputStream, null, options);
                inputStream.close();
            } catch(IOException exc) {
                Log.e("NTP", exc.getMessage());
            } finally {
              try {
                if (inputStream != null) {
                  inputStream.close();
                }
              } catch (IOException exception) {
                Log.e("NTP", exception.getMessage());
                return null;
              }
            }
            centerPointX = mWallpaper.getFocalPointX() == 0 ? (imageBitmap.getWidth()/2) : mWallpaper.getFocalPointX();
            centerPointY = mWallpaper.getFocalPointY() == 0 ? (imageBitmap.getHeight()/2) : mWallpaper.getFocalPointY();
        } else {
            BackgroundImage mBackgroundImage = (BackgroundImage) ntpImage;
            imageBitmap = BitmapFactory.decodeResource(mContext.getResources(), mBackgroundImage.getImageDrawable(), options);

            centerPointX = mBackgroundImage.getCenterPoint();
            centerPointY = 0;
        }
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

        int startY = (newImageHeight - layoutHeight)/2;
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

        imageBitmap = Bitmap.createScaledBitmap(imageBitmap, newImageWidth, newImageHeight, true);

        Bitmap newBitmap = Bitmap.createBitmap(imageBitmap, startX, startY, layoutWidth, (int) layoutHeight);
        Bitmap bitmapWithGradient = ImageUtils.addGradient(newBitmap);

        imageBitmap.recycle();
        newBitmap.recycle();

        return bitmapWithGradient;
    }

    public static Bitmap getTopSiteBitmap(String iconPath) {
        Context mContext = ContextUtils.getApplicationContext();
        try {
            Uri imageFileUri = Uri.parse("file://"+ iconPath);
            InputStream inputStream = mContext.getContentResolver().openInputStream(imageFileUri);
            return BitmapFactory.decodeStream(inputStream);
        } catch(FileNotFoundException exc) {
            Log.e("NTP", exc.getMessage());
            return null;
        }
    }

    public static boolean shouldEnableNTPFeature(boolean isMoreTabs) {
    	if(Build.VERSION.SDK_INT > Build.VERSION_CODES.M
    		|| (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP && !isMoreTabs)) {
    		return true;
    	}
    	return false;
    }

    public static NTPImage getNTPImage(NTPBackgroundImagesBridge mNTPBackgroundImagesBridge) {
    	NTPBackgroundImagesBridge.Wallpaper mWallpaper = mNTPBackgroundImagesBridge.getCurrentWallpaper();
    	if (mWallpaper != null) {
    		return mWallpaper;
    	} else {
    		return SponsoredImageUtil.getBackgroundImage();
    	}
    }
}
