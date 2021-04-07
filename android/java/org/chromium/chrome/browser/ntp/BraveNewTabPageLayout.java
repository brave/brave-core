/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Pair;
import android.view.ContextMenu;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.ViewPager;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;
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
import org.chromium.chrome.browser.ntp.widget.NTPWidgetItem;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetStackActivity;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
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
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;

public class BraveNewTabPageLayout extends NewTabPageLayout {
    private static final String TAG = "BraveNewTabPageView";
    private static final String BRAVE_BINANCE = "https://brave.com/binance/";
    private static final String BRAVE_REF_URL = "https://brave.com/r/";

    private View mBraveStatsViewFallBackLayout;

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
    private LinearLayout ntpWidgetLayout;
    private LinearLayout bianceDisconnectLayout;
    private LinearLayout binanceWidgetLayout;
    private ProgressBar binanceWidgetProgress;
    private TextView mTopsiteErrorMessage;

    private BinanceNativeWorker mBinanceNativeWorker;
    private CryptoWidgetBottomSheetDialogFragment cryptoWidgetBottomSheetDialogFragment;
    private CountDownTimer countDownTimer;
    private List<NTPWidgetItem> widgetList = new ArrayList<NTPWidgetItem>();
    public static final int NTP_WIDGET_STACK_CODE = 3333;

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
        ntpWidgetLayout = findViewById(R.id.ntp_widget_layout);
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
                cancelTimer();
                if (NTPWidgetManager.getInstance().getBinanceWidget() == position) {
                    startTimer();
                }
                updateAndShowIndicators(position);
                NTPWidgetManager.getInstance().setNTPWidgetOrder(position);
            }

            @Override
            public void onPageScrollStateChanged(int state) {}
        });
        showWidgetBasedOnOrder();
        NTPUtil.showBREBottomBanner(this);
    }

    private void showFallBackNTPLayout() {
        if (mBraveStatsViewFallBackLayout != null
                && mBraveStatsViewFallBackLayout.getParent() != null) {
            ((ViewGroup) mBraveStatsViewFallBackLayout.getParent())
                    .removeView(mBraveStatsViewFallBackLayout);
        }
        LayoutInflater inflater =
                (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mBraveStatsViewFallBackLayout = inflater.inflate(R.layout.brave_stats_layout, null);
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
            new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        layoutParams.setMargins(0, dpToPx(mActivity, 16), 0, dpToPx(mActivity, 16));
        mBraveStatsViewFallBackLayout.setLayoutParams(layoutParams);
        mBraveStatsViewFallBackLayout.requestLayout();

        mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_title_layout)
                .setVisibility(View.GONE);
        ((TextView) mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_text_ads))
                .setTextColor(mActivity.getResources().getColor(R.color.shield_text_color));
        ((TextView) mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_data_saved_text))
                .setTextColor(mActivity.getResources().getColor(R.color.shield_text_color));
        ((TextView) mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_text_time))
                .setTextColor(mActivity.getResources().getColor(R.color.shield_text_color));
        ((TextView) mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_text_time_count))
                .setTextColor(mActivity.getResources().getColor(R.color.shield_text_color));
        ((TextView) mBraveStatsViewFallBackLayout.findViewById(
                 R.id.brave_stats_text_time_count_text))
                .setTextColor(mActivity.getResources().getColor(R.color.shield_text_color));
        mBraveStatsViewFallBackLayout.setBackgroundColor(
                mActivity.getResources().getColor(android.R.color.transparent));
        mBraveStatsViewFallBackLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                checkForBraveStats();
            }
        });
        BraveStatsUtil.updateBraveStatsLayout(mBraveStatsViewFallBackLayout);
        mainLayout.addView(mBraveStatsViewFallBackLayout, 0);

        int insertionPoint = mainLayout.indexOfChild(findViewById(R.id.ntp_middle_spacer)) + 1;
        if (mSiteSectionView.getParent() != null) {
            ((ViewGroup) mSiteSectionView.getParent()).removeView(mSiteSectionView);
        }
        mainLayout.addView(mSiteSectionView, insertionPoint);
    }

    private List<NTPWidgetItem> setWidgetList() {
        NTPWidgetManager ntpWidgetManager = NTPWidgetManager.getInstance();
        LayoutInflater inflater =
                (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        Map<Integer, NTPWidgetItem> ntpWidgetMap = new TreeMap<>();
        if (mSiteSectionView != null && mSiteSectionView.getParent() != null) {
            ((ViewGroup) mSiteSectionView.getParent()).removeView(mSiteSectionView);
        }

        for (String widget : ntpWidgetManager.getUsedWidgets()) {
            NTPWidgetItem ntpWidgetItem = NTPWidgetManager.mWidgetsMap.get(widget);
            if (widget.equals(NTPWidgetManager.PREF_PRIVATE_STATS)) {
                View mBraveStatsView = inflater.inflate(R.layout.brave_stats_layout, null);
                mBraveStatsView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                        checkForBraveStats();
                    }
                });
                ntpWidgetItem.setWidgetView(mBraveStatsView);
                ntpWidgetMap.put(ntpWidgetManager.getPrivateStatsWidget(), ntpWidgetItem);
            } else if (widget.equals(NTPWidgetManager.PREF_FAVORITES)) {
                View mTopSitesLayout = inflater.inflate(R.layout.top_sites_layout, null);
                FrameLayout mTopSitesGridLayout =
                        mTopSitesLayout.findViewById(R.id.top_sites_grid_layout);
                mTopsiteErrorMessage =
                        mTopSitesLayout.findViewById(R.id.widget_error_title);

                if (shouldShowSuperReferral() && superReferralSitesLayout != null) {
                    if (superReferralSitesLayout.getParent() != null) {
                        ((ViewGroup) superReferralSitesLayout.getParent())
                                .removeView(superReferralSitesLayout);
                    }
                    mTopSitesGridLayout.addView(superReferralSitesLayout);
                    ntpWidgetItem.setWidgetView(mTopSitesLayout);
                    ntpWidgetMap.put(ntpWidgetManager.getFavoritesWidget(), ntpWidgetItem);
                } else if (!mNTPBackgroundImagesBridge.isSuperReferral()
                        || !NTPBackgroundImagesBridge.enableSponsoredImages()
                        || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
                    boolean showPlaceholder =
                            getTileGroup().hasReceivedData() && getTileGroup().isEmpty();
                    if (mSiteSectionView != null && !showPlaceholder) {
                        mTopsiteErrorMessage.setVisibility(View.GONE);
                        if (mSiteSectionView.getLayoutParams()
                                        instanceof ViewGroup.MarginLayoutParams) {
                            mSiteSectionView.setPadding(0, dpToPx(mActivity, 8), 0, 0);
                            mSiteSectionView.requestLayout();
                        }
                        mTopSitesGridLayout.addView(mSiteSectionView);
                    } else {
                        mTopsiteErrorMessage.setVisibility(View.VISIBLE);
                    }
                    ntpWidgetItem.setWidgetView(mTopSitesLayout);
                    ntpWidgetMap.put(ntpWidgetManager.getFavoritesWidget(), ntpWidgetItem);
                }
            } else if (widget.equals(NTPWidgetManager.PREF_BINANCE)) {
                View binanceWidgetView = inflater.inflate(R.layout.crypto_widget_layout, null);
                binanceWidgetLayout = binanceWidgetView.findViewById(R.id.binance_widget_layout);
                bianceDisconnectLayout =
                        binanceWidgetView.findViewById(R.id.binance_disconnect_layout);
                binanceWidgetProgress =
                        binanceWidgetView.findViewById(R.id.binance_widget_progress);
                binanceWidgetProgress.setVisibility(View.GONE);
                binanceWidgetView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (InternetConnection.isNetworkAvailable(mActivity)) {
                            if (BinanceWidgetManager.getInstance()
                                            .isUserAuthenticatedForBinance()) {
                                cancelTimer();
                                cryptoWidgetBottomSheetDialogFragment =
                                        new CryptoWidgetBottomSheetDialogFragment();
                                cryptoWidgetBottomSheetDialogFragment.show(
                                        ((BraveActivity) mActivity).getSupportFragmentManager(),
                                        CryptoWidgetBottomSheetDialogFragment.TAG_FRAGMENT);
                            } else {
                                TabUtils.openUrlInSameTab(mBinanceNativeWorker.getOAuthClientUrl());
                                bianceDisconnectLayout.setVisibility(View.GONE);
                                binanceWidgetProgress.setVisibility(View.VISIBLE);
                            }
                        } else {
                            Toast.makeText(mActivity,
                                         mActivity.getResources().getString(
                                                 R.string.please_check_the_connection),
                                         Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }
                });
                Button connectButton = binanceWidgetView.findViewById(R.id.btn_connect);
                connectButton.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        TabUtils.openUrlInSameTab(mBinanceNativeWorker.getOAuthClientUrl());
                        bianceDisconnectLayout.setVisibility(View.GONE);
                        binanceWidgetProgress.setVisibility(View.VISIBLE);
                    }
                });
                ntpWidgetItem.setWidgetView(binanceWidgetView);
                ntpWidgetMap.put(ntpWidgetManager.getBinanceWidget(), ntpWidgetItem);
            }
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
            ntpWidgetViewPager.setCurrentItem(selectedOrder, true);
            updateAndShowIndicators(selectedOrder);
        }
    }

    private void showWidgets() {
        List<NTPWidgetItem> tempList = setWidgetList();
        if (tempList.size() > 0) {
            ntpWidgetLayout.setVisibility(View.VISIBLE);
            if (mBraveStatsViewFallBackLayout != null
                    && mBraveStatsViewFallBackLayout.getParent() != null) {
                ((ViewGroup) mBraveStatsViewFallBackLayout.getParent())
                        .removeView(mBraveStatsViewFallBackLayout);
            }
        } else {
            ntpWidgetLayout.setVisibility(View.GONE);
            if (!UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                showFallBackNTPLayout();
            }
        }

        if (ntpWidgetAdapter != null) {
            ntpWidgetAdapter.setWidgetList(tempList);
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
                && !OnboardingPrefManager.getInstance().isNewOnboardingShown()
                && OnboardingPrefManager.getInstance().isP3aOnboardingShown()) {
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
            if (binanceWidgetLayout != null) {
                binanceWidgetLayout.setVisibility(View.GONE);
            }
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
            checkForNonDisruptiveBanner(ntpImage);
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
            ActivityLifecycleDispatcher lifecycleDispatcher, NewTabPageUma uma, boolean isIncognito,
            WindowAndroid windowAndroid) {
        super.initialize(manager, activity, tileGroupDelegate, searchProviderHasLogo,
                searchProviderIsGoogle, scrollDelegate, contextMenuManager, uiConfig, tabProvider,
                lifecycleDispatcher, uma, isIncognito, windowAndroid);

        assert (activity instanceof BraveActivity);
        mActivity = activity;
        ((BraveActivity) mActivity).dismissShieldsTooltip();
    }

    private void showNTPImage(NTPImage ntpImage) {
        Display display = mActivity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        NTPUtil.updateOrientedUI(mActivity, this, size);
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
                    mQRCodeShareDialogFragment.setQRCodeText(
                            BRAVE_REF_URL + mNTPBackgroundImagesBridge.getSuperReferralCode());
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
                                TabUtils.openUrlInSameTab(
                                        backgroundImage.getImageCredit().getUrl());
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

    private void checkForNonDisruptiveBanner(NTPImage ntpImage) {
        int brOption = NTPUtil.checkForNonDisruptiveBanner(ntpImage, sponsoredTab);
        if (SponsoredImageUtil.BR_INVALID_OPTION != brOption && !NTPUtil.isReferralEnabled()
                && ((!BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                             Profile.getLastUsedRegularProfile())
                            && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal())
                        || BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                                Profile.getLastUsedRegularProfile()))) {
            NTPUtil.showNonDisruptiveBanner((BraveActivity) mActivity, this, brOption,
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
        checkForNonDisruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(getTab()).get(String.valueOf(getTabImpl().getId())) == null) {
            SponsoredTab mSponsoredTab = new SponsoredTab(mNTPBackgroundImagesBridge);
            TabAttributes.from(getTab()).set(String.valueOf(getTabImpl().getId()), mSponsoredTab);
        }
        sponsoredTab = TabAttributes.from(getTab()).get(String.valueOf((getTabImpl()).getId()));
        if (shouldShowSuperReferral()) mNTPBackgroundImagesBridge.getTopSites();
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
            }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
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
                            mNTPBackgroundImagesBridge.wallpaperLogoClicked(mWallpaper);
                        }
                    }
                });
            }
        }
    };

    private void loadTopSites(List<TopSiteTable> topSites) {
        superReferralSitesLayout = new LinearLayout(mActivity);
        superReferralSitesLayout.setWeightSum(1f);
        superReferralSitesLayout.setOrientation(LinearLayout.HORIZONTAL);
        superReferralSitesLayout.setBackgroundColor(
                mActivity.getResources().getColor(R.color.topsite_bg_color));

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

            view.setPadding(0, dpToPx(mActivity, 12), 0, 0);

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
                    cancelTimer();
                    openWidgetStack();
                }

                @Override
                public void onMenuRemove(int position, boolean isBinanceWidget) {
                    if (isBinanceWidget) {
                        mBinanceNativeWorker.revokeToken();
                        BinanceWidgetManager.getInstance().setBinanceAccountBalance("");
                        BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(false);
                        if (cryptoWidgetBottomSheetDialogFragment != null) {
                            cryptoWidgetBottomSheetDialogFragment.dismiss();
                        }
                    }

                    if (BraveActivity.getBraveActivity() != null 
                        && BraveActivity.getBraveActivity().getActivityTab() != null 
                        && !UserPrefs.get(Profile.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                        && NTPWidgetManager.getInstance().getUsedWidgets().size() <= 0) {
                        BraveActivity.getBraveActivity().getActivityTab().reloadIgnoringCache();
                    } else {
                        showWidgets();
                    }
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
                    BinanceWidgetManager.getInstance().setBinanceAccountBalance("");
                    BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(false);
                    if (cryptoWidgetBottomSheetDialogFragment != null) {
                        cryptoWidgetBottomSheetDialogFragment.dismiss();
                    }
                    // Reset binance widget to connect page
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
                if (bianceDisconnectLayout != null) {
                    bianceDisconnectLayout.setVisibility(View.GONE);
                }
                if (binanceWidgetProgress != null) {
                    binanceWidgetProgress.setVisibility(View.VISIBLE);
                }
            }
        };

        @Override
        public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {
            if (InternetConnection.isNetworkAvailable(mActivity)) {
                if (!isSuccess) {
                    BinanceWidgetManager.getInstance().setUserAuthenticationForBinance(isSuccess);
                    if (cryptoWidgetBottomSheetDialogFragment != null) {
                        cryptoWidgetBottomSheetDialogFragment.dismiss();
                    }
                } else {
                    if (jsonBalances != null && !TextUtils.isEmpty(jsonBalances)) {
                        BinanceWidgetManager.getInstance().setBinanceAccountBalance(jsonBalances);
                    }
                    try {
                        BinanceWidgetManager.binanceAccountBalance = new BinanceAccountBalance(
                                BinanceWidgetManager.getInstance().getBinanceAccountBalance());
                    } catch (JSONException e) {
                        Log.e("NTP", e.getMessage());
                    }
                }
            }
            // Reset binance widget to connect page
            showWidgets();
        };
    };

    // start timer function
    public void startTimer() {
        countDownTimer = new CountDownTimer(30000, 1000) {
            @Override
            public void onTick(long millisUntilFinished) {}
            @Override
            public void onFinish() {
                if (BinanceWidgetManager.getInstance().isUserAuthenticatedForBinance()) {
                    mBinanceNativeWorker.getAccountBalances();
                }
                if ((BraveActivity) mActivity != null) startTimer();
            }
        };
        countDownTimer.start();
    }

    // cancel timer
    public void cancelTimer() {
        if (countDownTimer != null) countDownTimer.cancel();
    }

    public void openWidgetStack() {
        final FragmentManager fm = ((BraveActivity) mActivity).getSupportFragmentManager();
        Fragment auxiliary = new Fragment() {
            @Override
            public void onActivityResult(int requestCode, int resultCode, Intent data) {
                super.onActivityResult(requestCode, resultCode, data);
                fm.beginTransaction().remove(this).commit();
                if (requestCode == NTP_WIDGET_STACK_CODE) {
                    Log.e("NTP", "Inside result");
                    showWidgets();
                }
            }
        };
        fm.beginTransaction().add(auxiliary, "FRAGMENT_TAG").commit();
        fm.executePendingTransactions();

        Intent ntpWidgetStackActivityIntent = new Intent(mActivity, NTPWidgetStackActivity.class);
        ntpWidgetStackActivityIntent.putExtra(NTPWidgetStackActivity.FROM_SETTINGS, false);
        auxiliary.startActivityForResult(ntpWidgetStackActivityIntent, NTP_WIDGET_STACK_CODE);
    }

    @Override
    public void onTileCountChanged() {
        if (mTopsiteErrorMessage == null) {
            return;
        }

        if (getPlaceholder() != null
                && ((ViewGroup)getPlaceholder().getParent()) != null) {
            ((ViewGroup)getPlaceholder().getParent()).removeView(getPlaceholder());
        }

        boolean showPlaceholder =
            getTileGroup().hasReceivedData() && getTileGroup().isEmpty();
        if (!showPlaceholder) {
            mTopsiteErrorMessage.setVisibility(View.GONE);
        } else {
            mTopsiteErrorMessage.setVisibility(View.VISIBLE);
        }
    }
}
