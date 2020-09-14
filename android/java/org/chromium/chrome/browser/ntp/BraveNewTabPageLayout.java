/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.util.AttributeSet;
import android.util.Pair;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import android.os.CountDownTimer;

import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.viewpager.widget.ViewPager;

import org.json.JSONException;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.custom_layout.VerticalViewPager;
import org.chromium.chrome.browser.explore_sites.ExploreSitesBridge;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.TopSiteTable;
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetAdapter;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetBottomSheetDialogFragment;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetItem;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.model.TopSite;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.ntp_background_images.util.FetchWallpaperWorkerTask;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.offlinepages.DownloadUiActionFlags;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.RequestCoordinatorBridge;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.SiteSection;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class BraveNewTabPageLayout extends NewTabPageLayout {
    private static final String TAG = "BraveNewTabPageView";
    private static final String BRAVE_BINANCE = "https://brave.com/binance/";
    private static final String BRAVE_REF_URL = "https://brave.com/r/";

    private ImageView bgImageView;
    private Profile mProfile;

    private SponsoredTab sponsoredTab;

    private BitmapDrawable imageDrawable;

    private FetchWallpaperWorkerTask mWorkerTask;
    private boolean isFromBottomSheet;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private ViewGroup mainLayout;
    private DatabaseHelper mDatabaseHelper;

    private ViewGroup mSiteSectionView;
    private LottieAnimationView mBadgeAnimationView;
    private VerticalViewPager ntpWidgetViewPager;
    private NTPWidgetAdapter ntpWidgetAdapter;

    private Tab mTab;
    private Activity mActivity;
    private LinearLayout indicatorLayout;
    private LinearLayout superReferralSitesLayout;

    private BinanceNativeWorker mBinanceNativeWorker;
    private CountDownTimer countDownTimer = null;

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedRegularProfile();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        mBinanceNativeWorker = BinanceNativeWorker.getInstance();
        mNTPBackgroundImagesBridge.setNewTabPageListener(newTabPageListener);
        mDatabaseHelper = DatabaseHelper.getInstance();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        indicatorLayout = findViewById(R.id.indicator_layout);

        ntpWidgetViewPager = findViewById(R.id.ntp_widget_view_pager);
        ntpWidgetAdapter = new NTPWidgetAdapter();
        ntpWidgetAdapter.setNTPWidgetListener(ntpWidgetListener);
        ntpWidgetViewPager.setAdapter(ntpWidgetAdapter);

        ntpWidgetViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(
                int position, float positionOffset, int positionOffsetPixels) {}

            @Override
            public void onPageSelected(int position) {
                updateAndShowIndicators(position);
                NTPWidgetManager.getInstance().setNTPWidgetOrder(position);
            }

            @Override
            public void onPageScrollStateChanged(int state) {}
        });
        showWidgetBasedOnOrder();
    }

    private List<NTPWidgetItem> getWidgetList() {
        NTPWidgetManager ntpWidgetManager = NTPWidgetManager.getInstance();
        LayoutInflater inflater =
            (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        Map<Integer, NTPWidgetItem> ntpWidgetMap = new TreeMap<>();
        if (ntpWidgetManager.getPrivateStatsWidget() != -1) {
            View mBraveStatsView = inflater.inflate(R.layout.brave_stats_layout, null);
            mBraveStatsView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    checkForBraveStats();
                }
            });
            NTPWidgetItem ntpWidgetItem = new NTPWidgetItem(NTPWidgetManager.PREF_PRIVATE_STATS,
                    "Privacy Stats",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates.");
            ntpWidgetItem.setWidgetView(mBraveStatsView);
            ntpWidgetMap.put(ntpWidgetManager.getPrivateStatsWidget(), ntpWidgetItem);
        }
        if (ntpWidgetManager.getFavoritesWidget() != -1) {
            View mTopSitesLayout = inflater.inflate(R.layout.top_sites_layout, null);
            FrameLayout mTopSitesGridLayout = mTopSitesLayout.findViewById(R.id.top_sites_grid_layout);

            NTPWidgetItem ntpWidgetItem = new NTPWidgetItem(NTPWidgetManager.PREF_FAVORITES,
                    "Favorites",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates.");
            if (shouldShowSuperReferral() && superReferralSitesLayout != null) {
                mTopSitesGridLayout.addView(superReferralSitesLayout);
                ntpWidgetItem.setWidgetView(mTopSitesLayout);
                ntpWidgetMap.put(ntpWidgetManager.getFavoritesWidget(), ntpWidgetItem);
            } else if (!mNTPBackgroundImagesBridge.isSuperReferral()
                       || !NTPBackgroundImagesBridge.enableSponsoredImages()
                       || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
                boolean showPlaceholder = getTileGroup().hasReceivedData() && getTileGroup().isEmpty();
                if (mSiteSectionView != null
                        && !showPlaceholder) {
                    mTopSitesGridLayout.addView(mSiteSectionView);
                    ntpWidgetItem.setWidgetView(mTopSitesLayout);
                    ntpWidgetMap.put(ntpWidgetManager.getFavoritesWidget(), ntpWidgetItem);
                }
            }
        }
        // if (ntpWidgetManager.getBraveRewardsWidget() != -1) {
        //     View braveRewardsWidgetView =
        //         inflater.inflate(R.layout.brave_rewards_widget_layout, null);
        //     NTPWidgetItem ntpWidgetItem = new NTPWidgetItem(NTPWidgetManager.PREF_BRAVE_REWARDS,
        //             "Brave Rewards",
        //             "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates.");
        //     ntpWidgetItem.setWidgetView(braveRewardsWidgetView);
        //     ntpWidgetMap.put(ntpWidgetManager.getBraveRewardsWidget(), ntpWidgetItem);
        // }
        if (ntpWidgetManager.getBinanceWidget() != -1) {
            View binanceWidgetView = inflater.inflate(R.layout.crypto_widget_layout, null);
            binanceWidgetView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    CryptoWidgetBottomSheetDialogFragment cryptoWidgetBottomSheetDialogFragment =
                        new CryptoWidgetBottomSheetDialogFragment();
                    cryptoWidgetBottomSheetDialogFragment.show(
                        ((BraveActivity) mActivity).getSupportFragmentManager(),
                        "crypto_widget_bottomSheet_dialog_fragment");
                    // Log.e("NTP", "Binance URL : " + mBinanceNativeWorker.getOAuthClientUrl());
                    // TabUtils.openUrlInSameTab(mBinanceNativeWorker.getOAuthClientUrl());
                }
            });
            Button connectButton = binanceWidgetView.findViewById(R.id.btn_connect);
            connectButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Log.e("NTP", "Binance URL : " + mBinanceNativeWorker.getOAuthClientUrl());
                    TabUtils.openUrlInSameTab(mBinanceNativeWorker.getOAuthClientUrl());
                }
            });
            NTPWidgetItem ntpWidgetItem = new NTPWidgetItem(NTPWidgetManager.PREF_BINANCE,
                    "Binance",
                    "Trackers &amp; Ads Blocked, Saved Bandwidth, and Time Saved Estimates.");
            ntpWidgetItem.setWidgetView(binanceWidgetView);
            ntpWidgetMap.put(ntpWidgetManager.getBinanceWidget(), ntpWidgetItem);
        }

        return new ArrayList<NTPWidgetItem>(ntpWidgetMap.values());
    }

    private boolean shouldShowSuperReferral() {
        return mNTPBackgroundImagesBridge.isSuperReferral()
               && NTPBackgroundImagesBridge.enableSponsoredImages()
               && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    }

    private void showWidgetBasedOnOrder() {
        if (ntpWidgetViewPager != null) {
            int selectedOrder = NTPWidgetManager.getInstance().getNTPWidgetOrder();
            ntpWidgetViewPager.setCurrentItem(selectedOrder);
            updateAndShowIndicators(selectedOrder);
        }
    }

    private void showWidgets() {
        if (ntpWidgetAdapter != null) {
            ntpWidgetAdapter.clearWidgets();
            ntpWidgetAdapter.setWidgetList(getWidgetList());
            ntpWidgetAdapter.notifyDataSetChanged();
            showWidgetBasedOnOrder();
        }
    }

    private void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            ((BraveActivity)mActivity).showOnboardingV2(true);
        }
    }

    protected void insertSiteSectionView() {
        mainLayout = findViewById(R.id.ntp_main_layout);

        mSiteSectionView = SiteSection.inflateSiteSection(mainLayout);
        ViewGroup.LayoutParams layoutParams = mSiteSectionView.getLayoutParams();
        layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        // If the explore sites section exists as its own section, then space it more closely.
        int variation = ExploreSitesBridge.getVariation();
        if (ExploreSitesBridge.isEnabled(variation)) {
            ((MarginLayoutParams) layoutParams).bottomMargin =
                getResources().getDimensionPixelOffset(
                    R.dimen.tile_grid_layout_vertical_spacing);
        }
        mSiteSectionView.setLayoutParams(layoutParams);
    }

    protected int getMaxRowsForMostVisitedTiles() {
        if (UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                && NTPUtil.shouldEnableNTPFeature()) {
            return 1;
        } else {
            return 2;
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (sponsoredTab == null) {
            initilizeSponsoredTab();
        }
        if (getPlaceholder() != null
                && ((ViewGroup)getPlaceholder().getParent()) != null) {
            ((ViewGroup)getPlaceholder().getParent()).removeView(getPlaceholder());
        }
        checkAndShowNTPImage(false);
        mNTPBackgroundImagesBridge.addObserver(mNTPBackgroundImageServiceObserver);
        if (PackageUtils.isFirstInstall(mActivity)
                && !OnboardingPrefManager.getInstance().isNewOnboardingShown()) {
            ((BraveActivity)mActivity).showOnboardingV2(false);
        }
        if (OnboardingPrefManager.getInstance().isFromNotification() ) {
            ((BraveActivity)mActivity).showOnboardingV2(false);
            OnboardingPrefManager.getInstance().setFromNotification(false);
        }
        if (mBadgeAnimationView != null
                && !OnboardingPrefManager.getInstance().shouldShowBadgeAnimation()) {
            mBadgeAnimationView.setVisibility(View.INVISIBLE);
        }
        showWidgets();
        if (BinanceWidgetManager.getInstance().isUserAuthenticatedForBinance()) {
            mBinanceNativeWorker.getAccountBalances();
        }
        mBinanceNativeWorker.AddObserver(mBinanaceObserver);
        startTimer();
    }

    @Override
    protected void onDetachedFromWindow() {
        if (mWorkerTask != null && mWorkerTask.getStatus() == AsyncTask.Status.RUNNING) {
            mWorkerTask.cancel(true);
            mWorkerTask = null;
        }

        if (!isFromBottomSheet) {
            setBackgroundResource(0);
            if (imageDrawable != null && imageDrawable.getBitmap() != null && !imageDrawable.getBitmap().isRecycled()) {
                imageDrawable.getBitmap().recycle();
            }
        }
        mNTPBackgroundImagesBridge.removeObserver(mNTPBackgroundImageServiceObserver);
        mBinanceNativeWorker.RemoveObserver(mBinanaceObserver);
        cancelTimer();
        super.onDetachedFromWindow();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (sponsoredTab != null && NTPUtil.shouldEnableNTPFeature()) {
            if (bgImageView != null) {
                // We need to redraw image to fit parent properly
                bgImageView.setImageResource(android.R.color.transparent);
            }
            NTPImage ntpImage = sponsoredTab.getTabNTPImage(false);
            if (ntpImage == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            } else if (ntpImage instanceof Wallpaper) {
                Wallpaper mWallpaper = (Wallpaper) ntpImage;
                if (mWallpaper == null) {
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
    public void initialize(NewTabPageManager manager, Activity activity,
            TileGroup.Delegate tileGroupDelegate, boolean searchProviderHasLogo,
            boolean searchProviderIsGoogle, ScrollDelegate scrollDelegate,
            ContextMenuManager contextMenuManager, UiConfig uiConfig, Supplier<Tab> tabProvider,
            ActivityLifecycleDispatcher lifecycleDispatcher, NewTabPageUma uma) {
        super.initialize(manager, activity, tileGroupDelegate, searchProviderHasLogo,
                searchProviderIsGoogle, scrollDelegate, contextMenuManager, uiConfig, tabProvider,
                lifecycleDispatcher, uma);

        assert (activity instanceof BraveActivity);
        mActivity = activity;
        ((BraveActivity)mActivity).dismissShieldsTooltip();
    }

    private void showNTPImage(NTPImage ntpImage) {
        NTPUtil.updateOrientedUI(mActivity, this);
        ImageView mSponsoredLogo = (ImageView) findViewById(R.id.sponsored_logo);
        FloatingActionButton mSuperReferralLogo = (FloatingActionButton) findViewById(R.id.super_referral_logo);
        TextView mCreditText = (TextView) findViewById(R.id.credit_text);
        if (ntpImage instanceof Wallpaper
                && NTPUtil.isReferralEnabled()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            setBackgroundImage(ntpImage);
            mSuperReferralLogo.setVisibility(View.VISIBLE);
            mCreditText.setVisibility(View.GONE);
            int floatingButtonIcon =
                GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                ? R.drawable.qrcode_dark
                : R.drawable.qrcode_light;
            mSuperReferralLogo.setImageResource(floatingButtonIcon);
            mSuperReferralLogo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    QRCodeShareDialogFragment mQRCodeShareDialogFragment =
                        new QRCodeShareDialogFragment();
                    mQRCodeShareDialogFragment.setQRCodeText(BRAVE_REF_URL + mNTPBackgroundImagesBridge.getSuperReferralCode());
                    mQRCodeShareDialogFragment.show(
                        ((BraveActivity) mActivity).getSupportFragmentManager(),
                        "QRCodeShareDialogFragment");
                }
            });
        } else if (UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(
                       BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                   && sponsoredTab != null
                   && NTPUtil.shouldEnableNTPFeature()) {
            setBackgroundImage(ntpImage);
            if (ntpImage instanceof BackgroundImage) {
                BackgroundImage backgroundImage = (BackgroundImage) ntpImage;
                mSponsoredLogo.setVisibility(View.GONE);
                mSuperReferralLogo.setVisibility(View.GONE);
                if (backgroundImage.getImageCredit() != null) {
                    String imageCreditStr = String.format(getResources().getString(R.string.photo_by, backgroundImage.getImageCredit().getName()));

                    SpannableStringBuilder spannableString = new SpannableStringBuilder(imageCreditStr);
                    spannableString.setSpan(
                        new android.text.style.StyleSpan(android.graphics.Typeface.BOLD),
                        ((imageCreditStr.length() - 1)
                         - (backgroundImage.getImageCredit().getName().length() - 1)),
                        imageCreditStr.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                    mCreditText.setText(spannableString);
                    mCreditText.setVisibility(View.VISIBLE);
                    mCreditText.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (backgroundImage.getImageCredit() != null) {
                                TabUtils.openUrlInSameTab(backgroundImage.getImageCredit().getUrl());
                            }
                        }
                    });
                }
            }
        }
    }

    private void setBackgroundImage(NTPImage ntpImage) {
        bgImageView = (ImageView) findViewById(R.id.bg_image_view);
        bgImageView.setScaleType(ImageView.ScaleType.MATRIX);

        ViewTreeObserver observer = bgImageView.getViewTreeObserver();
        observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                mWorkerTask = new FetchWallpaperWorkerTask(ntpImage, bgImageView.getMeasuredWidth(), bgImageView.getMeasuredHeight(), wallpaperRetrievedCallback);
                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

                bgImageView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }
        });
    }

    private void checkForNonDistruptiveBanner(NTPImage ntpImage) {
        int brOption = NTPUtil.checkForNonDistruptiveBanner(ntpImage, sponsoredTab);
        if (SponsoredImageUtil.BR_INVALID_OPTION != brOption && !NTPUtil.isReferralEnabled()) {
            NTPUtil.showNonDistruptiveBanner((BraveActivity) mActivity, this, brOption,
                                             sponsoredTab, newTabPageListener);
        }
    }

    private void checkAndShowNTPImage(boolean isReset) {
        NTPImage ntpImage = sponsoredTab.getTabNTPImage(isReset);
        if (ntpImage == null) {
            sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
        } else if (ntpImage instanceof Wallpaper) {
            Wallpaper mWallpaper = (Wallpaper) ntpImage;
            if (mWallpaper == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            }
        }
        checkForNonDistruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(getTab()).get(String.valueOf(getTabImpl().getId())) == null) {
            SponsoredTab mSponsoredTab = new SponsoredTab(mNTPBackgroundImagesBridge);
            TabAttributes.from(getTab()).set(String.valueOf(getTabImpl().getId()), mSponsoredTab);
        }
        sponsoredTab = TabAttributes.from(getTab()).get(String.valueOf((getTabImpl()).getId()));
        if (shouldShowSuperReferral())
            mNTPBackgroundImagesBridge.getTopSites();
    }

    private NewTabPageListener newTabPageListener = new NewTabPageListener() {
        @Override
        public void updateInteractableFlag(boolean isBottomSheet) {
            isFromBottomSheet = isBottomSheet;
        }

        @Override
        public void updateNTPImage() {
            if (sponsoredTab == null) {
                initilizeSponsoredTab();
            }
            checkAndShowNTPImage(false);
        }

        @Override
        public void updateTopSites(List<TopSite> topSites) {
            Log.e("NTP", "Inside updateTopSites");
            new AsyncTask<List<TopSiteTable>>() {
                @Override
                protected List<TopSiteTable> doInBackground() {
                    for (TopSite topSite : topSites) {
                        mDatabaseHelper.insertTopSite(topSite);
                    }
                    return mDatabaseHelper.getAllTopSites();
                }

                @Override
                protected void onPostExecute(List<TopSiteTable> topSites) {
                    assert ThreadUtils.runningOnUiThread();
                    if (isCancelled()) return;

                    loadTopSites(topSites);
                }
            } .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    };

    private NTPBackgroundImagesBridge.NTPBackgroundImageServiceObserver mNTPBackgroundImageServiceObserver = new NTPBackgroundImagesBridge.NTPBackgroundImageServiceObserver() {
        @Override
        public void onUpdated() {
            if (NTPUtil.isReferralEnabled()) {
                checkAndShowNTPImage(true);
                if (shouldShowSuperReferral()) {
                    mNTPBackgroundImagesBridge.getTopSites();
                }
            }
        }
    };

    private FetchWallpaperWorkerTask.WallpaperRetrievedCallback wallpaperRetrievedCallback = new FetchWallpaperWorkerTask.WallpaperRetrievedCallback() {
        @Override
        public void bgWallpaperRetrieved(Bitmap bgWallpaper) {
            bgImageView.setImageBitmap(bgWallpaper);
        }

        @Override
        public void logoRetrieved(Wallpaper mWallpaper, Bitmap logoWallpaper) {
            if (!NTPUtil.isReferralEnabled()) {
                FloatingActionButton mSuperReferralLogo = (FloatingActionButton) findViewById(R.id.super_referral_logo);
                mSuperReferralLogo.setVisibility(View.GONE);

                ImageView sponsoredLogo = (ImageView) findViewById(R.id.sponsored_logo);
                sponsoredLogo.setVisibility(View.VISIBLE);
                sponsoredLogo.setImageBitmap(logoWallpaper);
                sponsoredLogo.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (mWallpaper.getLogoDestinationUrl() != null) {
                            TabUtils.openUrlInSameTab(mWallpaper.getLogoDestinationUrl());
                        }
                    }
                });
            }
        }
    };

    private void loadTopSites(List<TopSiteTable> topSites) {
        superReferralSitesLayout = new LinearLayout(mActivity);
        superReferralSitesLayout.setWeightSum(1f);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT);
        params.setMargins(16, 16, 16, 16);
        superReferralSitesLayout.setLayoutParams(params);
        superReferralSitesLayout.setOrientation(LinearLayout.HORIZONTAL);
        superReferralSitesLayout.setBackgroundColor(mActivity.getResources().getColor(R.color.topsite_bg_color));

        LayoutInflater inflater = (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        for (TopSiteTable topSite : topSites) {
            final View view = inflater.inflate(R.layout.suggestions_tile_view, null);

            TextView tileViewTitleTv = view.findViewById(R.id.tile_view_title);
            tileViewTitleTv.setText(topSite.getName());
            tileViewTitleTv.setTextColor(getResources().getColor(android.R.color.black));

            ImageView iconIv = view.findViewById(R.id.tile_view_icon);
            if (NTPUtil.imageCache.get(topSite.getDestinationUrl()) == null) {
                NTPUtil.imageCache.put(topSite.getDestinationUrl(), new java.lang.ref.SoftReference(NTPUtil.getTopSiteBitmap(topSite.getImagePath())));
            }
            iconIv.setImageBitmap(NTPUtil.imageCache.get(topSite.getDestinationUrl()).get());
            iconIv.setBackgroundColor(mActivity.getResources().getColor(android.R.color.white));
            iconIv.setClickable(false);

            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    TabUtils.openUrlInSameTab(topSite.getDestinationUrl());
                }
            });

            int paddingTop = getResources().getDimensionPixelSize(R.dimen.tile_grid_layout_no_logo_padding_top);
            view.setPadding(0, paddingTop, 0, view.getPaddingBottom());

            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT);
            layoutParams.weight = 0.25f;
            layoutParams.gravity = Gravity.CENTER;
            view.setLayoutParams(layoutParams);
            view.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
                @Override
                public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                    menu.add(R.string.contextmenu_open_in_new_tab).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            TabUtils.openUrlInNewTab(false, topSite.getDestinationUrl());
                            return true;
                        }
                    });
                    menu.add(R.string.contextmenu_open_in_incognito_tab).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            TabUtils.openUrlInNewTab(true, topSite.getDestinationUrl());
                            return true;
                        }
                    });
                    menu.add(R.string.contextmenu_save_link).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            if (getTab() != null) {
                                OfflinePageBridge.getForProfile(mProfile).scheduleDownload(getTab().getWebContents(),
                                        OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE, topSite.getDestinationUrl(), DownloadUiActionFlags.ALL);
                            } else {
                                RequestCoordinatorBridge.getForProfile(mProfile).savePageLater(
                                    topSite.getDestinationUrl(), OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE, true /* userRequested */);
                            }
                            return true;
                        }
                    });
                    menu.add(R.string.remove).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            NTPUtil.imageCache.remove(topSite.getDestinationUrl());
                            mDatabaseHelper.deleteTopSite(topSite.getDestinationUrl());
                            NTPUtil.addToRemovedTopSite(topSite.getDestinationUrl());
                            superReferralSitesLayout.removeView(view);
                            return true;
                        }
                    });
                }
            });
            superReferralSitesLayout.addView(view);
        }
        showWidgets();
    }

    public void setTab(Tab tab) {
        mTab = tab;
    }

    private Tab getTab() {
        assert mTab != null;
        return mTab;
    }

    private TabImpl getTabImpl() {
        return (TabImpl) getTab();
    }

    private void updateAndShowIndicators(int position) {
        indicatorLayout.removeAllViews();
        for (int i = 0; i < ntpWidgetAdapter.getCount(); i++) {
            TextView dotTextView = new TextView(mActivity);
            dotTextView.setText(Html.fromHtml("&#9679;"));
            dotTextView.setTextColor(getResources().getColor(android.R.color.white));
            dotTextView.setTextSize(8);
            if (position == i) {
                dotTextView.setAlpha(1.0f);
            } else {
                dotTextView.setAlpha(0.4f);
            }
            indicatorLayout.addView(dotTextView);
        }
    }

    // NTP related methods
    private NTPWidgetAdapter.NTPWidgetListener ntpWidgetListener =
    new NTPWidgetAdapter.NTPWidgetListener() {
        @Override
        public void onMenuEdit() {
            NTPWidgetBottomSheetDialogFragment ntpWidgetBottomSheetDialogFragment =
                NTPWidgetBottomSheetDialogFragment.newInstance();
            ntpWidgetBottomSheetDialogFragment.setNTPWidgetListener(ntpWidgetListener);
            ntpWidgetBottomSheetDialogFragment.setWidgetList(getWidgetList());
            ntpWidgetBottomSheetDialogFragment.show(
                ((BraveActivity) mActivity).getSupportFragmentManager(),
                "NTPWidgetBottomSheetDialogFragment");
        }

        @Override
        public void onMenuRemove(int position) {
            ntpWidgetAdapter.removeWidgetItem(position);
            ntpWidgetAdapter.notifyDataSetChanged();
            showWidgetBasedOnOrder();
        }

        @Override
        public void onMenuLearnMore() {
            TabUtils.openUrlInSameTab(BRAVE_BINANCE);
        }

        @Override
        public void onMenuRefreshData() {
            mBinanceNativeWorker.getAccountBalances();
        }

        @Override
        public void onMenuDisconnect() {
            mBinanceNativeWorker.revokeToken();
        }

        @Override
        public void onBottomSheetDismiss() {
            showWidgets();
        }
    };

    private BinanceObserver mBinanaceObserver = new BinanceObserver() {
        @Override
        public void OnGetAccessToken(boolean isSuccess) {
            Log.e("NTP", "OnGetAccessToken : " + isSuccess);
            BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(isSuccess);
            if (isSuccess) {
                mBinanceNativeWorker.getAccountBalances();
            }
        };

        @Override
        public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {
            if (!isSuccess) {
                BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(isSuccess);
            }
            try {
                BinanceWidgetManager.getInstance().setBinanceAccountBalance(new BinanceAccountBalance(jsonBalances));
                // Log.e("NTP", "AccountBalances : " + BinanceWidgetManager.getInstance().getBinanceAccountBalance().toString());
            } catch (JSONException e) {
                Log.e("NTP", e.getMessage());
            }
            // Reset binance widget to connect page
            ntpWidgetAdapter.notifyDataSetChanged();

            // mBinanceNativeWorker.getCoinNetworks();
            // mBinanceNativeWorker.getConvertAssets();
        };

        @Override
        public void OnGetConvertQuote(String quoteId, String quotePrice, String totalFee, String totalAmount) {

        };

        @Override
        public void OnGetCoinNetworks(String jsonNetworks) {
            // Log.e("NTP", "CoinNetworks : " + jsonNetworks);
        };

        @Override
        public void OnGetDepositInfo(String depositAddress, String depositTag, boolean isSuccess) {};

        @Override
        public void OnConfirmConvert(boolean isSuccess, String message) {};

        @Override
        public void OnGetConvertAssets(String jsonAssets) {
            // Log.e("NTP", "ConvertAssets : " + jsonAssets);
        };

        @Override
        public void OnRevokeToken(boolean isSuccess) {
            BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(!isSuccess);
            // Reset binance widget to connect page
            ntpWidgetAdapter.notifyDataSetChanged();
        };
    };

    //start timer function
    public void startTimer() {
        countDownTimer = new CountDownTimer(30000, 1000) {
            public void onTick(long millisUntilFinished) {
            }
            public void onFinish() {
                if (BinanceWidgetManager.getInstance().isUserAuthenticatedForBinance()) {
                    mBinanceNativeWorker.getAccountBalances();
                }
                if ((BraveActivity)mActivity != null)
                    startTimer();
            }
        };
        countDownTimer.start();
    }


    //cancel timer
    public void cancelTimer() {
        if (countDownTimer != null)
            countDownTimer.cancel();
    }
}
