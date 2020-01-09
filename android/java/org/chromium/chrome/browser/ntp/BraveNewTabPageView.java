/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.view.ViewTreeObserver;
import java.util.Calendar;
import android.content.res.Configuration;
import android.graphics.drawable.GradientDrawable;
import android.graphics.Color;
import android.app.Activity;
import android.os.Build;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Shader;
import android.text.Spannable;
import android.text.SpannableStringBuilder;

import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.base.ContextUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.ntp.NewTabPageView;
import org.chromium.chrome.browser.tabmodel.TabLaunchType;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.BraveTab;
import org.chromium.chrome.browser.preferences.BackgroundImagesPreferences;
import org.chromium.chrome.browser.ntp.sponsored.BackgroundImage;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImage;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;

public class BraveNewTabPageView extends NewTabPageView {
    private static final String TAG = "BraveNewTabPageView";

    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";
    private static final short MILLISECONDS_PER_ITEM = 50;

    private TextView mAdsBlockedCountTextView;
    private TextView mHttpsUpgradesCountTextView;
    private TextView mEstTimeSavedCountTextView;
    private TextView mAdsBlockedTextView;
    private TextView mHttpsUpgradesTextView;
    private TextView mEstTimeSavedTextView;
    private Profile mProfile;
    private SharedPreferences mSharedPreferences;
    private BackgroundImage backgroundImage;

    public BraveNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedProfile();
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        showBackgroundImage();
    }

    @Override
    public void initialize(NewTabPageManager manager, Tab tab, TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle, int scrollPosition,
            long constructedTimeNs) {
        super.initialize(manager, tab, tileGroupDelegate,
            searchProviderHasLogo, searchProviderIsGoogle, scrollPosition,
            constructedTimeNs);

        backgroundImage = ((BraveTab)tab).getTabBackgroundImage();
        showBackgroundImage();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        ViewGroup braveStatsView = (ViewGroup) getNewTabPageLayout().findViewById(R.id.brave_stats);
        mAdsBlockedCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_ads_count);
        mHttpsUpgradesCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_https_count);
        mEstTimeSavedCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_time_count);

        mAdsBlockedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_ads);
        mHttpsUpgradesTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_https);
        mEstTimeSavedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_time);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == VISIBLE) {
            updateBraveStats();
        }
    }

    /**
     * Sets up Brave stats.
     */
    private void updateBraveStats() {
        TraceEvent.begin(TAG + ".updateBraveStats()");
        long trackersBlockedCount = BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long httpsUpgradesCount = BravePrefServiceBridge.getInstance().getHttpsUpgradesCount(mProfile);
        long estimatedMillisecondsSaved = (trackersBlockedCount + adsBlockedCount) * MILLISECONDS_PER_ITEM;

        mAdsBlockedCountTextView.setText(getBraveStatsStringFormNumber(adsBlockedCount));
        mHttpsUpgradesCountTextView.setText(getBraveStatsStringFormNumber(httpsUpgradesCount));
        mEstTimeSavedCountTextView.setText(getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000));

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)) {
            mAdsBlockedTextView.setTextColor(getResources().getColor(android.R.color.white));
            mHttpsUpgradesTextView.setTextColor(getResources().getColor(android.R.color.white));
            mEstTimeSavedTextView.setTextColor(getResources().getColor(android.R.color.white));            
            mEstTimeSavedCountTextView.setTextColor(getResources().getColor(android.R.color.white));
        }

        TraceEvent.end(TAG + ".updateBraveStats()");
    }

    /*
    * Gets string view of specific number for Brave stats
    */
    private String getBraveStatsStringFormNumber(long number) {
        String result = "";
        String suffix = "";
        if (number >= 1000 * 1000 * 1000) {
            result = result + (number / (1000 * 1000 * 1000));
            number = number % (1000 * 1000 * 1000);
            result = result + "." + (number / (10 * 1000 * 1000));
            suffix = "B";
        }
        else if (number >= (10 * 1000 * 1000) && number < (1000 * 1000 * 1000)) {
            result = result + (number / (1000 * 1000));
            suffix = "M";
        }
        else if (number >= (1000 * 1000) && number < (10 * 1000 * 1000)) {
            result = result + (number / (1000 * 1000));
            number = number % (1000 * 1000);
            result = result + "." + (number / (100 * 1000));
            suffix = "M";
        }
        else if (number >= (10 * 1000) && number < (1000 * 1000)) {
            result = result + (number / 1000);
            suffix = "K";
        }
        else if (number >= 1000 && number < (10* 1000)) {
            result = result + (number / 1000);
            number = number % 1000;
            result = result + "." + (number / 100);
            suffix = "K";
        }
        else {
            result = result + number;
        }
        result = result + suffix;
        return result;
    }

    /*
    * Gets string view of specific time in seconds for Brave stats
    */
    private String getBraveStatsStringFromTime(long seconds) {
        String result = "";
        if (seconds > 24 * 60 * 60) {
            result = result + (seconds / (24 * 60 * 60)) + "d";
        }
        else if (seconds > 60 * 60) {
            result = result + (seconds / (60 * 60)) + "h";
        }
        else if (seconds > 60) {
            result = result + (seconds / 60) + "m";
        }
        else {
            result = result + seconds + "s";
        }
        return result;
    }

    private void showBackgroundImage() {
        ViewGroup mNewTabPageLayout = getNewTabPageLayout();
        TextView creditText = (TextView)mNewTabPageLayout.findViewById(R.id.credit_text);

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)
            && Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            ViewTreeObserver observer = mNewTabPageLayout.getViewTreeObserver();
            observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    int layoutWidth = mNewTabPageLayout.getMeasuredWidth();
                    int layoutHeight = mNewTabPageLayout.getMeasuredHeight();
                    BitmapFactory.Options options = new BitmapFactory.Options();
                    options.inScaled = false;
                    Bitmap imageBitmap = BitmapFactory.decodeResource(mNewTabPageLayout.getResources(), backgroundImage.getImageDrawable(), options);
                    float imageWidth = imageBitmap.getWidth();
                    float imageHeight = imageBitmap.getHeight();
                    float centerPoint = backgroundImage.getCenterPoint();
                    float centerRatio = centerPoint / imageWidth;
                    float imageWHRatio = imageWidth / imageHeight;
                    int newImageWidth = (int) (layoutHeight * imageWHRatio);
                    int newImageHeight = layoutHeight;
                    if (newImageWidth < layoutWidth) {
                        // Image is now too small so we need to adjust width and height based on
                        // This covers landscape and strange tablet sizes.
                        float imageHWRatio = imageHeight / imageWidth;
                        newImageWidth = layoutWidth;
                        newImageHeight = (int) (newImageWidth * imageHWRatio);
                    }
                    int newCenter = (int) (newImageWidth * centerRatio);
                    int startX = (int) (newCenter - (layoutWidth / 2));
                    if (newCenter < layoutWidth / 2) {
                        // Need to crop starting at 0 to newImageWidth - left aligned image
                        startX = 0;
                    } else if (newImageWidth - newCenter < layoutWidth / 2) {
                        // Need to crop right side of image - right aligned
                        startX = newImageWidth - layoutWidth;
                    }
                    imageBitmap = Bitmap.createScaledBitmap(imageBitmap, newImageWidth, newImageHeight, true);

                    Bitmap newBitmap = Bitmap.createBitmap(imageBitmap, startX, (newImageHeight - layoutHeight) / 2, layoutWidth, (int) layoutHeight);

                    Bitmap bitmapWithGradient = addGradient(newBitmap, mNewTabPageLayout.getContext().getResources().getColor(R.color.black_alpha_50),Color.TRANSPARENT);

                    imageBitmap.recycle();
                    newBitmap.recycle();

                    // Center vertically, and crop to new center
                    final BitmapDrawable imageDrawable = new BitmapDrawable(mNewTabPageLayout.getResources(), bitmapWithGradient);

                    mNewTabPageLayout.setBackground(imageDrawable);

                    if (backgroundImage.getImageCredit() != null) {
                        if (backgroundImage instanceof SponsoredImage) {
                            mNewTabPageLayout.setOnClickListener(new View.OnClickListener() {
                                @Override
                                public void onClick(View view) {
                                    openImageCredit();
                                }
                            });
                        } else {
                            String imageCreditStr = String.format(mNewTabPageLayout.getResources().getString(R.string.photo_by, backgroundImage.getImageCredit().getName()));

                            SpannableStringBuilder spannableString = new SpannableStringBuilder(imageCreditStr);
                            spannableString.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), ((imageCreditStr.length()-1) - (backgroundImage.getImageCredit().getName().length()-1)), imageCreditStr.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                            creditText.setText(spannableString);
                            creditText.setVisibility(View.VISIBLE);
                            creditText.setOnClickListener(new View.OnClickListener() {
                                @Override
                                public void onClick(View view) {
                                    openImageCredit();
                                }
                            });
                        }
                    } else {
                        creditText.setVisibility(View.GONE);
                    }

                    mNewTabPageLayout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                }
            });
        } else {
            creditText.setVisibility(View.GONE);
        }
    }

    private Bitmap addGradient(Bitmap src, int color1, int color2) {
        int w = src.getWidth();
        int h = src.getHeight();
        Bitmap result = Bitmap.createBitmap(src,0,0,w,h);
        Canvas canvas = new Canvas(result);

        Paint paint = new Paint();
        LinearGradient shader = new LinearGradient(0,0,0,h/3, color1, color2, Shader.TileMode.CLAMP);
        paint.setShader(shader);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DARKEN));
        canvas.drawRect(0,0,w,h/3,paint);

        return result;
    }

    private void openImageCredit() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof ChromeTabbedActivity)) continue;
            ChromeTabbedActivity chromeTabbedActivity =  (ChromeTabbedActivity)ref;
            if (backgroundImage.getImageCredit() != null) {
                LoadUrlParams loadUrlParams = new LoadUrlParams(backgroundImage.getImageCredit().getUrl());
                chromeTabbedActivity.getActivityTab().loadUrl(loadUrlParams);
            } 
        }
    }
}
