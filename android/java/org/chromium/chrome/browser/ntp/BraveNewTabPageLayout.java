/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.AttributeSet;
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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;

import com.airbnb.lottie.LottieAnimationView;
import com.bumptech.glide.Glide;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.Supplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.DisplayAd;
import org.chromium.brave_news.mojom.Feed;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.FeedPage;
import org.chromium.brave_news.mojom.FeedPageItem;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.brave_news.CardBuilderFeedCard;
import org.chromium.chrome.browser.brave_news.LinearLayoutManagerWrapper;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.feed.FeedSurfaceScrollDelegate;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.TopSiteTable;
import org.chromium.chrome.browser.logo.LogoCoordinator;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
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
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.query_tiles.BraveQueryTileSection;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.settings.BraveNewsPreferencesV2;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.suggestions.tile.MostVisitedTilesGridLayout;
import org.chromium.chrome.browser.suggestions.tile.TileGroup.Delegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

public class BraveNewTabPageLayout
        extends NewTabPageLayout implements ConnectionErrorHandler, OnBraveNtpListener {
    private static final String TAG = "BraveNewTabPage";

    private static final int MINIMUM_VISIBLE_HEIGHT_THRESHOLD = 50;
    private static final String BRAVE_RECYCLERVIEW_POSITION = "recyclerview_visible_position_";
    private static final String BRAVE_RECYCLERVIEW_OFFSET_POSITION =
            "recyclerview_offset_position_";

    // To delete in bytecode, parent variable will be used instead.
    private ViewGroup mMvTilesContainerLayout;
    private LogoCoordinator mLogoCoordinator;

    // Own members.
    private final Context mContext;
    private ImageView mBgImageView;
    private Profile mProfile;
    private SponsoredTab mSponsoredTab;

    private BitmapDrawable mImageDrawable;

    private FetchWallpaperWorkerTask mWorkerTask;
    private boolean mIsFromBottomSheet;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private ViewGroup mMainLayout;
    private DatabaseHelper mDatabaseHelper;

    private LottieAnimationView mBadgeAnimationView;

    private Tab mTab;
    private Activity mActivity;
    private LinearLayout mSuperReferralSitesLayout;

    private BraveNtpAdapter mNtpAdapter;
    private Bitmap mSponsoredLogo;
    private Wallpaper mWallpaper;

    private CopyOnWriteArrayList<FeedItemsCard> mNewsItemsFeedCard =
            new CopyOnWriteArrayList<FeedItemsCard>();
    private RecyclerView mRecyclerView;
    private LinearLayout mNewsSettingsBar;
    private LinearLayout mNewContentLayout;
    private TextView mNewContentText;
    private ProgressBar mNewContentProgressBar;

    private NTPImage mNtpImageGlobal;
    private BraveNewsController mBraveNewsController;

    private long mStartCardViewTime;
    private long mEndCardViewTime;
    private String mCreativeInstanceId;
    private String mUuid;
    //@TODO alex make an enum
    private String mCardType;
    private int mItemPosition;
    private int mPrevVisibleNewsCardPosition = -1;
    private int mNewsSessionCardViews;
    private FeedItemsCard mVisibleCard;
    private String mFeedHash;
    private SharedPreferencesManager.Observer mPreferenceObserver;
    private boolean mComesFromNewTab;
    private boolean mIsTopSitesEnabled;
    private boolean mIsBraveStatsEnabled;
    private boolean mIsDisplayNews;
    private boolean mIsDisplayNewsOptin;
    private boolean mNewsFeedViewedOnce;

    private Supplier<Tab> mTabProvider;

    private static final int SHOW_BRAVE_RATE_ENTRY_AT = 10; // 10th row

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        mProfile = Profile.getLastUsedRegularProfile();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        mNTPBackgroundImagesBridge.setNewTabPageListener(mNewTabPageListener);
        mDatabaseHelper = DatabaseHelper.getInstance();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mComesFromNewTab = false;

        NTPUtil.showBREBottomBanner(this);
        mFeedHash = "";
        initBraveNewsController();
        try {
            if (BraveNewsUtils.shouldDisplayNews()
                    && BraveActivity.getBraveActivity().isLoadedFeed()) {
                CopyOnWriteArrayList<FeedItemsCard> existingNewsFeedObject =
                        BraveActivity.getBraveActivity().getNewsItemsFeedCards();
                if (existingNewsFeedObject != null) {
                    mNewsItemsFeedCard = existingNewsFeedObject;
                }
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onFinishInflate " + e);
        }
    }

    protected void updateTileGridPlaceholderVisibility() {
        // This function is kept empty to avoid placeholder implementation
    }

    private boolean shouldShowSuperReferral() {
        return mNTPBackgroundImagesBridge.isSuperReferral()
                && NTPBackgroundImagesBridge.enableSponsoredImages()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    }

    @Override
    public void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            ((BraveActivity) mActivity).showOnboardingV2(false);
        }
    }

    protected void insertSiteSectionView() {
        mMainLayout = findViewById(R.id.ntp_content);

        mMvTilesContainerLayout = (ViewGroup) LayoutInflater.from(mMainLayout.getContext())
                                          .inflate(R.layout.mv_tiles_container, mMainLayout, false);
        mMvTilesContainerLayout.setVisibility(View.VISIBLE);

        mMvTilesContainerLayout.post(new Runnable() {
            @Override
            public void run() {
                mMvTilesContainerLayout.addOnLayoutChangeListener(
                        (View view, int left, int top, int right, int bottom, int oldLeft,
                                int oldTop, int oldRight, int oldBottom) -> {
                            int oldHeight = oldBottom - oldTop;
                            int newHeight = bottom - top;

                            if (oldHeight != newHeight && mIsTopSitesEnabled
                                    && mNtpAdapter != null) {
                                new Handler(Looper.getMainLooper()).post(() -> {
                                    mNtpAdapter.notifyItemRangeChanged(mNtpAdapter.getStatsCount(),
                                            mNtpAdapter.getNewContentCount() + 2);
                                });
                            }
                        });
            }
        });

        // The page contents are initially hidden; otherwise they'll be drawn centered on the
        // page before the tiles are available and then jump upwards to make space once the
        // tiles are available.
        if (getVisibility() != View.VISIBLE) setVisibility(View.VISIBLE);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        if (mSponsoredTab == null) {
            initilizeSponsoredTab();
        }
        checkAndShowNTPImage(false);
        mNTPBackgroundImagesBridge.addObserver(mNTPBackgroundImageServiceObserver);

        if (OnboardingPrefManager.getInstance().isFromNotification() ) {
            ((BraveActivity)mActivity).showOnboardingV2(false);
            OnboardingPrefManager.getInstance().setFromNotification(false);
        }
        if (mBadgeAnimationView != null
                && !OnboardingPrefManager.getInstance().shouldShowBadgeAnimation()) {
            mBadgeAnimationView.setVisibility(View.INVISIBLE);
        }
        initBraveNewsController();
        if (BravePrefServiceBridge.getInstance().getNewsOptIn()) {
            new Handler(Looper.getMainLooper()).post(() -> {
                try {
                    Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                    if (tab != null && tab.getUrl().getSpec() != null
                            && UrlUtilities.isNTPUrl(tab.getUrl().getSpec())) {
                        // purges display ads on tab change
                        if (BraveActivity.getBraveActivity().getLastTabId() != tab.getId()) {
                            if (mBraveNewsController != null) {
                                mBraveNewsController.onDisplayAdPurgeOrphanedEvents();
                            }
                        }

                        BraveActivity.getBraveActivity().setLastTabId(tab.getId());
                    }
                } catch (BraveActivity.BraveActivityNotFoundException e) {
                    Log.e(TAG, "onAttachedToWindow " + e);
                }
            });
        }

        mIsDisplayNewsOptin = shouldDisplayNewsOptin();
        mIsDisplayNews = BraveNewsUtils.shouldDisplayNews();

        initPreferenceObserver();
        if (mPreferenceObserver != null) {
            SharedPreferencesManager.getInstance().addObserver(mPreferenceObserver);
        }
        setNtpViews();
    }

    @SuppressLint("ClickableViewAccessibility")
    private void setNtpViews() {
        mNewsSettingsBar = findViewById(R.id.news_settings_bar);
        // to make sure that tap on the settings bar doesn't go through and
        // trigger the article view
        mNewsSettingsBar.setOnClickListener(view -> {});

        // Double tap on the settings bar to scroll back up
        mNewsSettingsBar.setOnTouchListener(new OnTouchListener() {
            private GestureDetector mGestureDetector =
                    new GestureDetector(mActivity, new GestureDetector.SimpleOnGestureListener() {
                        @Override
                        public boolean onDoubleTap(MotionEvent e) {
                            mRecyclerView.smoothScrollToPosition(0);
                            return super.onDoubleTap(e);
                        }
                    });

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                mGestureDetector.onTouchEvent(event);
                return true;
            }
        });
        mNewContentLayout = findViewById(R.id.news_load_new_content);
        mNewContentText = findViewById(R.id.new_content_button_text);
        mNewContentProgressBar = findViewById(R.id.new_content_loading_spinner);
        mNewContentLayout.setOnClickListener(view -> { loadNewContent(); });
        ImageView ivNewsSettings = findViewById(R.id.news_settings_button);
        ivNewsSettings.setOnClickListener(view -> {
            SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
            settingsLauncher.launchSettingsActivity(getContext(), BraveNewsPreferencesV2.class);
        });

        mRecyclerView = findViewById(R.id.recyclerview);
        LinearLayoutManagerWrapper linearLayoutManager =
                new LinearLayoutManagerWrapper(mActivity, LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        mRecyclerView.post(new Runnable() {
            @Override
            public void run() {
                setNtpRecyclerView(linearLayoutManager);
            }
        });
    }

    private boolean shouldDisplayTopSites() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                BackgroundImagesPreferences.PREF_SHOW_TOP_SITES, true);
    }

    private boolean shouldDisplayBraveStats() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS, true);
    }

    private void setNtpRecyclerView(LinearLayoutManager linearLayoutManager) {
        mIsTopSitesEnabled = shouldDisplayTopSites();
        mIsBraveStatsEnabled = shouldDisplayBraveStats();

        if (mNtpAdapter == null) {
            if (mActivity != null && !mActivity.isDestroyed() && !mActivity.isFinishing()) {
                mNtpAdapter = new BraveNtpAdapter(mActivity, this, Glide.with(mActivity),
                        mNewsItemsFeedCard, mBraveNewsController, mMvTilesContainerLayout,
                        mNtpImageGlobal, mSponsoredTab, mWallpaper, mSponsoredLogo,
                        mNTPBackgroundImagesBridge, false, mRecyclerView.getHeight(),
                        mIsTopSitesEnabled, mIsBraveStatsEnabled, mIsDisplayNews,
                        mIsDisplayNewsOptin);

                mRecyclerView.setAdapter(mNtpAdapter);

                if (mRecyclerView.getItemAnimator() != null) {
                    RecyclerView.ItemAnimator itemAnimator = mRecyclerView.getItemAnimator();
                    if (itemAnimator instanceof SimpleItemAnimator) {
                        SimpleItemAnimator simpleItemAnimator = (SimpleItemAnimator) itemAnimator;
                        simpleItemAnimator.setSupportsChangeAnimations(false);
                    }
                }
            }
        } else {
            mNtpAdapter.setRecyclerViewHeight(mRecyclerView.getHeight());
            mNtpAdapter.setTopSitesEnabled(mIsTopSitesEnabled);
            mNtpAdapter.setBraveStatsEnabled(mIsBraveStatsEnabled);
            mNtpAdapter.setDisplayNews(mIsDisplayNews);
        }

        if (mNtpAdapter == null) return;

        if (mIsDisplayNews) {
            try {
                boolean isFeedLoaded = BraveActivity.getBraveActivity().isLoadedFeed();
                boolean isFromNewTab = BraveActivity.getBraveActivity().isComesFromNewTab();

                Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                int offsetPosition = (tab != null) ? SharedPreferencesManager.getInstance().readInt(
                                             BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(), 0)
                                                   : 0;

                int itemPosition = (tab != null) ? SharedPreferencesManager.getInstance().readInt(
                                           BRAVE_RECYCLERVIEW_POSITION + tab.getId(), 0)
                                                 : 0;

                if (offsetPosition == 0 && itemPosition == 0) {
                    isFeedLoaded = false;
                }

                if (!isFeedLoaded || isFromNewTab) {
                    mNtpAdapter.setNewsLoading(true);
                    getFeed(false);

                } else {
                    keepPosition();
                }
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "setNtpRecyclerView " + e);
            }
        } else {
            keepPosition();
        }

        mPrevVisibleNewsCardPosition = firstNewsFeedPosition() - 1;
        mRecyclerView.addOnScrollListener(new RecyclerView.OnScrollListener() {
            @Override
            public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState) {
                super.onScrollStateChanged(recyclerView, newState);

                int firstVisibleItemPosition = linearLayoutManager.findFirstVisibleItemPosition();

                int newsFeedPosition = firstNewsFeedPosition();

                if (newState == RecyclerView.SCROLL_STATE_IDLE) {
                    try {
                        if (BraveActivity.getBraveActivity().getActivityTab() != null
                                && mRecyclerView.getChildCount() > 0) {
                            View firstChild = mRecyclerView.getChildAt(0);
                            if (firstChild != null) {
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
                        }
                    } catch (BraveActivity.BraveActivityNotFoundException e) {
                        Log.e(TAG, "onScrollStateChanged " + e);
                    }
                }
                if (mIsDisplayNews && firstVisibleItemPosition >= newsFeedPosition - 1) {
                    if (!mNewsFeedViewedOnce && mBraveNewsController != null) {
                        // Brave News interaction started
                        mBraveNewsController.onInteractionSessionStarted();
                        mNewsFeedViewedOnce = true;
                    }
                    if (newState == RecyclerView.SCROLL_STATE_DRAGGING) {
                        mEndCardViewTime = System.currentTimeMillis();
                        long timeDiff = mEndCardViewTime - mStartCardViewTime;
                        // if viewed for more than 100 ms send the event
                        if (timeDiff > BraveNewsUtils.BRAVE_NEWS_VIEWD_CARD_TIME) {
                            if (mVisibleCard != null && mCardType != null) {
                                // send viewed cards events
                                if (mCardType.equals("promo") && !mCardType.equals("displayad")) {
                                    if (!mUuid.equals("") && !mCreativeInstanceId.equals("")) {
                                        mVisibleCard.setViewStatSent(true);
                                        if (mBraveNewsController != null) {
                                            mBraveNewsController.onPromotedItemView(
                                                    mUuid, mCreativeInstanceId);
                                        }
                                    }
                                }
                            }
                        }

                        int lastVisibleItemPosition =
                                linearLayoutManager.findLastCompletelyVisibleItemPosition();
                        if (mNewsItemsFeedCard != null && mNewsItemsFeedCard.size() > 0
                                && lastVisibleItemPosition >= newsFeedPosition
                                && lastVisibleItemPosition > mPrevVisibleNewsCardPosition) {
                            short newCardViews = 0;
                            for (int i = mPrevVisibleNewsCardPosition + 1;
                                    i <= lastVisibleItemPosition; i++) {
                                int itemCardPosition = i - newsFeedPosition;
                                if (itemCardPosition >= 0
                                        && itemCardPosition < mNewsItemsFeedCard.size()) {
                                    FeedItemsCard itemsCard =
                                            mNewsItemsFeedCard.get(itemCardPosition);
                                    if (itemsCard != null) {
                                        List<FeedItemCard> feedItems = itemsCard.getFeedItems();
                                        // Two items are shown as two cards side by side,
                                        // and three or more items is shown as one card as a list
                                        newCardViews =
                                                (short) (feedItems != null && feedItems.size() == 2
                                                                ? 2
                                                                : 1);
                                        mNewsSessionCardViews += newCardViews;
                                    }
                                }
                            }
                            if (mBraveNewsController != null) {
                                mBraveNewsController.onSessionCardViewsCountChanged(
                                        (short) mNewsSessionCardViews, newCardViews);
                            }
                            mPrevVisibleNewsCardPosition = lastVisibleItemPosition;
                        }
                    }

                    if (newState == RecyclerView.SCROLL_STATE_IDLE) {
                        mStartCardViewTime = System.currentTimeMillis();
                        int lastVisibleItemPosition =
                                linearLayoutManager.findLastVisibleItemPosition();

                        mFeedHash = SharedPreferencesManager.getInstance().readString(
                                BravePreferenceKeys.BRAVE_NEWS_FEED_HASH, "");
                        //@TODO alex optimize feed availability check
                        if (mBraveNewsController != null) {
                            mBraveNewsController.isFeedUpdateAvailable(
                                    mFeedHash, isNewsFeedAvailable -> {
                                        if (isNewsFeedAvailable) {
                                            mPrevVisibleNewsCardPosition =
                                                    mPrevVisibleNewsCardPosition + 1;

                                            setNewContentChanges(true);
                                        }
                                    });
                        }

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

                            int newsFeedViewPosition = viewPosition - newsFeedPosition;
                            if (newsFeedViewPosition >= 0
                                    && newsFeedViewPosition < mNewsItemsFeedCard.size()) {
                                if (visiblePercentageFinal >= MINIMUM_VISIBLE_HEIGHT_THRESHOLD) {
                                    mVisibleCard = mNewsItemsFeedCard.get(newsFeedViewPosition);
                                    // get params for view PROMOTED_ARTICLE
                                    if (mVisibleCard.getCardType() == CardType.PROMOTED_ARTICLE) {
                                        mItemPosition = newsFeedViewPosition;
                                        mCreativeInstanceId =
                                                BraveNewsUtils.getPromotionIdItem(mVisibleCard);
                                        mUuid = mVisibleCard.getUuid();
                                        mCardType = "promo";
                                    }

                                    // get params for view DISPLAY_AD
                                    if (mVisibleCard.getCardType() == CardType.DISPLAY_AD) {
                                        mItemPosition = newsFeedViewPosition;
                                        DisplayAd currentDisplayAd =
                                                BraveNewsUtils.getFromDisplayAdsMap(
                                                        newsFeedViewPosition);
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
                                                                            > MINIMUM_VISIBLE_HEIGHT_THRESHOLD
                                                                    && mBraveNewsController
                                                                            != null) {
                                                                mVisibleCard.setViewStatSent(true);
                                                                mBraveNewsController.onDisplayAdView(
                                                                        mUuid, mCreativeInstanceId);

                                                                insertAd();
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
            }

            private void insertAd() {
                DisplayAd currentDisplayAd = BraveNewsUtils.getFromDisplayAdsMap(mItemPosition);
                try {
                    mDatabaseHelper.insertAd(currentDisplayAd, mItemPosition,
                            BraveActivity.getBraveActivity().getActivityTab().getId());
                } catch (BraveActivity.BraveActivityNotFoundException e) {
                    Log.e(TAG, "insertAd " + e);
                }
            }

            @Override
            public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
                super.onScrolled(recyclerView, dx, dy);

                if (mIsDisplayNews) {
                    int lastVisibleItemPosition =
                            linearLayoutManager.findLastCompletelyVisibleItemPosition();

                    if (!mNtpAdapter.shouldDisplayNewsLoading() && mNewsItemsFeedCard != null
                            && mNewsItemsFeedCard.size() > 0
                            && lastVisibleItemPosition > mNtpAdapter.getStatsCount()
                                            + mNtpAdapter.getTopSitesCount()
                                            + mNtpAdapter.getNewContentCount()) {
                        if (mNewsSettingsBar.getVisibility() != View.VISIBLE) {
                            mNewsSettingsBar.setVisibility(View.VISIBLE);
                        }
                        mNtpAdapter.setImageCreditAlpha(0f);
                    } else if (lastVisibleItemPosition > -1) {
                        if (mNewsSettingsBar.getVisibility() != View.GONE) {
                            mNewsSettingsBar.setVisibility(View.GONE);
                        }
                        mNtpAdapter.setImageCreditAlpha(1f);
                    }

                    if (mNtpAdapter.isNewContent()) {
                        int firstVisibleItemPosition =
                                linearLayoutManager.findFirstVisibleItemPosition();

                        if (firstVisibleItemPosition
                                >= mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount()) {
                            mNewContentLayout.setVisibility(View.VISIBLE);
                        } else {
                            mNewContentLayout.setVisibility(View.GONE);
                        }
                    } else {
                        mNewContentLayout.setVisibility(View.GONE);
                    }
                } else if (mIsDisplayNewsOptin) {
                    int lastVisibleItemPosition =
                            linearLayoutManager.findLastCompletelyVisibleItemPosition();

                    if (lastVisibleItemPosition == mNtpAdapter.getItemCount() - 1) {
                        mNtpAdapter.setImageCreditAlpha(0f);
                    } else {
                        mNtpAdapter.setImageCreditAlpha(1f);
                    }
                }
            }
        });
    }

    private void keepPosition() {
        try {
            Tab tab = BraveActivity.getBraveActivity().getActivityTab();
            if (tab != null) {
                int itemPosition = SharedPreferencesManager.getInstance().readInt(
                        BRAVE_RECYCLERVIEW_POSITION + tab.getId(), 0);

                new Handler(Looper.getMainLooper()).postDelayed(() -> {
                    if (mNtpAdapter != null && mNtpAdapter.getItemCount() > itemPosition) {
                        RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
                        if (manager instanceof LinearLayoutManager) {
                            int offsetPosition = SharedPreferencesManager.getInstance().readInt(
                                    BRAVE_RECYCLERVIEW_OFFSET_POSITION + tab.getId(), 0);

                            if (itemPosition
                                    == mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount()
                                            + mNtpAdapter.getNewContentCount()) {
                                offsetPosition -= mNtpAdapter.getTopMarginImageCredit();
                            }

                            LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
                            linearLayoutManager.scrollToPositionWithOffset(
                                    itemPosition, offsetPosition);
                        }
                    }
                }, 10);
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "keepPosition " + e);
        }
    }

    private int firstNewsFeedPosition() {
        if (mNtpAdapter != null) {
            return mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount()
                    + mNtpAdapter.getNewContentCount() + 1;
        }
        return 0;
    }

    @Override
    public void updateNewsOptin(boolean isOptin) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(BraveNewsPreferencesV2.PREF_SHOW_OPTIN, false);
        sharedPreferencesEditor.apply();
        if (isOptin) {
            BravePrefServiceBridge.getInstance().setNewsOptIn(true);
        }
        BravePrefServiceBridge.getInstance().setShowNews(isOptin);

        mIsDisplayNewsOptin = false;
        mIsDisplayNews = isOptin;
        mNtpAdapter.removeNewsOptin();
        mNtpAdapter.setImageCreditAlpha(1f);
        mNtpAdapter.setDisplayNews(mIsDisplayNews);

        if (isOptin && mBraveNewsController != null && BraveNewsUtils.getLocale() == null) {
            BraveNewsUtils.getBraveNewsSettingsData(mBraveNewsController, null);
        }
    }

    private boolean shouldDisplayNewsOptin() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                BraveNewsPreferencesV2.PREF_SHOW_OPTIN, true);
    }

    private void initPreferenceObserver() {
        mPreferenceObserver = (key) -> {
            if (TextUtils.equals(key, BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE)) {
                if (SharedPreferencesManager.getInstance().readBoolean(
                            BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, false)) {
                    new Handler(Looper.getMainLooper()).postDelayed(() -> {
                        mPrevVisibleNewsCardPosition = mPrevVisibleNewsCardPosition + 1;
                        setNewContentChanges(true);
                    }, 10);
                }

            } else if (TextUtils.equals(key, BravePreferenceKeys.BRAVE_NEWS_PREF_SHOW_NEWS)) {
                new Handler(Looper.getMainLooper()).postDelayed(() -> { refreshFeed(); }, 10);
            } else if (TextUtils.equals(key, BackgroundImagesPreferences.PREF_SHOW_TOP_SITES)) {
                mIsTopSitesEnabled = shouldDisplayTopSites();
                mNtpAdapter.setTopSitesEnabled(mIsTopSitesEnabled);
            } else if (TextUtils.equals(key, BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS)) {
                mIsBraveStatsEnabled = shouldDisplayBraveStats();
                mNtpAdapter.setBraveStatsEnabled(mIsBraveStatsEnabled);
            }
        };
    }

    @Override
    protected void onDetachedFromWindow() {
        if (mWorkerTask != null && mWorkerTask.getStatus() == AsyncTask.Status.RUNNING) {
            mWorkerTask.cancel(true);
            mWorkerTask = null;
        }

        if (!mIsFromBottomSheet) {
            setBackgroundResource(0);
            if (mImageDrawable != null && mImageDrawable.getBitmap() != null
                    && !mImageDrawable.getBitmap().isRecycled()) {
                mImageDrawable.getBitmap().recycle();
            }
        }
        mNTPBackgroundImagesBridge.removeObserver(mNTPBackgroundImageServiceObserver);

        if (mNewsItemsFeedCard != null && mNewsItemsFeedCard.size() > 0) {
            try {
                BraveActivity.getBraveActivity().setNewsItemsFeedCards(mNewsItemsFeedCard);
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "onDetachedFromWindow " + e);
            }
        }

        if (mBraveNewsController != null) {
            mBraveNewsController.close();
            mBraveNewsController = null;
        }

        // removes preference observer
        SharedPreferencesManager.getInstance().removeObserver(mPreferenceObserver);
        mPreferenceObserver = null;

        mRecyclerView.clearOnScrollListeners();
        super.onDetachedFromWindow();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (mSponsoredTab != null && NTPUtil.shouldEnableNTPFeature()) {
            NTPImage ntpImage = mSponsoredTab.getTabNTPImage(false);
            if (ntpImage == null) {
                mSponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            } else if (ntpImage instanceof Wallpaper) {
                Wallpaper mWallpaper = (Wallpaper) ntpImage;
                if (mWallpaper == null) {
                    mSponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
                }
            }
            checkForNonDisruptiveBanner(ntpImage);
            super.onConfigurationChanged(newConfig);
            showNTPImage(ntpImage);

            new Handler(Looper.getMainLooper()).postDelayed(() -> {
                if (mNtpAdapter != null) {
                    mNtpAdapter.setRecyclerViewHeight(mRecyclerView.getHeight());
                }
                keepPosition();
            }, 10);
        } else {
            super.onConfigurationChanged(newConfig);
        }
    }

    @Override
    public void loadNewContent() {
        mNtpAdapter.setNewContentLoading(true);
        mNewContentText.setVisibility(View.GONE);
        mNewContentProgressBar.setVisibility(View.VISIBLE);
        mNewContentLayout.setClickable(false);
        SharedPreferencesManager.getInstance().writeBoolean(
                BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, false);

        getFeed(true);
    }

    @Override
    public void getFeed(boolean isNewContent) {
        if (!isNewContent) {
            mNtpAdapter.setImageCreditAlpha(1f);
            mNtpAdapter.setNewsLoading(true);
        }
        initBraveNewsController();
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> { mBraveNewsController.getFeed(feed -> { runFeed(isNewContent, feed); }); });
    }

    private void runFeed(boolean isNewContent, Feed feed) {
        if (feed == null) {
            processFeed(isNewContent);
            return;
        }

        mFeedHash = feed.hash;
        int newsItemsFeedCardSize = mNewsItemsFeedCard.size();
        new Handler(Looper.getMainLooper()).post(() -> {
            mNtpAdapter.notifyItemRangeRemoved(
                    mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount() + 1,
                    newsItemsFeedCardSize);
        });

        mNewsItemsFeedCard.clear();
        BraveNewsUtils.initCurrentAds();
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

        if (mNewsItemsFeedCard.size() > 0 || (feed.pages != null && feed.pages.length > 0)) {
            //  adds empty card to trigger Display ad call for the second card, when the
            //  user starts scrolling
            FeedItemsCard displayAdCard = new FeedItemsCard();
            DisplayAd displayAd = new DisplayAd();
            displayAdCard.setCardType(CardType.DISPLAY_AD);
            displayAdCard.setDisplayAd(displayAd);
            displayAdCard.setUuid(UUID.randomUUID().toString());
            mNewsItemsFeedCard.add(displayAdCard);
        }

        // start page loop
        int noPages = 0;
        int itemIndex = 0;
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

                // For show brave rating UI in news list at 10 th row
                if (RateUtils.getInstance().shouldShowRateDialog(mActivity)
                        && mNewsItemsFeedCard.size() == SHOW_BRAVE_RATE_ENTRY_AT) {
                    // Dummy entry for Rating prompt
                    FeedItemsCard dummy = new FeedItemsCard();
                    dummy.setCardType(CardBuilderFeedCard.CARDTYPE_BRAVE_RATING);
                    dummy.setUuid(UUID.randomUUID().toString());
                    mNewsItemsFeedCard.add(dummy);
                }
            }
        } // end page loop

        processFeed(isNewContent);
        try {
            BraveActivity.getBraveActivity().setNewsItemsFeedCards(mNewsItemsFeedCard);
            BraveActivity.getBraveActivity().setLoadedFeed(true);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "getFeed " + e);
        }
    }

    private void refreshFeed() {
        boolean isShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();
        mIsDisplayNews = BraveNewsUtils.shouldDisplayNews();
        if (!isShowNewsOn) {
            mNtpAdapter.setDisplayNews(mIsDisplayNews);

            if (mNtpAdapter.isNewContent()) {
                mPrevVisibleNewsCardPosition = mPrevVisibleNewsCardPosition - 1;
                setNewContentChanges(false);
            }
            mNtpAdapter.setImageCreditAlpha(1f);
            mNewsSettingsBar.setVisibility(View.GONE);
            return;
        }

        if (mIsDisplayNews) {
            if (mIsDisplayNewsOptin) {
                mIsDisplayNewsOptin = false;
                mNtpAdapter.removeNewsOptin();
                mNtpAdapter.setImageCreditAlpha(1f);
            }
            mNtpAdapter.setDisplayNews(mIsDisplayNews);
            getFeed(false);
        }
    }

    private void processFeed(boolean isNewContent) {
        new Handler(Looper.getMainLooper()).post(() -> {
            mNtpAdapter.setNewsLoading(false);
            if (mNewsItemsFeedCard != null && mNewsItemsFeedCard.size() > 0) {
                mNtpAdapter.notifyItemRangeChanged(
                        mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount(),
                        mNtpAdapter.getItemCount() - mNtpAdapter.getStatsCount()
                                - mNtpAdapter.getTopSitesCount());
            }

            if (isNewContent) {
                mPrevVisibleNewsCardPosition = mPrevVisibleNewsCardPosition - 1;
                setNewContentChanges(false);
                RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
                if (manager instanceof LinearLayoutManager) {
                    LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
                    linearLayoutManager.scrollToPositionWithOffset(
                            mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount() + 1,
                            dpToPx(mActivity, 60));
                }
            }
            try {
                BraveActivity.getBraveActivity().setComesFromNewTab(false);
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "processFeed " + e);
            }
        });
    }

    private void setNewContentChanges(boolean isNewContent) {
        if (isNewContent) {
            if (mNtpAdapter != null) {
                mNtpAdapter.setNewContent(true);
                RecyclerView.LayoutManager manager = mRecyclerView.getLayoutManager();
                if (manager instanceof LinearLayoutManager) {
                    LinearLayoutManager linearLayoutManager = (LinearLayoutManager) manager;
                    int firstVisibleItemPosition =
                            linearLayoutManager.findFirstVisibleItemPosition();

                    if (firstVisibleItemPosition
                            >= mNtpAdapter.getStatsCount() + mNtpAdapter.getTopSitesCount()) {
                        mNewContentLayout.setVisibility(View.VISIBLE);
                    }
                }
            }

        } else {
            if (mNtpAdapter != null) {
                mNtpAdapter.setNewContent(false);
            }
            mNewContentLayout.setVisibility(View.GONE);
            mNewContentProgressBar.setVisibility(View.GONE);
            mNewContentText.setVisibility(View.VISIBLE);
            mNewContentLayout.setClickable(true);
        }
    }

    @Override
    public void initialize(NewTabPageManager manager, Activity activity, Delegate tileGroupDelegate,
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle,
            FeedSurfaceScrollDelegate scrollDelegate, TouchEnabledDelegate touchEnabledDelegate,
            UiConfig uiConfig, ActivityLifecycleDispatcher lifecycleDispatcher, NewTabPageUma uma,
            boolean isIncognito, WindowAndroid windowAndroid, boolean isNtpAsHomeSurfaceEnabled,
            boolean isSurfacePolishEnabled, boolean isSurfacePolishOmniboxColorEnabled) {
        super.initialize(manager, activity, tileGroupDelegate, searchProviderHasLogo,
                searchProviderIsGoogle, scrollDelegate, touchEnabledDelegate, uiConfig,
                lifecycleDispatcher, uma, isIncognito, windowAndroid, isNtpAsHomeSurfaceEnabled,
                isSurfacePolishEnabled, isSurfacePolishOmniboxColorEnabled);

        assert mMvTilesContainerLayout != null : "Something has changed in the upstream!";

        if (mMvTilesContainerLayout != null && !isScrollableMvtEnabled()) {
            ViewGroup tilesLayout = mMvTilesContainerLayout.findViewById(R.id.mv_tiles_layout);

            assert tilesLayout
                    instanceof MostVisitedTilesGridLayout
                : "Something has changed in the upstream!";

            if (tilesLayout instanceof MostVisitedTilesGridLayout) {
                ((MostVisitedTilesGridLayout) tilesLayout)
                        .setMaxRows(
                                BraveQueryTileSection.getMaxRowsForMostVisitedTiles(getContext()));
            }
        }

        assert (activity instanceof BraveActivity);
        mActivity = activity;
        ((BraveActivity) mActivity).dismissShieldsTooltip();
        ((BraveActivity) mActivity).setNewTabPageManager(manager);
    }

    public void setTabProvider(Supplier<Tab> tabProvider) {
        mTabProvider = tabProvider;
    }

    private void showNTPImage(NTPImage ntpImage) {
        Display display = mActivity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        mNtpImageGlobal = ntpImage;
        if (mNtpAdapter != null) {
            mNtpAdapter.setNtpImage(ntpImage);
        }
        if (ntpImage instanceof Wallpaper
                && NTPUtil.isReferralEnabled()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            setBackgroundImage(ntpImage);

        } else if (UserPrefs.get(Profile.getLastUsedRegularProfile())
                           .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                && mSponsoredTab != null && NTPUtil.shouldEnableNTPFeature()) {
            setBackgroundImage(ntpImage);
        }
    }

    private void setBackgroundImage(NTPImage ntpImage) {
        mBgImageView = (ImageView) findViewById(R.id.bg_image_view);
        mBgImageView.setScaleType(ImageView.ScaleType.MATRIX);

        ViewTreeObserver observer = mBgImageView.getViewTreeObserver();
        observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                mWorkerTask =
                        new FetchWallpaperWorkerTask(ntpImage, mBgImageView.getMeasuredWidth(),
                                mBgImageView.getMeasuredHeight(), mWallpaperRetrievedCallback);
                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

                mBgImageView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }
        });
    }

    private void checkForNonDisruptiveBanner(NTPImage ntpImage) {
        int brOption = NTPUtil.checkForNonDisruptiveBanner(ntpImage, mSponsoredTab);
        if (SponsoredImageUtil.BR_INVALID_OPTION != brOption && !NTPUtil.isReferralEnabled()
                && ((BraveRewardsHelper.isRewardsEnabled()
                        || BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()))
                && (!ContextUtils.getAppSharedPreferences().getBoolean(
                            BraveNewsPreferencesV2.PREF_SHOW_OPTIN, true)
                        && !BravePrefServiceBridge.getInstance().getShowNews())) {
            NTPUtil.showNonDisruptiveBanner(
                    (BraveActivity) mActivity, this, brOption, mSponsoredTab, mNewTabPageListener);
        }
    }

    private void checkAndShowNTPImage(boolean isReset) {
        NTPImage ntpImage = mSponsoredTab.getTabNTPImage(isReset);
        if (ntpImage == null) {
            mSponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
        } else if (ntpImage instanceof Wallpaper) {
            Wallpaper mWallpaper = (Wallpaper) ntpImage;
            if (mWallpaper == null) {
                mSponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            }
        }
        checkForNonDisruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(getTab()).get(String.valueOf(getTabImpl().getId())) == null) {
            SponsoredTab sponsoredTab = new SponsoredTab(mNTPBackgroundImagesBridge);
            TabAttributes.from(getTab()).set(String.valueOf(getTabImpl().getId()), sponsoredTab);
        }
        mSponsoredTab = TabAttributes.from(getTab()).get(String.valueOf((getTabImpl()).getId()));
        if (shouldShowSuperReferral()) mNTPBackgroundImagesBridge.getTopSites();
    }

    private NewTabPageListener mNewTabPageListener = new NewTabPageListener() {
        @Override
        public void updateInteractableFlag(boolean isBottomSheet) {
            mIsFromBottomSheet = isBottomSheet;
        }

        @Override
        public void updateNTPImage() {
            if (mSponsoredTab == null) {
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

    private FetchWallpaperWorkerTask.WallpaperRetrievedCallback mWallpaperRetrievedCallback =
            new FetchWallpaperWorkerTask.WallpaperRetrievedCallback() {
                @Override
                public void bgWallpaperRetrieved(Bitmap bgWallpaper) {
                    if (mBgImageView != null) {
                        mBgImageView.setImageBitmap(bgWallpaper);
                    }
                }

                @Override
                public void logoRetrieved(Wallpaper wallpaper, Bitmap logoWallpaper) {
                    if (!NTPUtil.isReferralEnabled()) {
                        mWallpaper = wallpaper;
                        mSponsoredLogo = logoWallpaper;
                        if (mNtpAdapter != null) {
                            mNtpAdapter.setSponsoredLogo(mWallpaper, logoWallpaper);
                        }
                    }
                }
            };

    private void loadTopSites(List<TopSiteTable> topSites) {
        mSuperReferralSitesLayout = new LinearLayout(mActivity);
        mSuperReferralSitesLayout.setWeightSum(1f);
        mSuperReferralSitesLayout.setOrientation(LinearLayout.HORIZONTAL);
        mSuperReferralSitesLayout.setBackgroundColor(
                mActivity.getResources().getColor(R.color.topsite_bg_color));

        LayoutInflater inflater =
                (LayoutInflater) mActivity.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        for (TopSiteTable topSite : topSites) {
            final View tileView = inflater.inflate(R.layout.suggestions_tile_view, null);

            TextView tileViewTitleTv = tileView.findViewById(R.id.tile_view_title);
            tileViewTitleTv.setText(topSite.getName());
            tileViewTitleTv.setTextColor(
                    getResources().getColor(R.color.brave_state_time_count_color));

            ImageView iconIv = tileView.findViewById(R.id.tile_view_icon);
            if (NTPUtil.imageCache.get(topSite.getDestinationUrl()) == null) {
                NTPUtil.imageCache.put(topSite.getDestinationUrl(),
                        new java.lang.ref.SoftReference(
                                NTPUtil.getTopSiteBitmap(topSite.getImagePath())));
            }
            iconIv.setImageBitmap(NTPUtil.imageCache.get(topSite.getDestinationUrl()).get());
            iconIv.setBackgroundColor(mActivity.getResources().getColor(android.R.color.white));
            iconIv.setClickable(false);

            tileView.setOnClickListener(
                    view -> { TabUtils.openUrlInSameTab(topSite.getDestinationUrl()); });

            tileView.setPadding(0, dpToPx(mActivity, 12), 0, 0);

            LinearLayout.LayoutParams layoutParams =
                    new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT);
            layoutParams.weight = 0.25f;
            layoutParams.gravity = Gravity.CENTER;
            tileView.setLayoutParams(layoutParams);
            tileView.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
                @Override
                public void onCreateContextMenu(
                        ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                    menu.add(R.string.contextmenu_open_in_new_tab)
                            .setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {
                                    TabUtils.openUrlInNewTab(false, topSite.getDestinationUrl());
                                    return true;
                                }
                            });
                    menu.add(R.string.contextmenu_open_in_incognito_tab)
                            .setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {
                                    TabUtils.openUrlInNewTab(true, topSite.getDestinationUrl());
                                    return true;
                                }
                            });
                    menu.add(R.string.contextmenu_save_link)
                            .setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {
                                    if (getTab() != null) {
                                        OfflinePageBridge.getForProfile(mProfile).scheduleDownload(
                                                getTab().getWebContents(),
                                                OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE,
                                                topSite.getDestinationUrl(),
                                                DownloadUiActionFlags.ALL);
                                    } else {
                                        RequestCoordinatorBridge.getForProfile(mProfile)
                                                .savePageLater(topSite.getDestinationUrl(),
                                                        OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE,
                                                        true /* userRequested */);
                                    }
                                    return true;
                                }
                            });
                    menu.add(R.string.remove)
                            .setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {
                                    NTPUtil.imageCache.remove(topSite.getDestinationUrl());
                                    mDatabaseHelper.deleteTopSite(topSite.getDestinationUrl());
                                    NTPUtil.addToRemovedTopSite(topSite.getDestinationUrl());
                                    mSuperReferralSitesLayout.removeView(tileView);
                                    return true;
                                }
                            });
                }
            });
            mSuperReferralSitesLayout.addView(tileView);
        }
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
    public void onConnectionError(MojoException e) {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
        initBraveNewsController();
    }

    private void initBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        mBraveNewsController =
                BraveNewsControllerFactory.getInstance().getBraveNewsController(this);

        if (mNtpAdapter != null) {
            mNtpAdapter.setBraveNewsController(mBraveNewsController);
        }
    }

    protected boolean isScrollableMvtEnabled() {
        return ChromeFeatureList.isEnabled(ChromeFeatureList.SHOW_SCROLLABLE_MVT_ON_NTP_ANDROID)
                && !DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext)
                && UserPrefs.get(Profile.getLastUsedRegularProfile())
                           .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE);
    }

    @Override
    void setSearchProviderTopMargin(int topMargin) {
        if (mLogoCoordinator != null) mLogoCoordinator.setTopMargin(topMargin);
    }

    @Override
    void setSearchProviderBottomMargin(int bottomMargin) {
        if (mLogoCoordinator != null) mLogoCoordinator.setBottomMargin(bottomMargin);
    }
}
