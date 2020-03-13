/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.view.View;
import android.view.ViewTreeObserver;
import android.content.res.Configuration;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableStringBuilder;

import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp.NewTabPageView;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.ntp_sponsored_images.NTPImage;
import org.chromium.chrome.browser.ntp_sponsored_images.BackgroundImage;
import org.chromium.chrome.browser.ntp_sponsored_images.NewTabPageListener;
import org.chromium.chrome.browser.ntp_sponsored_images.SponsoredImageUtil;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.ntp_sponsored_images.NTPUtil;
import org.chromium.chrome.browser.ntp_sponsored_images.SponsoredTab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.ntp_sponsored_images.NTPSponsoredImagesBridge;

public class BraveNewTabPageView extends NewTabPageView {
    private static final String TAG = "BraveNewTabPageView";

    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";
    private static final short MILLISECONDS_PER_ITEM = 50;
    private static final int BOTTOM_TOOLBAR_HEIGHT = 56;

    private TextView mAdsBlockedCountTextView;
    private TextView mHttpsUpgradesCountTextView;
    private TextView mEstTimeSavedCountTextView;
    private TextView mAdsBlockedTextView;
    private TextView mHttpsUpgradesTextView;
    private TextView mEstTimeSavedTextView;
    private Profile mProfile;

    private TabImpl mTabImpl;
    private Tab mTab;
    private SponsoredTab sponsoredTab;

    private NewTabPageLayout mNewTabPageLayout;
    private SharedPreferences mSharedPreferences;
    private BitmapDrawable imageDrawable;

    private boolean isFromBottomSheet;

    private NTPSponsoredImagesBridge mNTPSponsoredImagesBridge;

    // TODO - call NTPSponsoredImagesBridge.getCurrentWallpaper
    // if null then display regular background image
    // on every NTP load call NTPSponsoredImagesBridge.registerPageView()
    public BraveNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedProfile();
        mNewTabPageLayout = getNewTabPageLayout();
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
        mNTPSponsoredImagesBridge = NTPSponsoredImagesBridge.getInstance(mProfile);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            NTPImage ntpImage = sponsoredTab.getTabNTPImage();
            if(ntpImage == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            } else if (ntpImage instanceof NTPSponsoredImagesBridge.Wallpaper) {
                NTPSponsoredImagesBridge.Wallpaper mWallpaper = (NTPSponsoredImagesBridge.Wallpaper) ntpImage;
                if(mWallpaper == null) {
                    sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
                }
            }
            checkForNonDistruptiveBanner(ntpImage);
            super.onConfigurationChanged(newConfig);
            showNTPImage(ntpImage);
        } else {
            super.onConfigurationChanged(newConfig);
        }
    }

    @Override
    public void initialize(NewTabPageManager manager, Tab tab, TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle, int scrollPosition,
            long constructedTimeNs) {
        super.initialize(manager, tab, tileGroupDelegate,
            searchProviderHasLogo, searchProviderIsGoogle, scrollPosition,
            constructedTimeNs);

        mTabImpl = (TabImpl) tab;
        mTab = tab;

        if (mNTPSponsoredImagesBridge.getCurrentWallpaper() != null) {
            Log.i("NTP", "Wallpaper is not null");
        } else {
            Log.i("NTP", "Wallpaper is null");
        }

        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            if (sponsoredTab == null)
                initilizeSponsoredTab();
            NTPImage ntpImage = sponsoredTab.getTabNTPImage();
            checkForNonDistruptiveBanner(ntpImage);
            showNTPImage(ntpImage);
        } else if(Build.VERSION.SDK_INT <= Build.VERSION_CODES.M && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mTabImpl.addObserver(mTabObserver);
        }
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

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)
            && sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            mAdsBlockedTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mHttpsUpgradesTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mEstTimeSavedTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mEstTimeSavedCountTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
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

    private void showNTPImage(NTPImage ntpImage) {
        NTPUtil.updateOrientedUI(mTabImpl.getActivity(), mNewTabPageLayout);

        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)
            && sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            ViewTreeObserver observer = mNewTabPageLayout.getViewTreeObserver();
            observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    Bitmap wallpaperBitmap = NTPUtil.getWallpaperBitmap(ntpImage, mNewTabPageLayout.getMeasuredWidth(), mNewTabPageLayout.getMeasuredHeight());
                    imageDrawable = new BitmapDrawable(mNewTabPageLayout.getResources(), wallpaperBitmap);
                    mNewTabPageLayout.setBackground(imageDrawable);

                    if (ntpImage instanceof NTPSponsoredImagesBridge.Wallpaper) {
                        NTPSponsoredImagesBridge.Wallpaper mWallpaper = (NTPSponsoredImagesBridge.Wallpaper) ntpImage;
                        if (mWallpaper.getLogoBitmap() != null ) {
                            ImageView sponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
                            sponsoredLogo.setVisibility(View.VISIBLE);
                            sponsoredLogo.setImageBitmap(mWallpaper.getLogoBitmap());
                            sponsoredLogo.setOnClickListener(new View.OnClickListener() {
                                @Override
                                public void onClick(View view) {
                                    if (mWallpaper.getLogoDestinationUrl() != null) {
                                        NTPUtil.openImageCredit(mWallpaper.getLogoDestinationUrl());
                                    }
                                }
                            });
                        }
                    } else {
                        BackgroundImage backgroundImage = (BackgroundImage) ntpImage;
                        ImageView sponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
                        sponsoredLogo.setVisibility(View.GONE);
                        if (backgroundImage.getImageCredit() != null) {
                            String imageCreditStr = String.format(mNewTabPageLayout.getResources().getString(R.string.photo_by, backgroundImage.getImageCredit().getName()));

                            SpannableStringBuilder spannableString = new SpannableStringBuilder(imageCreditStr);
                            spannableString.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), ((imageCreditStr.length()-1) - (backgroundImage.getImageCredit().getName().length()-1)), imageCreditStr.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                            TextView creditText = (TextView)mNewTabPageLayout.findViewById(R.id.credit_text);
                            creditText.setText(spannableString);
                            creditText.setVisibility(View.VISIBLE);
                            creditText.setOnClickListener(new View.OnClickListener() {
                                @Override
                                public void onClick(View view) {
                                    if (backgroundImage.getImageCredit() != null) {
                                        NTPUtil.openImageCredit(backgroundImage.getImageCredit().getUrl());
                                    }
                                }
                            });
                        }
                    }
                    mNewTabPageLayout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                }
            });
        }
    }

    private void checkForNonDistruptiveBanner(NTPImage ntpImage) {
        int brOption = NTPUtil.checkForNonDistruptiveBanner(ntpImage, sponsoredTab);
        if (SponsoredImageUtil.BR_INVALID_OPTION != brOption) {
            NTPUtil.showNonDistruptiveBanner(mTabImpl.getActivity(), mNewTabPageLayout, brOption, sponsoredTab, newTabPageListener);
        }
    }

    private void checkAndShowNTPImage() {
        NTPImage ntpImage = sponsoredTab.getTabNTPImage();
        if(ntpImage == null) {
            sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
        } else if (ntpImage instanceof NTPSponsoredImagesBridge.Wallpaper) {
            NTPSponsoredImagesBridge.Wallpaper mWallpaper = (NTPSponsoredImagesBridge.Wallpaper) ntpImage;
            if(mWallpaper == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            }
        }
        checkForNonDistruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(mTab).get(String.valueOf((mTabImpl).getId())) == null) {
            SponsoredTab mSponsoredTab = new SponsoredTab(mNTPSponsoredImagesBridge);
            TabAttributes.from(mTab).set(String.valueOf((mTabImpl).getId()), mSponsoredTab);
        }
        sponsoredTab = TabAttributes.from(mTab).get(String.valueOf((mTabImpl).getId()));
    }

    private TabObserver mTabObserver = new EmptyTabObserver() {
        @Override
        public void onInteractabilityChanged(boolean interactable) {
            // Force a layout update if the tab is now in the foreground.
            if (interactable) {
                if (sponsoredTab == null)
                    initilizeSponsoredTab();
                if(!sponsoredTab.isMoreTabs()) {
                    checkAndShowNTPImage();
                }
            } else {
                if(!isFromBottomSheet){
                    mNewTabPageLayout.setBackgroundResource(0);
                    if (imageDrawable != null && imageDrawable.getBitmap() != null && !imageDrawable.getBitmap().isRecycled()) {
                        imageDrawable.getBitmap().recycle();
                    }
                }
            }
        }
    };

    private NewTabPageListener newTabPageListener = new NewTabPageListener() {
        @Override
        public void updateInteractableFlag(boolean isBottomSheet) {
            isFromBottomSheet = isBottomSheet;
        }

        @Override
        public void updateNTPImage() {
            if (sponsoredTab == null)
                initilizeSponsoredTab();
            checkAndShowNTPImage();
        }
    };
}
