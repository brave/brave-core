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
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.view.View;
import android.view.ViewTreeObserver;
import android.content.res.Configuration;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import java.io.File;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import android.net.Uri;

import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp.NewTabPageView;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.ntp.sponsored.NTPImage;
import org.chromium.chrome.browser.ntp.sponsored.BackgroundImage;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImage;
import org.chromium.chrome.browser.ntp.sponsored.NewTabPageListener;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredImageUtil;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.util.LocaleUtils;
import org.chromium.chrome.browser.util.ImageUtils;
import org.chromium.chrome.browser.ntp.sponsored.NTPUtil;
import org.chromium.chrome.browser.ntp.sponsored.SponsoredTab;
import org.chromium.chrome.browser.tab.TabAttributes;

import static org.chromium.chrome.browser.util.ViewUtils.dpToPx;

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

    public BraveNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedProfile();
        mNewTabPageLayout = getNewTabPageLayout();
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            NTPImage ntpImage = sponsoredTab.getTabNTPImage();
            if(ntpImage == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            } else if (ntpImage instanceof SponsoredImage) {
                String countryCode = LocaleUtils.getCountryCode();
                SponsoredImage sponsoredImage = (SponsoredImage) ntpImage;
                File imageFile = new File(PathUtils.getDataDirectory(), countryCode + "_" + sponsoredImage.getImageUrl());
                if(!imageFile.exists()) {
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
                    String countryCode = LocaleUtils.getCountryCode();

                    int layoutWidth = mNewTabPageLayout.getMeasuredWidth();
                    int layoutHeight = mNewTabPageLayout.getMeasuredHeight();
                    BitmapFactory.Options options = new BitmapFactory.Options();
                    options.inScaled = false;
                    options.inJustDecodeBounds = false;

                    Bitmap imageBitmap = null;
                    float imageWidth;
                    float imageHeight;
                    float centerPointX;
                    float centerPointY;

                    if (ntpImage instanceof SponsoredImage) {
                        SponsoredImage sponsoredImage = (SponsoredImage) ntpImage;
                        File imageFile = new File(PathUtils.getDataDirectory(), countryCode + "_" + sponsoredImage.getImageUrl());
                        try {
                            Uri imageFileUri = Uri.parse("file://"+imageFile.getAbsolutePath());
                            InputStream inputStream = mTabImpl.getActivity().getContentResolver().openInputStream(imageFileUri);
                            imageBitmap = BitmapFactory.decodeStream(inputStream);
                            imageWidth = imageBitmap.getWidth();
                            imageHeight = imageBitmap.getHeight();
                            centerPointX = sponsoredImage.getFocalPointX() == 0 ? (imageWidth/2) : sponsoredImage.getFocalPointX();
                            centerPointY = sponsoredImage.getFocalPointY() == 0 ? (imageHeight/2) : sponsoredImage.getFocalPointY();

                            if (SponsoredImageUtil.getSponsoredLogo() != null ) {
                                ImageView sponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
                                sponsoredLogo.setVisibility(View.VISIBLE);
                                File logoFile = new File(PathUtils.getDataDirectory(),countryCode + "_" + SponsoredImageUtil.getSponsoredLogo().getImageUrl());
                                Bitmap logoBitmap = BitmapFactory.decodeFile(logoFile.getPath());
                                sponsoredLogo.setImageBitmap(logoBitmap);
                                sponsoredLogo.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View view) {
                                        if (SponsoredImageUtil.getSponsoredLogo().getDestinationUrl() != null) {
                                            NTPUtil.openImageCredit(SponsoredImageUtil.getSponsoredLogo().getDestinationUrl());
                                        }
                                    }
                                });
                            }
                        } catch (Exception exc) {
                            Log.e("NTP", exc.getMessage());
                            return;
                        }
                    } else {
                        BackgroundImage backgroundImage = (BackgroundImage) ntpImage;

                        ImageView sponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
                        sponsoredLogo.setVisibility(View.GONE);

                        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.M) {
                            Bitmap imageBitmapRes = BitmapFactory.decodeResource(mNewTabPageLayout.getResources(), backgroundImage.getImageDrawable(), options);
                            ByteArrayOutputStream stream = new ByteArrayOutputStream();
                            imageBitmapRes.compress(Bitmap.CompressFormat.JPEG,50,stream);
                            byte[] byteArray = stream.toByteArray();
                            imageBitmap = BitmapFactory.decodeByteArray(byteArray,0,byteArray.length);
                            imageBitmapRes.recycle();
                        } else {
                            imageBitmap = BitmapFactory.decodeResource(mNewTabPageLayout.getResources(), backgroundImage.getImageDrawable(), options);
                        }
                        imageWidth = imageBitmap.getWidth();
                        imageHeight = imageBitmap.getHeight();
                        centerPointX = backgroundImage.getCenterPoint();
                        centerPointY = 0;

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
                    Bitmap bitmapWithGradient = ImageUtils.addGradient(mTabImpl.getActivity(), newBitmap, dpToPx(mTabImpl.getActivity(),BOTTOM_TOOLBAR_HEIGHT));
                    Bitmap offsetBitmap = ImageUtils.topOffset(bitmapWithGradient, dpToPx(mTabImpl.getActivity(),BOTTOM_TOOLBAR_HEIGHT));

                    imageBitmap.recycle();
                    newBitmap.recycle();
                    bitmapWithGradient.recycle();

                    // Center vertically, and crop to new center
                    imageDrawable = new BitmapDrawable(mNewTabPageLayout.getResources(), offsetBitmap);
                    mNewTabPageLayout.setBackground(imageDrawable);

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
        } else if (ntpImage instanceof SponsoredImage) {
            String countryCode = LocaleUtils.getCountryCode();
            SponsoredImage sponsoredImage = (SponsoredImage) ntpImage;
            File imageFile = new File(PathUtils.getDataDirectory(), countryCode + "_" + sponsoredImage.getImageUrl());
            if(!imageFile.exists()) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
                ntpImage = sponsoredTab.getTabNTPImage();
            }
        }
        checkForNonDistruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(mTab).get(String.valueOf((mTabImpl).getId())) == null) {
            SponsoredTab mSponsoredTab = new SponsoredTab();
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
