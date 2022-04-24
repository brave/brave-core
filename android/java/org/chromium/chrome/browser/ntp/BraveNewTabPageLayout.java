/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.ContextMenu;
import android.view.Display;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.cardview.widget.CardView;
import androidx.core.widget.ImageViewCompat;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;

import com.airbnb.lottie.LottieAnimationView;
import com.bumptech.glide.Glide;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

import org.json.JSONException;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.DisplayAd;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.FeedPage;
import org.chromium.brave_news.mojom.FeedPageItem;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_news.BraveNewsAdapterFeedCard;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.brave_news.LinearLayoutManagerWrapper;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.custom_layout.VerticalViewPager;
import org.chromium.chrome.browser.explore_sites.ExploreSitesBridge;
import org.chromium.chrome.browser.feed.FeedSurfaceScrollDelegate;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.TopSiteTable;
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;
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
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveNewsPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.sync.settings.BraveManageSyncSettings;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeMap;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BraveNewTabPageLayout extends NewTabPageLayout implements ConnectionErrorHandler {
    private static final String TAG = "BraveNewTabPageView";
    private static final String BRAVE_REF_URL = "https://brave.com/r/";
    private static final String BRAVE_LEARN_MORE_URL =
            "https://brave.com/privacy/browser/#brave-today";
    private static final int ITEMS_PER_PAGE = 18;
    private static final int MINIMUM_VISIBLE_HEIGHT_THRESHOLD = 50;
    private static final int NEWS_SCROLL_TO_TOP_NEW = -1;
    private static final int NEWS_SCROLL_TO_TOP_RELOAD = -2;
    private static final String BRAVE_NESTED_SCROLLVIEW_POSITION = "nestedscrollview_position_";
    private static final String BRAVE_RECYCLERVIEW_POSITION = "recyclerview_visible_position_";
    private static final String BRAVE_RECYCLERVIEW_OFFSET_POSITION =
            "recyclerview_offset_position_";

    private View mBraveStatsViewFallBackLayout;

    private ImageView mBgImageView;
    private Profile mProfile;
    private BraveNewTabPageLayout mNtpContent;
    private LinearLayout mParentLayout;

    private SponsoredTab sponsoredTab;

    private BitmapDrawable imageDrawable;

    private FetchWallpaperWorkerTask mWorkerTask;
    private boolean isFromBottomSheet;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private ViewGroup mainLayout;
    private DatabaseHelper mDatabaseHelper;

    private ViewGroup mSiteSectionView;
    private TileGroup mTileGroup;
    private LottieAnimationView mBadgeAnimationView;

    private Tab mTab;
    private Activity mActivity;
    private LinearLayout superReferralSitesLayout;
    private TextView mTopsiteErrorMessage;

    // Brave news
    private BraveNewsAdapterFeedCard mAdapterFeedCard;
    private FrameLayout mOptinButton;
    private TextView mOptinText;
    private LinearLayout mOptinLayout;
    private TextView mOptinLearnMore;
    private ImageView mOptinClose;
    private CopyOnWriteArrayList<FeedItemsCard> mNewsItemsFeedCard =
            new CopyOnWriteArrayList<FeedItemsCard>();
    private LinearLayout mContainer;
    private RecyclerView mRecyclerView;
    private TextView mLoading;
    private View mLoadingView;
    private View mFeedSpinner;
    private NestedScrollView mParentScrollView;
    private ViewGroup mImageCreditLayout;
    private ViewGroup mSettingsBar;
    private ViewGroup mNewContentButton;
    private boolean isScrolled;
    private NTPImage mNtpImageGlobal;
    private boolean mSettingsBarIsClickable;
    private BraveNewsController mBraveNewsController;

    private ViewGroup mCompositorView;

    private long mStartCardViewTime;
    private long mEndCardViewTime;
    private String mCreativeInstanceId;
    private String mUuid;
    //@TODO alex make an enum
    private String mCardType;
    private int mItemPosition;
    private FeedItemsCard mVisibleCard;
    private boolean mIsNewsOn;
    private boolean mIsShowOptin;
    private boolean mIsShowNewsOn;
    private int mmViewedNewsCardsCount;

    private boolean mIsFeedLoaded;
    private CopyOnWriteArrayList<FeedItemsCard> mExistingNewsFeedObject;
    private int mPrevScrollPosition;
    private static int mFirstVisibleCard;
    private String mFeedHash;
    private SharedPreferencesManager.Observer mPreferenceObserver;
    private int mTouchX;
    private int mTouchY;
    private boolean mTouchScroll;
    private boolean mComesFromNewTab;

    private Supplier<Tab> mTabProvider;

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedRegularProfile();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        mNTPBackgroundImagesBridge.setNewTabPageListener(newTabPageListener);
        mDatabaseHelper = DatabaseHelper.getInstance();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mComesFromNewTab = false;
        mTouchScroll = false;

        NTPUtil.showBREBottomBanner(this);

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

            mIsNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();
            mIsShowOptin = sharedPreferences.getBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, true);
            mIsShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();

            mFeedHash = "";
            InitBraveNewsController();

            mComesFromNewTab = BraveActivity.getBraveActivity().isComesFromNewTab();
            mIsFeedLoaded = BraveActivity.getBraveActivity().isLoadedFeed();
            mExistingNewsFeedObject = BraveActivity.getBraveActivity().getNewsItemsFeedCards();
            mPrevScrollPosition = NEWS_SCROLL_TO_TOP_NEW;

            if (mIsNewsOn && mIsShowNewsOn && mIsFeedLoaded && mExistingNewsFeedObject != null) {
                mNewsItemsFeedCard = mExistingNewsFeedObject;
            }

            if (BraveActivity.getBraveActivity() != null && mIsNewsOn) {
                Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                if ((tab != null && tab.getUrl().getSpec() != null
                            && UrlUtilities.isNTPUrl(tab.getUrl().getSpec()))
                        || mComesFromNewTab) {
                    BraveActivity.getBraveActivity().inflateNewsSettingsBar();
                } else {
                    //  remove settings bar
                    BraveActivity.getBraveActivity().removeSettingsBar();
                    if (tab != null) {
                        SharedPreferencesManager.getInstance().writeInt(
                                BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(),
                                mPrevScrollPosition);
                    }
                }
            }
        }
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
        int margin = dpToPx(mActivity, 16);
        layoutParams.setMargins(margin, margin, margin, margin);
        mBraveStatsViewFallBackLayout.setLayoutParams(layoutParams);
        mBraveStatsViewFallBackLayout.requestLayout();

        mBraveStatsViewFallBackLayout.findViewById(R.id.brave_stats_title_layout)
                .setVisibility(View.GONE);
        mBraveStatsViewFallBackLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            @SuppressLint("SourceLockedOrientationActivity")
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
        mSiteSectionView.setBackgroundResource(R.drawable.rounded_dark_bg_alpha);
        mSiteSectionView.setLayoutParams(layoutParams);
        mSiteSectionView.requestLayout();
        mainLayout.addView(mSiteSectionView, insertionPoint);
    }

    protected void updateTileGridPlaceholderVisibility() {
        // This function is kept empty to avoid placeholder implementation
    }

    private boolean shouldShowSuperReferral() {
        return mNTPBackgroundImagesBridge.isSuperReferral()
                && NTPBackgroundImagesBridge.enableSponsoredImages()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    }

    private void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            ((BraveActivity)mActivity).showOnboardingV2(true);
        }
    }

    @SuppressLint("VisibleForTests")
    protected void insertSiteSectionView() {
        mainLayout = findViewById(R.id.ntp_main_layout);

        mSiteSectionView = NewTabPageLayout.inflateSiteSection(mainLayout);
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
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
            if (mSettingsBar != null) {
                mSettingsBar.setAlpha(0f);
                mSettingsBar.setVisibility(View.INVISIBLE);
            }
            InitBraveNewsController();
            initNews();
            if (BraveActivity.getBraveActivity() != null && mIsNewsOn) {
                new Handler().post(() -> {
                    Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                    if (tab != null && tab.getUrl().getSpec() != null
                            && UrlUtilities.isNTPUrl(tab.getUrl().getSpec())) {
                        // purges display ads on tab change
                        if (BraveActivity.getBraveActivity().getLastTabId() != tab.getId()) {
                            mBraveNewsController.onDisplayAdPurgeOrphanedEvents();
                        }

                        BraveActivity.getBraveActivity().setLastTabId(tab.getId());

                        BraveActivity.getBraveActivity().inflateNewsSettingsBar();
                        mSettingsBar =
                                (LinearLayout) mCompositorView.findViewById(R.id.news_settings_bar);
                        if (mSettingsBar != null) {
                            mSettingsBar.setVisibility(View.VISIBLE);
                        }
                        mNewContentButton = (RelativeLayout) mCompositorView.findViewById(
                                R.id.new_content_layout_id);
                    } else {
                        if (tab != null) {
                        }
                        if (!mComesFromNewTab) {
                            BraveActivity.getBraveActivity().removeSettingsBar();
                        }
                    }
                });
            }
            initPreferenceObserver();
            if (mPreferenceObserver != null) {
                SharedPreferencesManager.getInstance().addObserver(mPreferenceObserver);
            }
        }
        showFallBackNTPLayout();
    }

    private void initPreferenceObserver() {
        mPreferenceObserver = (key) -> {
            if (TextUtils.equals(key, BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE)) {
                if (mNewContentButton != null) {
                    mNewContentButton.setVisibility(View.VISIBLE);
                } else {
                    mNewContentButton = mCompositorView.findViewById(R.id.new_content_layout_id);
                    if (mNewContentButton != null) {
                        mNewContentButton.setVisibility(View.VISIBLE);
                    }
                }
            } else if (TextUtils.equals(key, BravePreferenceKeys.BRAVE_NEWS_PREF_SHOW_NEWS)) {
                if (BravePrefServiceBridge.getInstance().getShowNews()) {
                    if (BraveActivity.getBraveActivity() != null) {
                        BraveActivity.getBraveActivity().inflateNewsSettingsBar();
                    }
                }
                mIsNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();
                mSettingsBar = (LinearLayout) mCompositorView.findViewById(R.id.news_settings_bar);
                if (mSettingsBar != null) {
                    mSettingsBar.setVisibility(View.VISIBLE);
                }
                mNewContentButton =
                        (RelativeLayout) mCompositorView.findViewById(R.id.new_content_layout_id);
                refreshFeed();
            } else if (TextUtils.equals(key, BravePreferenceKeys.BRAVE_NEWS_PREF_TURN_ON_NEWS)) {
                mIsNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();
                SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
                mIsShowOptin =
                        sharedPreferences.getBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, false);
                mIsShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();
                mOptinLayout.setVisibility(View.GONE);
                mRecyclerView.setVisibility(View.VISIBLE);
                initNews();
            }
        };
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

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
            if (mNewsItemsFeedCard != null && mNewsItemsFeedCard.size() > 0) {
                if (BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().setNewsItemsFeedCards(mNewsItemsFeedCard);
                }
            }
            mTouchScroll = false;

            if (mSettingsBar != null) {
                mSettingsBar.setVisibility(View.INVISIBLE);
                mSettingsBar.setAlpha(0f);
                if (BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().removeSettingsBar();
                }
            }

            if (mNewContentButton != null) {
                mNewContentButton.setVisibility(View.INVISIBLE);
            }

            if (mBraveNewsController != null) {
                mBraveNewsController.close();
                mBraveNewsController = null;
            }

            // removes preference observer
            SharedPreferencesManager.getInstance().removeObserver(mPreferenceObserver);
            mPreferenceObserver = null;
        }
        super.onDetachedFromWindow();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (sponsoredTab != null && NTPUtil.shouldEnableNTPFeature()) {
            if (mBgImageView != null && !ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
                // We need to redraw image to fit parent properly
                mBgImageView.setImageResource(android.R.color.transparent);
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

            if (BraveActivity.getBraveActivity() != null
                    && BraveActivity.getBraveActivity().getActivityTab() != null) {
                Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                int prevRecyclerViewPosition = (tab != null)
                        ? SharedPreferencesManager.getInstance().readInt(
                                BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(), 0)
                        : 0;
                int prevScrollPosition = (tab != null)
                        ? SharedPreferencesManager.getInstance().readInt(
                                BRAVE_NESTED_SCROLLVIEW_POSITION + tab.getId(), 0)
                        : 0;
                int prevRecyclerViewItemPosition = (tab != null)
                        ? SharedPreferencesManager.getInstance().readInt(
                                BRAVE_RECYCLERVIEW_POSITION + tab.getId(), 0)
                        : 0;
                keepPosition(
                        prevScrollPosition, prevRecyclerViewPosition, prevRecyclerViewItemPosition);
            }
        } else {
            super.onConfigurationChanged(newConfig);
        }
    }

    private ViewGroup getView(int id) {
        ViewGroup view = findViewById(id);
        return findViewById(id);
    }

    // corrects position of image credit and for the loading spinner. Used when News is active
    private void correctPosition(boolean toTop) {
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        float dpHeight = displayMetrics.heightPixels / displayMetrics.density;
        int pxHeight = dpToPx(mActivity, dpHeight);

        boolean isTablet = ConfigurationUtils.isTablet(mActivity);
        boolean isLandscape = ConfigurationUtils.isLandscape(mActivity);

        if (mImageCreditLayout == null || mFeedSpinner == null) {
            mImageCreditLayout = findViewById(R.id.image_credit_layout);
            mFeedSpinner = findViewById(R.id.feed_spinner);
        }
        if (mImageCreditLayout != null) {
            LinearLayout.LayoutParams linearLayoutParams =
                    (LinearLayout.LayoutParams) mImageCreditLayout.getLayoutParams();

            int imageCreditCorrection = NTPUtil.correctImageCreditLayoutTopPosition(
                    mNtpImageGlobal, mSiteSectionView.getHeight());
            if (toTop) {
                imageCreditCorrection = 0;
            }
            linearLayoutParams.setMargins(0, imageCreditCorrection, 0, 0);
            if (mImageCreditLayout != null) {
                mImageCreditLayout.setLayoutParams(linearLayoutParams);
            }
            mImageCreditLayout.requestLayout();
        }

        if (mFeedSpinner != null) {
            FrameLayout.LayoutParams feedSpinnerParams =
                    (FrameLayout.LayoutParams) mFeedSpinner.getLayoutParams();
            FrameLayout bottomToolbar = (FrameLayout) findViewById(R.id.bottom_toolbar);
            feedSpinnerParams.topMargin = (int) (dpHeight - 30);
            mFeedSpinner.setLayoutParams(feedSpinnerParams);
        }
    }

    private void getFeed() {
        ExecutorService executors = Executors.newFixedThreadPool(1);

        InitBraveNewsController();

        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                mBraveNewsController.getFeed((feed) -> {
                    if (feed == null) {
                        processFeed();
                        mContainer.setVisibility(View.VISIBLE);
                        mRecyclerView.setVisibility(View.VISIBLE);
                        return;
                    }
                    try {
                        mFeedHash = feed.hash;
                        mNewsItemsFeedCard.clear();
                        BraveNewsUtils.initCurrentAds();
                        mAdapterFeedCard.notifyItemRangeRemoved(0, mNewsItemsFeedCard.size());
                        SharedPreferencesManager.getInstance().writeString(
                                BravePreferenceKeys.BRAVE_NEWS_FEED_HASH, feed.hash);
                        if (feed.featuredItem != null) {
                            // process Featured item
                            FeedItem featuredItem = feed.featuredItem;
                            FeedItemsCard featuredItemsCard = new FeedItemsCard();

                            FeedItemMetadata featuredItemMetaData = new FeedItemMetadata();
                            Article featuredArticle = featuredItem.getArticle();
                            FeedItemMetadata featuredArticleData = featuredArticle.data;

                            FeedItemCard featuredItemCard = new FeedItemCard();
                            List<FeedItemCard> featuredCardItems = new ArrayList<>();

                            featuredItemsCard.setCardType(CardType.HEADLINE);
                            featuredItemsCard.setUuid(UUID.randomUUID().toString());

                            featuredItemCard.setFeedItem(featuredItem);
                            featuredCardItems.add(featuredItemCard);

                            featuredItemsCard.setFeedItems(featuredCardItems);
                            mNewsItemsFeedCard.add(featuredItemsCard);
                        }

                        // adds empty card to trigger Display ad call for the second card, when the
                        // user starts scrolling
                        FeedItemsCard displayAdCard = new FeedItemsCard();
                        DisplayAd displayAd = new DisplayAd();
                        displayAdCard.setCardType(CardType.DISPLAY_AD);
                        displayAdCard.setDisplayAd(displayAd);
                        displayAdCard.setUuid(UUID.randomUUID().toString());
                        mNewsItemsFeedCard.add(displayAdCard);

                        // start page loop
                        int noPages = 0;
                        int itemIndex = 0;
                        int totalPages = feed.pages.length;
                        for (FeedPage page : feed.pages) {
                            for (FeedPageItem cardData : page.items) {
                                // if for any reason we get an empty object, unless it's a
                                // DISPLAY_AD we skip it
                                if (cardData.cardType != CardType.DISPLAY_AD) {
                                    if (cardData.items.length == 0) {
                                        continue;
                                    }
                                }
                                FeedItemsCard feedItemsCard = new FeedItemsCard();

                                feedItemsCard.setCardType(cardData.cardType);
                                feedItemsCard.setUuid(UUID.randomUUID().toString());
                                List<FeedItemCard> cardItems = new ArrayList<>();
                                for (FeedItem item : cardData.items) {
                                    FeedItemMetadata itemMetaData = new FeedItemMetadata();
                                    FeedItemCard feedItemCard = new FeedItemCard();
                                    feedItemCard.setFeedItem(item);

                                    cardItems.add(feedItemCard);

                                    feedItemsCard.setFeedItems(cardItems);
                                }
                                mNewsItemsFeedCard.add(feedItemsCard);
                            }
                        } // end page loop
                        processFeed();

                        mContainer.setVisibility(View.VISIBLE);
                        mRecyclerView.setVisibility(View.VISIBLE);

                        BraveActivity.getBraveActivity().setNewsItemsFeedCards(mNewsItemsFeedCard);
                        BraveActivity.getBraveActivity().setLoadedFeed(true);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                });
            }
        };
        executors.submit(runnable);
    }

    private void refreshFeed() {
        mIsShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();
        if (!mIsShowNewsOn) {
            correctPosition(false);
            if (mRecyclerView != null) {
                mRecyclerView.setVisibility(View.GONE);
            }
            if (mNewContentButton != null) {
                mNewContentButton.setVisibility(View.INVISIBLE);
            }
            mImageCreditLayout.setAlpha(1.0f);
            return;
        } else {
            mAdapterFeedCard = new BraveNewsAdapterFeedCard(
                    mActivity, Glide.with(mActivity), mNewsItemsFeedCard, mBraveNewsController);
            mRecyclerView.setAdapter(mAdapterFeedCard);

            mImageCreditLayout.setVisibility(View.VISIBLE);
            mImageCreditLayout.setAlpha(1.0f);
            SharedPreferencesManager.getInstance().removeObserver(mPreferenceObserver);
            initPreferenceObserver();
            if (mPreferenceObserver != null) {
                SharedPreferencesManager.getInstance().addObserver(mPreferenceObserver);
            }
        }
        if (mIsShowNewsOn && BravePrefServiceBridge.getInstance().getNewsOptIn()) {
            getFeed();
        }
    }

    private void keepPosition(int prevScrollPosition, int prevRecyclerViewPosition,
            int prevRecyclerViewItemPosition) {
        if ((mIsNewsOn != mIsShowNewsOn) || (mIsNewsOn && mIsShowOptin)) {
            return;
        }
        processFeed();
        int scrollPosition = prevScrollPosition;

        new Handler().postDelayed(() -> {
            mContainer.setVisibility(View.VISIBLE);
            mRecyclerView.setVisibility(View.VISIBLE);
            mParentScrollView.post(() -> { mParentScrollView.scrollTo(0, prevScrollPosition); });
            if (prevRecyclerViewItemPosition > 0) {
                RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
                if (manager instanceof LinearLayoutManager) {
                    LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
                    linearLayoutManager.scrollToPositionWithOffset(
                            prevRecyclerViewItemPosition, prevRecyclerViewPosition);
                }
            }
            correctPosition(false);
        }, 100);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void initNews() {
        mSettingsBarIsClickable = false;
        mRecyclerView = findViewById(R.id.newsRecycler);
        mContainer = (LinearLayout) findViewById(R.id.ntp_main_layout);
        mOptinButton = findViewById(R.id.optin_button);
        mOptinClose = findViewById(R.id.close_optin);
        mOptinLearnMore = findViewById(R.id.optin_learnmore);
        mOptinLayout = findViewById(R.id.optin_layout_id);
        mOptinText = findViewById(R.id.optin_button_text);
        mLoading = findViewById(R.id.loading);
        mLoadingView = findViewById(R.id.optin_loading_spinner);
        mFeedSpinner = findViewById(R.id.feed_spinner);
        mParentLayout = (LinearLayout) findViewById(R.id.parent_layout);
        mNtpContent = (BraveNewTabPageLayout) findViewById(R.id.ntp_content);
        mTouchX = -200;
        mTouchScroll = false;

        SharedPreferencesManager.getInstance().writeBoolean(
                BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, false);
        // init Brave news parameters
        mStartCardViewTime = 0;
        mEndCardViewTime = 0;
        mCreativeInstanceId = "";
        mUuid = "";
        //@TODO alex make an enum
        mCardType = "";
        mItemPosition = 0;
        mVisibleCard = null;

        ViewGroup.LayoutParams recyclerviewParams = mRecyclerView.getLayoutParams();
        recyclerviewParams.height = (ConfigurationUtils.isTablet(mActivity)
                                            && !ConfigurationUtils.isLandscape(mActivity))
                ? (int) dpToPx(mActivity, 1500)
                : (int) dpToPx(mActivity, 800);

        mRecyclerView.setLayoutParams(recyclerviewParams);

        if (mOptinLayout != null) {
            mLoadingView.setVisibility(View.GONE);
            mOptinLayout.setVisibility(View.GONE);
        }

        mRecyclerView.setDrawingCacheEnabled(true);
        mRecyclerView.setDrawingCacheQuality(View.DRAWING_CACHE_QUALITY_HIGH);
        mRecyclerView.setItemAnimator(null);

        mRecyclerView.setVisibility(View.GONE);

        mAdapterFeedCard = new BraveNewsAdapterFeedCard(
                mActivity, Glide.with(mActivity), mNewsItemsFeedCard, mBraveNewsController);
        mRecyclerView.setAdapter(mAdapterFeedCard);

        // Used to prevent a recyclerView layout bug
        mRecyclerView.setLayoutManager(
                new LinearLayoutManagerWrapper(mActivity, LinearLayoutManager.VERTICAL, false));

        mParentScrollView = (NestedScrollView) mNtpContent.getParent();

        ViewGroup rootView = (ViewGroup) mParentScrollView.getParent();
        rootView.setFocusableInTouchMode(true);
        mCompositorView = (ViewGroup) rootView.getParent();

        mImageCreditLayout = findViewById(R.id.image_credit_layout);

        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

        mIsNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();
        mIsShowOptin = sharedPreferences.getBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, true);
        mIsShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();

        if ((!mIsNewsOn && mIsShowOptin) || (mIsNewsOn && mIsShowOptin)) {
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, true);
            sharedPreferencesEditor.apply();
            mOptinLayout.setVisibility(View.VISIBLE);
        } else if (mIsShowNewsOn && mIsNewsOn) {
            if (mOptinLayout != null) {
                mOptinLayout.setVisibility(View.GONE);
            }
            mParentLayout.removeView(mOptinLayout);
            mFeedSpinner.setVisibility(View.VISIBLE);

            boolean isFeedLoaded = BraveActivity.getBraveActivity().isLoadedFeed();
            boolean isFromNewTab = BraveActivity.getBraveActivity().isComesFromNewTab();

            CopyOnWriteArrayList<FeedItemsCard> existingNewsFeedObject =
                    BraveActivity.getBraveActivity().getNewsItemsFeedCards();
            Tab tab = BraveActivity.getBraveActivity().getActivityTab();
            int prevRecyclerViewPosition = (tab != null)
                    ? SharedPreferencesManager.getInstance().readInt(
                            BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(), 0)
                    : 0;
            int prevScrollPosition = (tab != null) ? SharedPreferencesManager.getInstance().readInt(
                                             BRAVE_NESTED_SCROLLVIEW_POSITION + tab.getId(), 0)
                                                   : 0;
            int prevRecyclerViewItemPosition = (tab != null)
                    ? SharedPreferencesManager.getInstance().readInt(
                            BRAVE_RECYCLERVIEW_POSITION + tab.getId(), 0)
                    : 0;
            mmViewedNewsCardsCount = (tab != null) ? SharedPreferencesManager.getInstance().readInt(
                                             "mViewedNewsCardsCount_" + tab.getId())
                                                   : 0;

            if (prevScrollPosition == 0 && prevRecyclerViewPosition == 0
                    && prevRecyclerViewItemPosition == 0) {
                isFeedLoaded = false;
                existingNewsFeedObject = null;
                mmViewedNewsCardsCount = 0;
            }

            if (!isFeedLoaded || isFromNewTab) {
                getFeed();
                if (tab != null) {
                    SharedPreferencesManager.getInstance().writeInt(
                            BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(),
                            NEWS_SCROLL_TO_TOP_NEW);
                }

                // Brave News interaction started
                if (mBraveNewsController != null) {
                    mBraveNewsController.onInteractionSessionStarted();
                }
            } else {
                if (mActivity == null) {
                    mActivity = BraveActivity.getBraveActivity();
                }
                keepPosition(
                        prevScrollPosition, prevRecyclerViewPosition, prevRecyclerViewItemPosition);
            }
        } else {
            if (mOptinLayout != null) {
                mOptinLayout.setVisibility(View.GONE);
            }
        }

        ViewTreeObserver parentScrollViewObserver = mParentScrollView.getViewTreeObserver();
        mParentScrollView.setOnScrollChangeListener(new NestedScrollView.OnScrollChangeListener() {
            @Override
            public void onScrollChange(
                    NestedScrollView v, int scrollX, int scrollY, int oldScrollX, int oldScrollY) {
                if (scrollY != oldScrollY) {
                    try {
                        if (BraveActivity.getBraveActivity() != null
                                && BraveActivity.getBraveActivity().getActivityTab() != null) {
                            SharedPreferencesManager.getInstance().writeInt(
                                    BRAVE_NESTED_SCROLLVIEW_POSITION
                                            + BraveActivity.getBraveActivity()
                                                      .getActivityTab()
                                                      .getId(),
                                    scrollY);
                        }

                        RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
                        if (manager instanceof LinearLayoutManager) {
                            LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
                        }
                        isScrolled = false;
                        float value = (float) scrollY / mParentScrollView.getMaxScrollAmount();
                        if (value >= 1) {
                            value = 1;
                        }
                        float alpha = (float) (1 - value * 4);
                        if (alpha < 1f) {
                            mImageCreditLayout.setAlpha(alpha);
                            mImageCreditLayout.requestLayout();
                        }
                        if (BraveActivity.getBraveActivity() != null
                                && BraveActivity.getBraveActivity().getActivityTab() != null) {
                            if (UrlUtilities.isNTPUrl(BraveActivity.getBraveActivity()
                                                              .getActivityTab()
                                                              .getUrl()
                                                              .getSpec())) {
                                if (mSettingsBar != null) {
                                    if (BraveActivity.getBraveActivity() != null) {
                                        BraveActivity.getBraveActivity().inflateNewsSettingsBar();
                                    }
                                    if (mSettingsBar.getVisibility() == View.VISIBLE) {
                                        if (value > 0.4) {
                                            mSettingsBar.setAlpha((float) (value + 0.5));
                                        } else if (value < 0.4 && mSettingsBar.getAlpha() > 0f) {
                                            mSettingsBar.setAlpha((float) (value - 0.2));
                                        } else if (value == 1 && mSettingsBar.getAlpha() >= 1f) {
                                            mSettingsBar.setAlpha(1);
                                            mSettingsBar.requestLayout();
                                        }
                                        if (mSettingsBar.getAlpha() >= 1) {
                                            isScrolled = true;
                                            mSettingsBarIsClickable = true;
                                        } else {
                                            mSettingsBarIsClickable = false;
                                        }
                                        if (mSettingsBar.getAlpha() <= 0) {
                                            mSettingsBar.setVisibility(View.INVISIBLE);
                                        }
                                    } else {
                                        boolean isFromNewTab = BraveActivity.getBraveActivity()
                                                                       .isComesFromNewTab();
                                        if (scrollY > 200 && (mTouchScroll || isFromNewTab)) {
                                            mSettingsBar.setVisibility(View.VISIBLE);
                                            mSettingsBar.setAlpha(1);
                                        }
                                    }
                                }
                            }
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        parentScrollViewObserver.addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        try {
                            if (mSettingsBar != null) {
                                ImageView newsSettingsButton =
                                        (ImageView) mSettingsBar.findViewById(
                                                R.id.news_settings_button);
                                ViewTreeObserver.OnGlobalLayoutListener listener = this;
                                newsSettingsButton.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        if (mSettingsBarIsClickable
                                                || mSettingsBar.getAlpha() >= 1) {
                                            SettingsLauncher settingsLauncher =
                                                    new SettingsLauncherImpl();
                                            settingsLauncher.launchSettingsActivity(
                                                    getContext(), BraveNewsPreferences.class);
                                        }
                                    }
                                });

                                // Double tap on the settings bar to scroll back up
                                mSettingsBar.setOnTouchListener(new OnTouchListener() {
                                    private GestureDetector gestureDetector =
                                            new GestureDetector(mActivity,
                                                    new GestureDetector.SimpleOnGestureListener() {
                                                        @Override
                                                        public boolean onDoubleTap(MotionEvent e) {
                                                            if (BraveActivity.getBraveActivity()
                                                                            != null
                                                                    && BraveActivity.getBraveActivity()
                                                                                    .getActivityTab()
                                                                            != null) {
                                                                SharedPreferencesManager
                                                                        .getInstance()
                                                                        .writeInt(
                                                                                BRAVE_RECYCLERVIEW_OFFSET_POSITION
                                                                                        + BraveActivity
                                                                                                  .getBraveActivity()
                                                                                                  .getActivityTab()
                                                                                                  .getId(),
                                                                                0);
                                                            }
                                                            correctPosition(false);
                                                            mParentScrollView.fullScroll(
                                                                    NestedScrollView.FOCUS_UP);
                                                            mRecyclerView.scrollToPosition(0);
                                                            return super.onDoubleTap(e);
                                                        }
                                                    });

                                    @Override
                                    public boolean onTouch(View v, MotionEvent event) {
                                        gestureDetector.onTouchEvent(event);
                                        return true;
                                    }
                                });

                                // to make sure that tap on the settings bar doesn't go through and
                                // trigger the article view
                                mSettingsBar.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        return;
                                    }
                                });
                            }

                            if (mNewContentButton != null) {
                                ProgressBar loadingSpinner =
                                        (ProgressBar) mNewContentButton.findViewById(
                                                R.id.new_content_loading_spinner);
                                TextView newContentButtonText =
                                        (TextView) mNewContentButton.findViewById(
                                                R.id.new_content_button_text);
                                mNewContentButton.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        //@TODO alex check why visibility change doesn't work
                                        newContentButtonText.setVisibility(View.INVISIBLE);
                                        loadingSpinner.setVisibility(View.VISIBLE);
                                        mNewContentButton.setClickable(false);
                                        SharedPreferencesManager.getInstance().writeBoolean(
                                                BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE,
                                                false);
                                        if (!mIsShowNewsOn) {
                                            mIsShowNewsOn = true;
                                        }
                                        isScrolled = false;
                                        refreshFeed();
                                        new Handler().postDelayed(() -> {
                                            int pxHeight =
                                                    ConfigurationUtils.getDisplayMetrics(mActivity)
                                                            .get("height");
                                            int scrollToY;
                                            if (!ConfigurationUtils.isTablet(mActivity)
                                                    && ConfigurationUtils.isLandscape(mActivity)) {
                                                scrollToY = 0;

                                            } else if (ConfigurationUtils.isTablet(mActivity)
                                                    && !ConfigurationUtils.isLandscape(mActivity)) {
                                                scrollToY = pxHeight - dpToPx(getContext(), 320);

                                            } else if (ConfigurationUtils.isTablet(mActivity)
                                                    && ConfigurationUtils.isLandscape(mActivity)) {
                                                scrollToY = pxHeight - dpToPx(getContext(), 270);

                                            } else {
                                                scrollToY = pxHeight - dpToPx(getContext(), 215);
                                            }

                                            mParentScrollView.smoothScrollTo(0, scrollToY);
                                            if (BraveActivity.getBraveActivity() != null
                                                    && BraveActivity.getBraveActivity()
                                                                    .getActivityTab()
                                                            != null) {
                                                SharedPreferencesManager.getInstance().writeInt(
                                                        BRAVE_RECYCLERVIEW_OFFSET_POSITION
                                                                + BraveActivity.getBraveActivity()
                                                                          .getActivityTab()
                                                                          .getId(),
                                                        -1);

                                                SharedPreferencesManager.getInstance().writeInt(
                                                        BRAVE_RECYCLERVIEW_POSITION
                                                                + BraveActivity.getBraveActivity()
                                                                          .getActivityTab()
                                                                          .getId(),
                                                        -1);
                                            }
                                        }, 100);

                                        newContentButtonText.setVisibility(View.VISIBLE);
                                        loadingSpinner.setVisibility(View.GONE);
                                        mNewContentButton.setClickable(true);
                                        mNewContentButton.setVisibility(View.INVISIBLE);
                                        if (mImageCreditLayout != null) {
                                            mImageCreditLayout.setVisibility(View.VISIBLE);
                                            mImageCreditLayout.setAlpha(1);
                                            mImageCreditLayout.requestLayout();
                                        }
                                    }
                                });
                            }
                        } catch (Exception e) {
                            Log.e("bn", "Exception  addOnGlobalLayoutListener e: " + e);
                        }
                    }
                });

        RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
        if (manager instanceof LinearLayoutManager) {
            LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
            mFirstVisibleCard = linearLayoutManager.findFirstVisibleItemPosition();
            mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
                @Override
                public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState) {
                    super.onScrollStateChanged(recyclerView, newState);
                    int firstCompletelyVisibleItemPosition =
                            linearLayoutManager.findFirstCompletelyVisibleItemPosition();
                    if (newState == RecyclerView.SCROLL_STATE_DRAGGING) {
                        mEndCardViewTime = System.currentTimeMillis();
                        long timeDiff = mEndCardViewTime - mStartCardViewTime;
                        // if viewed for more than 100 ms send the event
                        if (timeDiff > BraveNewsUtils.BRAVE_NEWS_VIEWD_CARD_TIME) {
                            if (mVisibleCard != null) {
                                // send viewed cards events
                                if (mCardType.equals("promo") && !mCardType.equals("displayad")) {
                                    if (!mUuid.equals("") && !mCreativeInstanceId.equals("")) {
                                        mVisibleCard.setViewStatSent(true);
                                        mBraveNewsController.onPromotedItemView(
                                                mUuid, mCreativeInstanceId);
                                    }
                                } else {
                                    mmViewedNewsCardsCount++;
                                    SharedPreferencesManager.getInstance().writeInt(
                                            "mViewedNewsCardsCount_"
                                                    + BraveActivity.getBraveActivity()
                                                              .getActivityTab()
                                                              .getId(),
                                            mmViewedNewsCardsCount);

                                    if (mmViewedNewsCardsCount > 0) {
                                        mBraveNewsController.onSessionCardViewsCountChanged(
                                                (short) mmViewedNewsCardsCount);
                                    }
                                }
                            }
                        }
                    }

                    if (newState == RecyclerView.SCROLL_STATE_IDLE) {
                        mStartCardViewTime = System.currentTimeMillis();
                        int firstVisibleItemPosition =
                                linearLayoutManager.findFirstVisibleItemPosition();
                        int lastVisibleItemPosition =
                                linearLayoutManager.findLastVisibleItemPosition();
                        int scrollY = mParentScrollView.getScrollY();

                        if (BraveActivity.getBraveActivity() != null
                                && BraveActivity.getBraveActivity().getActivityTab() != null) {
                            View firstChild = mRecyclerView.getChildAt(0);
                            int firstVisiblePosition =
                                    mRecyclerView.getChildAdapterPosition(firstChild);
                            int verticalOffset = firstChild.getTop();

                            SharedPreferencesManager.getInstance().writeInt(
                                    BRAVE_RECYCLERVIEW_OFFSET_POSITION
                                            + BraveActivity.getBraveActivity()
                                                      .getActivityTab()
                                                      .getId(),
                                    verticalOffset);

                            SharedPreferencesManager.getInstance().writeInt(
                                    BRAVE_RECYCLERVIEW_POSITION
                                            + BraveActivity.getBraveActivity()
                                                      .getActivityTab()
                                                      .getId(),
                                    firstVisiblePosition);
                        }
                        mFeedHash = SharedPreferencesManager.getInstance().readString(
                                BravePreferenceKeys.BRAVE_NEWS_FEED_HASH, "");
                        //@TODO alex optimize feed availability check
                        mBraveNewsController.isFeedUpdateAvailable(
                                mFeedHash, isNewsFeedAvailable -> {
                                    if (mNewContentButton != null) {
                                        if (isNewsFeedAvailable) {
                                            mNewContentButton.setVisibility(View.VISIBLE);
                                        } else {
                                            mNewContentButton.setVisibility(View.INVISIBLE);
                                        }
                                    }
                                });

                        Rect rvRect = new Rect();
                        mRecyclerView.getGlobalVisibleRect(rvRect);

                        int visiblePercentage = 0;
                        for (int viewPosition = firstVisibleItemPosition;
                                viewPosition <= lastVisibleItemPosition; viewPosition++) {
                            Rect rowRect = new Rect();
                            if (linearLayoutManager.findViewByPosition(viewPosition) != null) {
                                linearLayoutManager.findViewByPosition(viewPosition)
                                        .getGlobalVisibleRect(rowRect);

                                if (linearLayoutManager.findViewByPosition(viewPosition).getHeight()
                                        > 0) {
                                    if (rowRect.bottom >= rvRect.bottom) {
                                        int visibleHeightFirst = rvRect.bottom - rowRect.top;
                                        visiblePercentage = (visibleHeightFirst * 100)
                                                / linearLayoutManager
                                                          .findViewByPosition(viewPosition)
                                                          .getHeight();
                                    } else {
                                        int visibleHeightFirst = rowRect.bottom - rvRect.top;
                                        visiblePercentage = (visibleHeightFirst * 100)
                                                / linearLayoutManager
                                                          .findViewByPosition(viewPosition)
                                                          .getHeight();
                                    }
                                }

                                if (visiblePercentage > 100) {
                                    visiblePercentage = 100;
                                }
                            }

                            final int visiblePercentageFinal = visiblePercentage;

                            if (viewPosition >= 0) {
                                if (visiblePercentageFinal >= MINIMUM_VISIBLE_HEIGHT_THRESHOLD) {
                                    mVisibleCard = mNewsItemsFeedCard.get(viewPosition);
                                    // get params for view PROMOTED_ARTICLE
                                    if (mVisibleCard.getCardType() == CardType.PROMOTED_ARTICLE) {
                                        mItemPosition = viewPosition;
                                        mCreativeInstanceId =
                                                BraveNewsUtils.getPromotionIdItem(mVisibleCard);
                                        mUuid = mVisibleCard.getUuid();
                                        mCardType = "promo";
                                    }
                                    // get params for view DISPLAY_AD
                                    if (mVisibleCard.getCardType() == CardType.DISPLAY_AD) {
                                        mItemPosition = viewPosition;
                                        DisplayAd currentDisplayAd =
                                                BraveNewsUtils.getFromDisplayAdsMap(viewPosition);
                                        if (currentDisplayAd != null) {
                                            mCreativeInstanceId = currentDisplayAd != null
                                                    ? currentDisplayAd.creativeInstanceId
                                                    : "";
                                            mUuid = currentDisplayAd != null ? currentDisplayAd.uuid
                                                                             : "";
                                            mCardType = "displayad";

                                            // if viewed for more than 100 ms and is more than 50%
                                            // visible send the event
                                            Timer timer = new Timer();
                                            timer.schedule(new TimerTask() {
                                                @Override
                                                public void run() {
                                                    new Thread() {
                                                        @Override
                                                        public void run() {
                                                            if (!mDatabaseHelper
                                                                            .isDisplayAdAlreadyAdded(
                                                                                    mUuid)
                                                                    && visiblePercentageFinal
                                                                            > MINIMUM_VISIBLE_HEIGHT_THRESHOLD) {
                                                                mVisibleCard.setViewStatSent(true);
                                                                mBraveNewsController.onDisplayAdView(
                                                                        mUuid, mCreativeInstanceId);
                                                                DisplayAd currentDisplayAd =
                                                                        BraveNewsUtils
                                                                                .getFromDisplayAdsMap(
                                                                                        mItemPosition);

                                                                mDatabaseHelper.insertAd(
                                                                        currentDisplayAd,
                                                                        mItemPosition,
                                                                        BraveActivity
                                                                                .getBraveActivity()
                                                                                .getActivityTab()
                                                                                .getId());
                                                            }
                                                        }
                                                    }.start();
                                                }
                                            }, BraveNewsUtils.BRAVE_NEWS_VIEWD_CARD_TIME);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                @Override
                public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
                    super.onScrolled(recyclerView, dx, dy);
                    try {
                        int offset = recyclerView.computeVerticalScrollOffset();
                        mTouchScroll = true;
                        mFirstVisibleCard = linearLayoutManager.findFirstVisibleItemPosition();
                        mParentScrollView.scrollBy(0, offset + 2);

                    } catch (Exception e) {
                        Log.e("bn", "Exception onScrolled:" + e);
                    }
                }
            });
        }
        if (mOptinLayout != null) {
            mOptinClose.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
                    sharedPreferencesEditor.putBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, false);
                    sharedPreferencesEditor.apply();
                    BravePrefServiceBridge.getInstance().setNewsOptIn(true);
                    BravePrefServiceBridge.getInstance().setShowNews(false);
                    correctPosition(false);
                    mParentScrollView.fullScroll(NestedScrollView.FOCUS_UP);
                    mImageCreditLayout.setAlpha(1.0f);
                    mOptinLayout.setVisibility(View.GONE);
                }
            });

            mOptinLearnMore.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    TabUtils.openUrlInSameTab(BRAVE_LEARN_MORE_URL);
                }
            });

            mOptinButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mOptinButton.setClickable(false);
                    mOptinText.setVisibility(View.INVISIBLE);
                    mLoadingView.setVisibility(View.VISIBLE);

                    SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
                    sharedPreferencesEditor.putBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, false);
                    sharedPreferencesEditor.apply();
                    BravePrefServiceBridge.getInstance().setNewsOptIn(true);
                    BravePrefServiceBridge.getInstance().setShowNews(true);
                    if (BraveActivity.getBraveActivity() != null) {
                        BraveActivity.getBraveActivity().inflateNewsSettingsBar();
                        mSettingsBar =
                                (LinearLayout) mCompositorView.findViewById(R.id.news_settings_bar);
                        mSettingsBar.setVisibility(View.INVISIBLE);
                        mNewContentButton = (RelativeLayout) mCompositorView.findViewById(
                                R.id.new_content_layout_id);
                    }

                    getFeed();
                    mParentScrollView.fullScroll(NestedScrollView.FOCUS_UP);
                    mRecyclerView.scrollToPosition(0);
                }
            });
        }
    }

    private void processFeed() {
        mFeedSpinner.setVisibility(View.GONE);
        if (mOptinLayout != null) {
            mOptinLayout.setVisibility(View.GONE);
        }

        if (mNewsItemsFeedCard != null && mNewsItemsFeedCard.size() > 0) {
            mAdapterFeedCard.notifyItemRangeInserted(0, mNewsItemsFeedCard.size());
        }

        isScrolled = true;

        if (BraveActivity.getBraveActivity() != null) {
            BraveActivity.getBraveActivity().setComesFromNewTab(false);
        }
    }

    @Override
    public void initialize(NewTabPageManager manager, Activity activity,
            TileGroup.Delegate tileGroupDelegate, boolean searchProviderHasLogo,
            boolean searchProviderIsGoogle, FeedSurfaceScrollDelegate scrollDelegate,
            ContextMenuManager contextMenuManager, TouchEnabledDelegate touchEnabledDelegate,
            UiConfig uiConfig, Supplier<Tab> tabProvider,
            ActivityLifecycleDispatcher lifecycleDispatcher, NewTabPageUma uma, boolean isIncognito,
            WindowAndroid windowAndroid) {
        super.initialize(manager, activity, tileGroupDelegate, searchProviderHasLogo,
                searchProviderIsGoogle, scrollDelegate, contextMenuManager, touchEnabledDelegate,
                uiConfig, tabProvider, lifecycleDispatcher, uma, isIncognito, windowAndroid);

        assert (activity instanceof BraveActivity);
        mActivity = activity;
        mTabProvider = tabProvider;
        ((BraveActivity) mActivity).dismissShieldsTooltip();
    }

    private void showNTPImage(NTPImage ntpImage) {
        Display display = mActivity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        mSiteSectionView.post(new Runnable() {
            @Override
            public void run() {
                correctPosition(false);
            }
        });
        NTPUtil.updateOrientedUI(mActivity, this, size, ntpImage, mSiteSectionView.getHeight());

        ImageView mSponsoredLogo = (ImageView) findViewById(R.id.sponsored_logo);
        FloatingActionButton mSuperReferralLogo = (FloatingActionButton) findViewById(R.id.super_referral_logo);
        TextView mCreditText = (TextView) findViewById(R.id.credit_text);
        mNtpImageGlobal = ntpImage;
        if (ntpImage instanceof Wallpaper
                && NTPUtil.isReferralEnabled()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            setBackgroundImage(ntpImage);
            mSuperReferralLogo.setVisibility(View.VISIBLE);
            mCreditText.setVisibility(View.GONE);
            int floatingButtonIcon = R.drawable.ic_qr_code;
            mSuperReferralLogo.setImageResource(floatingButtonIcon);
            int floatingButtonIconColor =
                    GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? android.R.color.white
                    : android.R.color.black;
            ImageViewCompat.setImageTintList(mSuperReferralLogo,
                    ColorStateList.valueOf(getResources().getColor(floatingButtonIconColor)));
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
        mBgImageView = (ImageView) findViewById(R.id.bg_image_view);
        mBgImageView.setScaleType(ImageView.ScaleType.MATRIX);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        mActivity.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceHeight = displayMetrics.heightPixels;
        int mDeviceWidth = displayMetrics.widthPixels;

        ViewTreeObserver observer = mBgImageView.getViewTreeObserver();
        observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                mWorkerTask = new FetchWallpaperWorkerTask(
                        ntpImage, mDeviceWidth, mDeviceHeight, wallpaperRetrievedCallback);
                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

                mBgImageView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
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
                                Profile.getLastUsedRegularProfile()))
                && (!mIsShowOptin && !mIsShowNewsOn)) {
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
            mParentScrollView = (NestedScrollView) mNtpContent.getParent();
            ViewGroup rootView = (ViewGroup) mParentScrollView.getParent();
            rootView.setFocusableInTouchMode(true);
            CompositorViewHolder compositorView = (CompositorViewHolder) rootView.getParent();
            final int childCount = compositorView.getChildCount();
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

    private FetchWallpaperWorkerTask.WallpaperRetrievedCallback
            wallpaperRetrievedCallback = new FetchWallpaperWorkerTask.WallpaperRetrievedCallback() {
        @Override
        public void bgWallpaperRetrieved(Bitmap bgWallpaper) {
            if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS)) {
                if (BraveActivity.getBraveActivity() != null && mTabProvider.get() != null
                        && !mTabProvider.get().isIncognito()) {
                    BraveActivity.getBraveActivity().setBackground(bgWallpaper);
                }
                try {
                    mParentScrollView = (NestedScrollView) mNtpContent.getParent();
                    if (mParentScrollView != null) {
                        ViewGroup rootView = (ViewGroup) mParentScrollView.getParent();

                        if (rootView != null) {
                            rootView.setFocusableInTouchMode(true);
                            ViewGroup compositorView = (ViewGroup) rootView.getParent();

                            if (BraveActivity.getBraveActivity() != null
                                    && BraveActivity.getBraveActivity().getActivityTab() != null
                                    && compositorView != null) {
                                if (UrlUtilities.isNTPUrl(BraveActivity.getBraveActivity()
                                                                  .getActivityTab()
                                                                  .getUrl()
                                                                  .getSpec())) {
                                    mPrevScrollPosition =
                                            SharedPreferencesManager.getInstance().readInt(
                                                    BRAVE_NESTED_SCROLLVIEW_POSITION
                                                            + BraveActivity.getBraveActivity()
                                                                      .getActivityTab()
                                                                      .getId(),
                                                    0);
                                    if (compositorView.getChildAt(2).getId()
                                            == R.id.news_settings_bar) {
                                        mSettingsBar = (LinearLayout) compositorView.getChildAt(2);
                                        mSettingsBar.setVisibility(View.INVISIBLE);
                                        mSettingsBar.setAlpha(0f);
                                    }
                                    if (compositorView.getChildAt(3).getId()
                                            == R.id.new_content_layout_id) {
                                        mNewContentButton =
                                                (RelativeLayout) compositorView.getChildAt(3);
                                        mNewContentButton.setVisibility(View.INVISIBLE);
                                    }
                                }
                            }
                        }
                    }
                } catch (Exception e) {
                    Log.e("bn", "crashinvestigation exception: " + e.getMessage());
                }

            } else {
                mBgImageView.setImageBitmap(bgWallpaper);
            }
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
            tileViewTitleTv.setTextColor(
                    getResources().getColor(R.color.brave_state_time_count_color));

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
        showFallBackNTPLayout();
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

    @Override
    public void onTileCountChanged() {
        new Handler().postDelayed(() -> {
            if (mTileGroup != null && mTileGroup.isEmpty()) {
                correctPosition(false);
            }
        }, 100);

        if (mTopsiteErrorMessage == null) {
            return;
        }

        boolean showPlaceholder =
                mTileGroup != null && mTileGroup.hasReceivedData() && mTileGroup.isEmpty();
        if (!showPlaceholder) {
            mTopsiteErrorMessage.setVisibility(View.GONE);
        } else {
            mTopsiteErrorMessage.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
        InitBraveNewsController();
    }

    private void InitBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        mBraveNewsController =
                BraveNewsControllerFactory.getInstance().getBraveNewsController(this);
    }
}
