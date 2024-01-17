/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.widget.ImageViewCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.RequestManager;
import com.google.android.material.floatingactionbutton.FloatingActionButton;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.QRCodeShareDialogFragment;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_news.CardBuilderFeedCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.user_prefs.UserPrefs;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

public class BraveNtpAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private Activity mActivity;
    private RequestManager mGlide;
    private BraveNewsController mBraveNewsController;
    private View mMvTilesContainerLayout;
    private CopyOnWriteArrayList<FeedItemsCard> mNewsItems;
    private NTPImage mNtpImage;
    private SponsoredTab mSponsoredTab;
    private Bitmap mSponsoredLogo;
    private Wallpaper mWallpaper;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private OnBraveNtpListener mOnBraveNtpListener;
    private boolean mIsDisplayNewsFeed;
    private boolean mIsDisplayNewsOptin;
    private boolean mIsNewsLoading;
    private boolean mIsNewContent;
    private boolean mIsNewContentLoading;
    private boolean mIsTopSitesEnabled;
    private boolean mIsBraveStatsEnabled;
    private int mRecyclerViewHeight;
    private int mStatsHeight;
    private int mTopSitesHeight;
    private int mNewContentHeight;
    private int mTopMarginImageCredit;
    private float mImageCreditAlpha = 1f;

    private static int TYPE_STATS = 1;
    private static int TYPE_TOP_SITES = 2;
    private static int TYPE_NEW_CONTENT = 3;
    private static int TYPE_IMAGE_CREDIT = 4;
    private static int TYPE_NEWS_OPTIN = 5;
    private static int TYPE_NEWS_LOADING = 6;
    private static int TYPE_NEWS = 7;
    private static int TYPE_NEWS_NO_CONTENT_SOURCES = 8;

    private static final int ONE_ITEM_SPACE = 1;
    private static final int TWO_ITEMS_SPACE = 2;
    private static final String TAG = "BraveNtpAdapter";

    public BraveNtpAdapter(Activity activity, OnBraveNtpListener onBraveNtpListener,
            RequestManager glide, CopyOnWriteArrayList<FeedItemsCard> newsItems,
            BraveNewsController braveNewsController, View mvTilesContainerLayout, NTPImage ntpImage,
            SponsoredTab sponsoredTab, Wallpaper wallpaper, Bitmap sponsoredLogo,
            NTPBackgroundImagesBridge nTPBackgroundImagesBridge, boolean isNewsLoading,
            int recyclerViewHeight, boolean isTopSitesEnabled, boolean isBraveStatsEnabled,
            boolean isDisplayNewsFeed, boolean isDisplayNewsOptin) {
        mActivity = activity;
        mOnBraveNtpListener = onBraveNtpListener;
        mGlide = glide;
        mNewsItems = newsItems;
        mBraveNewsController = braveNewsController;
        mMvTilesContainerLayout = mvTilesContainerLayout;
        mNtpImage = ntpImage;
        mSponsoredTab = sponsoredTab;
        mWallpaper = wallpaper;
        mSponsoredLogo = sponsoredLogo;
        mNTPBackgroundImagesBridge = nTPBackgroundImagesBridge;
        mIsNewsLoading = isNewsLoading;
        mRecyclerViewHeight = recyclerViewHeight;
        mIsTopSitesEnabled = isTopSitesEnabled;
        mIsBraveStatsEnabled = isBraveStatsEnabled;
        mIsDisplayNewsFeed = isDisplayNewsFeed;
        mIsDisplayNewsOptin = isDisplayNewsOptin;
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        if (holder instanceof StatsViewHolder) {
            StatsViewHolder statsViewHolder = (StatsViewHolder) holder;

            statsViewHolder.hideStatsImg.setOnClickListener(
                    view -> {
                        ChromeSharedPreferences.getInstance()
                                .writeBoolean(
                                        BackgroundImagesPreferences.PREF_SHOW_BRAVE_STATS, false);
                    });
            List<Pair<String, String>> statsPairs = BraveStatsUtil.getStatsPairs();

            statsViewHolder.adsBlockedCountTv.setText(statsPairs.get(0).first);
            statsViewHolder.dataSavedValueTv.setText(statsPairs.get(1).first);
            statsViewHolder.estTimeSavedCountTv.setText(statsPairs.get(2).first);
            statsViewHolder.adsBlockedCountTextTv.setText(statsPairs.get(0).second);
            statsViewHolder.dataSavedValueTextTv.setText(statsPairs.get(1).second);
            statsViewHolder.estTimeSavedCountTextTv.setText(statsPairs.get(2).second);

            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            int margin = dpToPx(mActivity, 16);
            layoutParams.setMargins(margin, margin, margin, 0);
            statsViewHolder.ntpStatsLayout.setLayoutParams(layoutParams);
            statsViewHolder.ntpStatsLayout.setOnClickListener(
                    view -> { mOnBraveNtpListener.checkForBraveStats(); });

            mStatsHeight = NTPUtil.getViewHeight(statsViewHolder.itemView) + margin;

        } else if (holder instanceof TopSitesViewHolder) {
            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            int margin = dpToPx(mActivity, 16);
            layoutParams.setMargins(margin, margin, margin, 0);

            mMvTilesContainerLayout.setLayoutParams(layoutParams);
            mMvTilesContainerLayout.setBackgroundResource(R.drawable.rounded_dark_bg_alpha);
            mTopSitesHeight = NTPUtil.getViewHeight(holder.itemView) + margin;

        } else if (holder instanceof NewContentViewHolder) {
            NewContentViewHolder newContentViewHolder = (NewContentViewHolder) holder;

            newContentViewHolder.newContentLayout.setOnClickListener(
                    view -> { mOnBraveNtpListener.loadNewContent(); });

            if (mIsNewContentLoading) {
                newContentViewHolder.newContentLayout.setClickable(false);
                newContentViewHolder.newContentText.setVisibility(View.GONE);
                newContentViewHolder.newContentProgressBar.setVisibility(View.VISIBLE);
            } else {
                newContentViewHolder.newContentLayout.setClickable(true);
                newContentViewHolder.newContentText.setVisibility(View.VISIBLE);
                newContentViewHolder.newContentProgressBar.setVisibility(View.GONE);
            }
            mNewContentHeight =
                    NTPUtil.getViewHeight(newContentViewHolder.itemView) + dpToPx(mActivity, 10);

        } else if (holder instanceof ImageCreditViewHolder) {
            ImageCreditViewHolder imageCreditViewHolder = (ImageCreditViewHolder) holder;

            if (mNtpImage instanceof Wallpaper && NTPUtil.isReferralEnabled()
                    && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                imageCreditViewHolder.superReferralLogo.setVisibility(View.VISIBLE);
                imageCreditViewHolder.creditTv.setVisibility(View.GONE);
                int floatingButtonIcon = R.drawable.ic_qr_code;
                imageCreditViewHolder.superReferralLogo.setImageResource(floatingButtonIcon);
                int floatingButtonIconColor =
                        GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                        ? android.R.color.white
                        : android.R.color.black;
                ImageViewCompat.setImageTintList(
                        imageCreditViewHolder.superReferralLogo,
                        ColorStateList.valueOf(mActivity.getColor(floatingButtonIconColor)));
                imageCreditViewHolder.superReferralLogo.setOnClickListener(view -> {
                    QRCodeShareDialogFragment qRCodeShareDialogFragment =
                            new QRCodeShareDialogFragment();
                    qRCodeShareDialogFragment.setQRCodeText(BraveConstants.BRAVE_REF_URL
                            + mNTPBackgroundImagesBridge.getSuperReferralCode());
                    qRCodeShareDialogFragment.show(
                            ((BraveActivity) mActivity).getSupportFragmentManager(),
                            "QRCodeShareDialogFragment");
                });

            } else if (UserPrefs.get(Profile.getLastUsedRegularProfile())
                               .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                    && mSponsoredTab != null && NTPUtil.shouldEnableNTPFeature()) {
                if (mNtpImage instanceof BackgroundImage) {
                    BackgroundImage backgroundImage = (BackgroundImage) mNtpImage;
                    imageCreditViewHolder.sponsoredLogo.setVisibility(View.GONE);
                    imageCreditViewHolder.superReferralLogo.setVisibility(View.GONE);

                    if (backgroundImage.getImageCredit() != null) {
                        String imageCreditStr = String.format(mActivity.getResources().getString(
                                R.string.photo_by, backgroundImage.getImageCredit().getName()));

                        SpannableStringBuilder spannableString =
                                new SpannableStringBuilder(imageCreditStr);
                        spannableString.setSpan(
                                new android.text.style.StyleSpan(android.graphics.Typeface.BOLD),
                                ((imageCreditStr.length() - 1)
                                        - (backgroundImage.getImageCredit().getName().length()
                                                - 1)),
                                imageCreditStr.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                        imageCreditViewHolder.creditTv.setText(spannableString);
                        imageCreditViewHolder.creditTv.setVisibility(View.VISIBLE);

                        imageCreditViewHolder.creditTv.setOnClickListener(view -> {
                            if (backgroundImage.getImageCredit() != null) {
                                TabUtils.openUrlInSameTab(
                                        backgroundImage.getImageCredit().getUrl());
                            }
                        });
                    }
                }
            }
            if (!NTPUtil.isReferralEnabled() && mSponsoredLogo != null) {
                imageCreditViewHolder.sponsoredLogo.setVisibility(View.VISIBLE);
                imageCreditViewHolder.sponsoredLogo.setImageBitmap(mSponsoredLogo);
                imageCreditViewHolder.sponsoredLogo.setOnClickListener(view -> {
                    if (mWallpaper.getLogoDestinationUrl() != null) {
                        TabUtils.openUrlInSameTab(mWallpaper.getLogoDestinationUrl());
                        mNTPBackgroundImagesBridge.wallpaperLogoClicked(mWallpaper);
                    }
                });
            }

            if (mRecyclerViewHeight > 0) {
                LinearLayout.LayoutParams layoutParams =
                        new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                LinearLayout.LayoutParams.WRAP_CONTENT);

                int extraMarginForNews =
                        (mIsDisplayNewsOptin || shouldDisplayNewsLoading() || mIsDisplayNewsFeed)
                        ? dpToPx(mActivity, 30)
                        : 0;

                mTopMarginImageCredit = mRecyclerViewHeight
                        - NTPUtil.getViewHeight(imageCreditViewHolder.itemView)
                        - extraMarginForNews;

                if (isStatsEnabled()) {
                    mTopMarginImageCredit -= mStatsHeight;
                } else {
                    mTopMarginImageCredit -= dpToPx(mActivity, 16);
                }

                if (mIsTopSitesEnabled) {
                    mTopMarginImageCredit -= mTopSitesHeight;
                }

                if (mIsNewContent) {
                    mTopMarginImageCredit -= mNewContentHeight;
                }

                if (mTopMarginImageCredit < 0) {
                    mTopMarginImageCredit = 0;
                }

                layoutParams.setMargins(0, mTopMarginImageCredit, 0, 0);

                imageCreditViewHolder.ntpImageCreditLayout.setLayoutParams(layoutParams);
            }
            imageCreditViewHolder.imageCreditLayout.setAlpha(mImageCreditAlpha);

        } else if (holder instanceof NewsOptinViewHolder) {
            NewsOptinViewHolder newsOptinViewHolder = (NewsOptinViewHolder) holder;

            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            int margin = dpToPx(mActivity, 30);
            layoutParams.setMargins(margin, 0, margin, margin);

            newsOptinViewHolder.itemView.setLayoutParams(layoutParams);

            newsOptinViewHolder.optinClose.setOnClickListener(
                    view -> { mOnBraveNtpListener.updateNewsOptin(false); });

            newsOptinViewHolder.optinLearnMore.setOnClickListener(view -> {
                TabUtils.openUrlInSameTab(BraveConstants.BRAVE_NEWS_LEARN_MORE_URL);
            });

            newsOptinViewHolder.optinButton.setOnClickListener(view -> {
                mOnBraveNtpListener.updateNewsOptin(true);
                mOnBraveNtpListener.getFeed(false);
            });

        } else if (holder instanceof NewsViewHolder) {
            NewsViewHolder newsViewHolder = (NewsViewHolder) holder;
            newsViewHolder.linearLayout.removeAllViews();

            int newsLoadingCount = shouldDisplayNewsLoading() ? 1 : 0;
            int newsPosition =
                    position
                            - getStatsCount()
                            - getTopSitesCount()
                            - ONE_ITEM_SPACE
                            - getNewContentCount()
                            - newsLoadingCount;
            if (newsPosition < mNewsItems.size()) {
                FeedItemsCard newsItem = mNewsItems.get(newsPosition);
                if (mBraveNewsController != null) {
                    new CardBuilderFeedCard(mBraveNewsController, mGlide,
                            newsViewHolder.linearLayout, mActivity, newsPosition, newsItem,
                            newsItem.getCardType());
                }
            }
        } else if (holder instanceof NoSourcesViewHolder) {
            NoSourcesViewHolder noSourcesViewHolder = (NoSourcesViewHolder) holder;

            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            int margin = dpToPx(mActivity, 30);
            layoutParams.setMargins(margin, 0, margin, margin);

            noSourcesViewHolder.itemView.setLayoutParams(layoutParams);

            noSourcesViewHolder.btnChooseContent.setOnClickListener(view -> {
                if (mActivity instanceof BraveActivity) {
                    ((BraveActivity) mActivity).openBraveNewsSettings();
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        int statsCount = getStatsCount();
        int topSitesCount = getTopSitesCount();
        int newsLoadingCount = shouldDisplayNewsLoading() ? 1 : 0;
        if (mIsDisplayNewsOptin) {
            return statsCount + topSitesCount + TWO_ITEMS_SPACE + newsLoadingCount;
        } else if (mIsDisplayNewsFeed) {
            int newsCount = 0;
            if (mNewsItems.size() > 0) {
                newsCount = mNewsItems.size();
            } else if (newsLoadingCount == 0) {
                newsCount = 1;
            }
            return statsCount + topSitesCount + ONE_ITEM_SPACE + getNewContentCount()
                    + newsLoadingCount + newsCount;
        } else {
            return statsCount + topSitesCount + ONE_ITEM_SPACE + newsLoadingCount;
        }
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view;
        if (viewType == TYPE_STATS) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.brave_stats_layout, parent, false);
            return new StatsViewHolder(view);

        } else if (viewType == TYPE_TOP_SITES) {
            return new TopSitesViewHolder(mMvTilesContainerLayout);

        } else if (viewType == TYPE_NEW_CONTENT) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.brave_news_load_new_content, parent, false);
            return new NewContentViewHolder(view);

        } else if (viewType == TYPE_IMAGE_CREDIT) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.ntp_image_credit, parent, false);
            return new ImageCreditViewHolder(view);

        } else if (viewType == TYPE_NEWS_OPTIN) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.optin_layout, parent, false);
            return new NewsOptinViewHolder(view);

        } else if (viewType == TYPE_NEWS_LOADING) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.news_loading, parent, false);
            return new NewsLoadingViewHolder(view);

        } else if (viewType == TYPE_NEWS_NO_CONTENT_SOURCES) {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.brave_news_no_sources, parent, false);
            return new NoSourcesViewHolder(view);

        } else {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.brave_news_row, parent, false);
            return new NewsViewHolder(view);
        }
    }

    @Override
    public int getItemViewType(int position) {
        int statsCount = getStatsCount();
        int topSitesCount = getTopSitesCount();

        if (position == 0 && statsCount == 1) {
            return TYPE_STATS;
        } else if (topSitesCount == 1 && position == statsCount) {
            return TYPE_TOP_SITES;
        } else if (position == statsCount + topSitesCount && mIsNewContent) {
            return TYPE_NEW_CONTENT;
        } else if ((position == statsCount + topSitesCount && !mIsNewContent)
                || (position == statsCount + topSitesCount + ONE_ITEM_SPACE && mIsNewContent)) {
            return TYPE_IMAGE_CREDIT;
        } else if (position == statsCount + topSitesCount + ONE_ITEM_SPACE && mIsDisplayNewsOptin
                && !mIsNewContent) {
            return TYPE_NEWS_OPTIN;
        } else if (position == statsCount + topSitesCount + ONE_ITEM_SPACE
                && shouldDisplayNewsLoading() && !mIsNewContent) {
            return TYPE_NEWS_LOADING;
        } else if (!shouldDisplayNewsLoading() && mNewsItems.size() == 0) {
            return TYPE_NEWS_NO_CONTENT_SOURCES;
        } else {
            return TYPE_NEWS;
        }
    }

    public int getStatsCount() {
        return isStatsEnabled() ? 1 : 0;
    }

    // Will be used in privacy hub feature
    private boolean isStatsEnabled() {
        return mIsBraveStatsEnabled;
    }

    public int getTopSitesCount() {
        return mIsTopSitesEnabled ? 1 : 0;
    }

    public void setTopSitesEnabled(boolean isTopSitesEnabled) {
        if (mIsTopSitesEnabled != isTopSitesEnabled) {
            mIsTopSitesEnabled = isTopSitesEnabled;
            if (mIsTopSitesEnabled) {
                notifyItemInserted(getStatsCount());
            } else {
                notifyItemRemoved(getStatsCount());
            }
            notifyItemRangeChanged(getStatsCount(),
                    getStatsCount() + getTopSitesCount() + getNewContentCount() + ONE_ITEM_SPACE);
        }
    }

    public void setBraveStatsEnabled(boolean isBraveStatsEnabled) {
        if (mIsBraveStatsEnabled != isBraveStatsEnabled) {
            mIsBraveStatsEnabled = isBraveStatsEnabled;
            if (mIsBraveStatsEnabled) {
                notifyItemInserted(getStatsCount());
            } else {
                notifyItemRemoved(getStatsCount());
            }
        }
    }

    public void setDisplayNewsFeed(boolean isDisplayNewsFeed) {
        if (mIsDisplayNewsFeed != isDisplayNewsFeed) {
            mIsDisplayNewsFeed = isDisplayNewsFeed;
            if (mIsDisplayNewsFeed) {
                notifyItemRangeChanged(getStatsCount() + getTopSitesCount(), TWO_ITEMS_SPACE);
            } else {
                notifyItemRangeRemoved(
                        getStatsCount() + getTopSitesCount() + ONE_ITEM_SPACE, mNewsItems.size());
            }
        }
    }

    public void removeNewsOptin() {
        mIsDisplayNewsOptin = false;
        notifyItemRemoved(getStatsCount() + getTopSitesCount() + ONE_ITEM_SPACE);
    }

    public boolean shouldDisplayNewsLoading() {
        return mIsNewsLoading && mIsDisplayNewsFeed;
    }

    public int getTopMarginImageCredit() {
        return mTopMarginImageCredit;
    }

    public void setNewsLoading(boolean isNewsLoading) {
        mIsNewsLoading = isNewsLoading;
        if (isNewsLoading) {
            notifyItemInserted(getStatsCount() + getTopSitesCount() + ONE_ITEM_SPACE);
        } else {
            notifyItemRemoved(getStatsCount() + getTopSitesCount() + ONE_ITEM_SPACE);
        }
        notifyItemRangeChanged(getStatsCount() + getTopSitesCount(), TWO_ITEMS_SPACE);
    }

    public void setNewContent(boolean isNewContent) {
        if (mIsNewContent != isNewContent) {
            mIsNewContent = isNewContent;
            int newContentPosition = getStatsCount() + getTopSitesCount();
            if (!isNewContent) {
                mIsNewContentLoading = false;
                notifyItemRemoved(newContentPosition);
            } else {
                notifyItemInserted(newContentPosition);
            }

            notifyItemRangeChanged(newContentPosition, TWO_ITEMS_SPACE);
        }
    }

    public boolean isNewContent() {
        return mIsNewContent;
    }

    public void setNewContentLoading(boolean isNewContentLoading) {
        mIsNewContentLoading = isNewContentLoading;
        notifyItemChanged(getStatsCount() + getTopSitesCount());
    }

    public int getNewContentCount() {
        return mIsNewContent ? 1 : 0;
    }

    public void setSponsoredLogo(Wallpaper wallpaper, Bitmap sponsoredLogo) {
        mWallpaper = wallpaper;
        mSponsoredLogo = sponsoredLogo;
        notifyItemChanged(getStatsCount() + getTopSitesCount() + getNewContentCount());
    }

    public void setNtpImage(NTPImage ntpImage) {
        mNtpImage = ntpImage;
        notifyItemChanged(getStatsCount() + getTopSitesCount() + getNewContentCount());
    }

    public void setBraveNewsController(BraveNewsController braveNewsController) {
        mBraveNewsController = braveNewsController;
        notifyItemChanged(
                getStatsCount() + getTopSitesCount() + getNewContentCount() + ONE_ITEM_SPACE);
    }

    public void setImageCreditAlpha(float alpha) {
        if (mImageCreditAlpha == alpha) {
            return;
        }
        // We have to use PostTask otherwise it's possible to get IllegalStateException
        // during a call to notifyItemChanged when scrolling is in progress, see details
        // here https://github.com/brave/brave-browser/issues/29343
        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            mImageCreditAlpha = alpha;
            try {
                notifyItemChanged(getStatsCount() + getTopSitesCount() + getNewContentCount());
            } catch (IllegalStateException e) {
                Log.e(TAG, "setImageCreditAlpha: " + e.getMessage());
            }
        });
    }

    public void setRecyclerViewHeight(int recyclerViewHeight) {
        mRecyclerViewHeight = recyclerViewHeight;
        int count = getStatsCount() + getTopSitesCount() + getNewContentCount() + ONE_ITEM_SPACE;
        if (getItemCount() > count) {
            count += 1;
        }
        notifyItemRangeChanged(0, count);
    }

    public static class StatsViewHolder extends RecyclerView.ViewHolder {
        LinearLayout ntpStatsLayout;
        LinearLayout titleLayout;
        ImageView hideStatsImg;
        TextView adsBlockedCountTv;
        TextView adsBlockedCountTextTv;
        TextView dataSavedValueTv;
        TextView dataSavedValueTextTv;
        TextView estTimeSavedCountTv;
        TextView estTimeSavedCountTextTv;

        StatsViewHolder(View itemView) {
            super(itemView);
            this.ntpStatsLayout = (LinearLayout) itemView.findViewById(R.id.ntp_stats_layout);
            this.titleLayout = (LinearLayout) itemView.findViewById(R.id.brave_stats_title_layout);
            this.hideStatsImg = (ImageView) itemView.findViewById(R.id.widget_more_option);
            this.adsBlockedCountTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_text_ads_count);
            this.adsBlockedCountTextTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_text_ads_count_text);
            this.dataSavedValueTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_data_saved_value);
            this.dataSavedValueTextTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_data_saved_value_text);
            this.estTimeSavedCountTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_text_time_count);
            this.estTimeSavedCountTextTv =
                    (TextView) itemView.findViewById(R.id.brave_stats_text_time_count_text);
            BraveTouchUtils.ensureMinTouchTarget(this.hideStatsImg);
        }
    }

    public static class TopSitesViewHolder extends RecyclerView.ViewHolder {
        TopSitesViewHolder(View itemView) {
            super(itemView);
        }
    }

    public static class NewContentViewHolder extends RecyclerView.ViewHolder {
        LinearLayout newContentLayout;
        TextView newContentText;
        ProgressBar newContentProgressBar;

        NewContentViewHolder(View itemView) {
            super(itemView);
            this.newContentLayout = (LinearLayout) itemView.findViewById(R.id.new_content_layout);
            this.newContentProgressBar =
                    (ProgressBar) itemView.findViewById(R.id.new_content_loading_spinner);
            this.newContentText = (TextView) itemView.findViewById(R.id.new_content_button_text);
        }
    }

    public static class ImageCreditViewHolder extends RecyclerView.ViewHolder {
        LinearLayout ntpImageCreditLayout;
        FrameLayout imageCreditLayout;
        FloatingActionButton superReferralLogo;
        TextView creditTv;
        ImageView sponsoredLogo;

        ImageCreditViewHolder(View itemView) {
            super(itemView);
            this.ntpImageCreditLayout =
                    (LinearLayout) itemView.findViewById(R.id.ntp_image_credit_layout);
            this.imageCreditLayout = (FrameLayout) itemView.findViewById(R.id.image_credit_layout);
            this.superReferralLogo =
                    (FloatingActionButton) itemView.findViewById(R.id.super_referral_logo);
            this.creditTv = (TextView) itemView.findViewById(R.id.credit_text);
            this.sponsoredLogo = (ImageView) itemView.findViewById(R.id.sponsored_logo);
            BraveTouchUtils.ensureMinTouchTarget(this.creditTv);
        }
    }

    public static class NewsOptinViewHolder extends RecyclerView.ViewHolder {
        FrameLayout optinButton;
        ProgressBar optinLoadingSpinner;
        ImageView optinClose;
        TextView optinLearnMore;
        TextView optinTv;

        NewsOptinViewHolder(View itemView) {
            super(itemView);
            optinButton = (FrameLayout) itemView.findViewById(R.id.optin_button);
            optinClose = (ImageView) itemView.findViewById(R.id.close_optin);
            optinLearnMore = (TextView) itemView.findViewById(R.id.optin_learnmore);
            optinTv = (TextView) itemView.findViewById(R.id.optin_button_text);
            optinLoadingSpinner = (ProgressBar) itemView.findViewById(R.id.optin_loading_spinner);
            BraveTouchUtils.ensureMinTouchTarget(optinButton);
            BraveTouchUtils.ensureMinTouchTarget(optinLearnMore);
        }
    }

    public static class NewsLoadingViewHolder extends RecyclerView.ViewHolder {
        LinearLayout linearLayout;

        NewsLoadingViewHolder(View itemView) {
            super(itemView);
            this.linearLayout = (LinearLayout) itemView.findViewById(R.id.card_layout);
        }
    }

    public static class NewsViewHolder extends RecyclerView.ViewHolder {
        LinearLayout linearLayout;

        NewsViewHolder(View itemView) {
            super(itemView);
            this.linearLayout = (LinearLayout) itemView.findViewById(R.id.card_layout);
        }
    }

    public static class NoSourcesViewHolder extends RecyclerView.ViewHolder {
        Button btnChooseContent;

        NoSourcesViewHolder(View itemView) {
            super(itemView);
            this.btnChooseContent = (Button) itemView.findViewById(R.id.btn_choose_content);
        }
    }
}
