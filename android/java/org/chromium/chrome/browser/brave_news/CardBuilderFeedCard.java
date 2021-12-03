/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import android.app.Activity;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.core.content.ContextCompat;
import androidx.core.widget.NestedScrollView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.Priority;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.load.resource.bitmap.CenterCrop;
import com.bumptech.glide.load.resource.bitmap.GranularRoundedCorners;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.RequestOptions;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.target.Target;
import com.bumptech.glide.request.transition.Transition;

import org.chromium.base.Log;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.Deal;
import org.chromium.brave_news.mojom.DisplayAd;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.Image;
import org.chromium.brave_news.mojom.PromotedArticle;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.brave_news.BraveNewsBottomSheetDialogFragment;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.url.mojom.Url;

import java.util.List;

public class CardBuilderFeedCard {
    private final int CARD_LAYOUT = 7;
    private final int BUTTON_LAYOUT = 8;
    private final int ROUNDED_TOP_LAYOUT = 9;

    private final int TITLE = 10;
    private final int DESC = 11;
    private final int URL = 12;
    private final int PUBLISHER = 13;
    private final int TIME = 14;
    private final int CATEGORY = 15;
    private final int DEALS = 16;

    private LinearLayout mLinearLayout;
    private Activity mActivity;
    private FeedItemsCard mNewsItem;
    private BraveNewsController mBraveNewsController;
    private int mType;
    private int mPosition;
    private View view;
    private int mHorizontalMargin;
    private int mDeviceWidth;
    private boolean mIsPromo;
    private String mCreativeInstanceId;
    private String mOffersCategory;

    private final String TAG = "BN";
    private final int MARGIN_VERTICAL = 10;
    private final String BRAVE_OFFERS_URL = "offers.brave.com";

    public CardBuilderFeedCard(BraveNewsController braveNewsController, LinearLayout layout,
            Activity activity, int position, FeedItemsCard newsItem, int type) {
        mLinearLayout = layout;
        mActivity = activity;
        mPosition = position;
        mType = type;
        mNewsItem = newsItem;
        mBraveNewsController = braveNewsController;

        mDeviceWidth = ConfigurationUtils.getDisplayMetrics(activity).get("width");

        mIsPromo = false;
        mCreativeInstanceId = "";
        mOffersCategory = "";

        boolean isTablet = ConfigurationUtils.isTablet(activity);
        boolean isLandscape = ConfigurationUtils.isLandscape(activity);

        mHorizontalMargin = isTablet
                ? isLandscape ? (int) (0.20 * mDeviceWidth) : (int) (0.10 * mDeviceWidth)
                : 40;
        try {
            createCard(mType, mPosition);
        } catch (Exception e) {
            Log.e(TAG, "Exception createCard:" + e.getMessage());
        }
    }

    public void initItems() {}

    public void removeCard(LinearLayout layout) {
        layout.removeAllViews();
        layout.setVisibility(View.GONE);
        layout.invalidate();
    }

    private Url getImage(FeedItemMetadata itemMetaData) {
        switch (itemMetaData.image.which()) {
            case Image.Tag.PaddedImageUrl:
                return itemMetaData.image.getPaddedImageUrl();
            case Image.Tag.ImageUrl:
                return itemMetaData.image.getImageUrl();
            default:
                return null;
        }
    }

    private GradientDrawable roundedBackground() {
        GradientDrawable shape = new GradientDrawable();
        shape.setCornerRadius(15);
        shape.setColor(mActivity.getResources().getColor(R.color.card_background));

        return shape;
    }

    public LinearLayout createCard(int type, int position) {
        TableLayout tableLayoutTopNews = new TableLayout(mActivity);

        TableLayout.LayoutParams tableParamsTopNews = new TableLayout.LayoutParams(
                TableLayout.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        TableLayout.LayoutParams tableParamsRow1 = new TableLayout.LayoutParams(
                TableLayout.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        TableLayout.LayoutParams rowTableParams = new TableLayout.LayoutParams(
                TableLayout.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        int displayHeight = mActivity.getResources().getDisplayMetrics().heightPixels;
        TableRow rowTop = new TableRow(mActivity);
        TextView topText = new TextView(mActivity);

        TableRow row1 = new TableRow(mActivity);
        TableRow row2 = new TableRow(mActivity);
        TableRow row3 = new TableRow(mActivity);

        LinearLayout layout1 = new LinearLayout(mActivity);
        LinearLayout layout2 = new LinearLayout(mActivity);
        LinearLayout layout3 = new LinearLayout(mActivity);

        LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        linearLayoutParams.setMargins(mHorizontalMargin, 0, mHorizontalMargin, 7 * MARGIN_VERTICAL);

        try {
            switch (type) {
                case CardType.HEADLINE:
                case CardType.PROMOTED_ARTICLE:
                    addElementsToSingleLayout(mLinearLayout, 0, type);
                    mLinearLayout.setBackground(
                            makeRound(CARD_LAYOUT, R.color.card_background, 30));
                    break;
                case CardType.DISPLAY_AD:
                    FrameLayout adLayoutUp = new FrameLayout(mActivity);
                    ImageView adImage = new ImageView(mActivity);
                    TextView adLogo = new TextView(mActivity);

                    LinearLayout adLayoutDown = new LinearLayout(mActivity);
                    TextView adTitle = new TextView(mActivity);
                    TextView adDesc = new TextView(mActivity);
                    Button adButton = new Button(mActivity);

                    TableRow.LayoutParams adLayoutParams = new TableRow.LayoutParams(
                            TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
                    TableRow.LayoutParams adLayoutDownParams = new TableRow.LayoutParams(
                            TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);

                    FrameLayout.LayoutParams adImageParams =
                            new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT,
                                    FrameLayout.LayoutParams.WRAP_CONTENT);

                    TableRow.LayoutParams adButtonParams = new TableRow.LayoutParams(
                            TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);

                    tableLayoutTopNews.setLayoutParams(
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT));
                    mLinearLayout.addView(tableLayoutTopNews);

                    linearLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    linearLayoutParams.setMargins(
                            mHorizontalMargin, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    mLinearLayout.setLayoutParams(linearLayoutParams);

                    mLinearLayout.setBackground(roundedBackground());

                    try {
                        mBraveNewsController.getDisplayAd(adData -> {
                            if (adData != null) {
                                NTPUtil.setsCurrentDisplayAd(adData);

                                FrameLayout.LayoutParams adLogoParams =
                                        new FrameLayout.LayoutParams(
                                                FrameLayout.LayoutParams.MATCH_PARENT,
                                                FrameLayout.LayoutParams.WRAP_CONTENT);
                                TableRow.LayoutParams adItemsParams = new TableRow.LayoutParams(
                                        TableRow.LayoutParams.MATCH_PARENT,
                                        TableRow.LayoutParams.WRAP_CONTENT);

                                rowTableParams.setMargins(50, 0, 50, 0);
                                rowTableParams.width = TableLayout.LayoutParams.MATCH_PARENT;
                                rowTableParams.height = TableLayout.LayoutParams.WRAP_CONTENT;
                                rowTableParams.gravity = Gravity.CENTER_HORIZONTAL;
                                rowTop.setGravity(Gravity.CENTER_HORIZONTAL);
                                rowTop.setLayoutParams(rowTableParams);
                                tableLayoutTopNews.addView(rowTop);

                                adLayoutParams.width = TableRow.LayoutParams.MATCH_PARENT;
                                adLayoutParams.height = TableRow.LayoutParams.WRAP_CONTENT;
                                adLayoutParams.bottomMargin = 2 * MARGIN_VERTICAL;
                                adLayoutUp.setLayoutParams(adLayoutParams);
                                rowTop.addView(adLayoutUp);

                                adImageParams.width = FrameLayout.LayoutParams.MATCH_PARENT;
                                adImageParams.height = FrameLayout.LayoutParams.MATCH_PARENT;

                                LinearLayout.LayoutParams adImageLinearParams =
                                        (LinearLayout.LayoutParams) adLayoutUp.getLayoutParams();
                                adImageLinearParams.weight = 1.0f;
                                adImage.setLayoutParams(adImageLinearParams);

                                setDisplayAdImage(adImage, adData.image, 1);
                                adLayoutUp.addView(adImage);
                                adLayoutUp.addView(adLogo);

                                adLogoParams.width = FrameLayout.LayoutParams.WRAP_CONTENT;
                                adLogoParams.height = FrameLayout.LayoutParams.WRAP_CONTENT;
                                adLogoParams.topMargin = 30;
                                adLogoParams.rightMargin = 0;
                                adLogoParams.gravity = Gravity.END;
                                adLogo.setGravity(Gravity.END);
                                adLogo.setCompoundDrawablesWithIntrinsicBounds(
                                        R.drawable.ic_rewards, 0, 0, 0);
                                adLogo.setText(R.string.brave_news_ad);
                                adLogo.setTextColor(mActivity.getResources().getColor(
                                        R.color.brave_theme_color));
                                GradientDrawable gd = new GradientDrawable();
                                gd.setColor(
                                        mActivity.getResources().getColor(R.color.news_text_color));
                                gd.setCornerRadius(15);
                                gd.setStroke(1,
                                        mActivity.getResources().getColor(
                                                R.color.brave_theme_color));
                                adLogo.setBackground(gd);
                                adLogo.setPadding(5, 5, 10, 5);
                                adLogo.setLayoutParams(adLogoParams);
                                adLogo.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        openUrlInSameTabAndSavePosition(
                                                BraveActivity.BRAVE_REWARDS_SETTINGS_URL);
                                    }
                                });

                                rowTableParams.setMargins(50, 0, 50, 0);
                                rowTableParams.width = TableLayout.LayoutParams.MATCH_PARENT;
                                rowTableParams.height = TableLayout.LayoutParams.WRAP_CONTENT;
                                row1.setLayoutParams(rowTableParams);
                                rowTableParams.bottomMargin = 3 * MARGIN_VERTICAL;
                                row2.setLayoutParams(rowTableParams);
                                tableLayoutTopNews.addView(row1);
                                tableLayoutTopNews.addView(row2);

                                row1.addView(adTitle);
                                row2.addView(adDesc);
                                row2.addView(adButton);

                                adItemsParams = new TableRow.LayoutParams(
                                        TableRow.LayoutParams.MATCH_PARENT,
                                        TableRow.LayoutParams.WRAP_CONTENT, 1.0f);
                                adTitle.setTextSize(17);
                                adItemsParams.bottomMargin = 2 * MARGIN_VERTICAL;
                                adTitle.setTypeface(null, Typeface.BOLD);
                                adTitle.setMaxLines(3);
                                adTitle.setTextColor(
                                        mActivity.getResources().getColor(R.color.news_text_color));
                                adTitle.setEllipsize(TextUtils.TruncateAt.END);
                                adTitle.setLayoutParams(adItemsParams);
                                adTitle.setText(adData.title);

                                adItemsParams = new TableRow.LayoutParams(
                                        TableRow.LayoutParams.WRAP_CONTENT,
                                        TableRow.LayoutParams.MATCH_PARENT);
                                adItemsParams.gravity = Gravity.CENTER_VERTICAL;
                                adDesc.setGravity(Gravity.CENTER_VERTICAL);
                                adItemsParams.weight = 1;
                                adItemsParams.width = 0;
                                adDesc.setLayoutParams(adItemsParams);

                                adDesc.setTextColor(
                                        mActivity.getResources().getColor(R.color.news_time_color));
                                adDesc.setTextSize(12);
                                adDesc.setText(adData.description);

                                adButtonParams.width = TableRow.LayoutParams.WRAP_CONTENT;
                                adButtonParams.height = 80; // TableRow.LayoutParams.WRAP_CONTENT;
                                adButton.setPadding(30, 0, 30, 0);
                                adButton.setTextSize(13);
                                adButton.setAllCaps(false);
                                GradientDrawable adButtonBG = new GradientDrawable();
                                adButtonBG.setColor(mActivity.getResources().getColor(
                                        android.R.color.transparent));
                                adButtonBG.setCornerRadius(55);
                                adButtonBG.setStroke(1,
                                        mActivity.getResources().getColor(R.color.news_text_color));
                                adButton.setBackground(adButtonBG);
                                adButton.setText(adData.ctaText);
                                adButton.setTextColor(
                                        mActivity.getResources().getColor(R.color.news_text_color));
                                adButton.setLayoutParams(adButtonParams);
                                adButton.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mBraveNewsController.onDisplayAdVisit(
                                                adData.uuid, adData.creativeInstanceId);
                                        openUrlInSameTabAndSavePosition(adData.targetUrl.url);
                                    }
                                });
                                adImage.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mBraveNewsController.onDisplayAdVisit(
                                                adData.uuid, adData.creativeInstanceId);
                                        openUrlInSameTabAndSavePosition(adData.targetUrl.url);
                                    }
                                });
                                adTitle.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mBraveNewsController.onDisplayAdVisit(
                                                adData.uuid, adData.creativeInstanceId);
                                        openUrlInSameTabAndSavePosition(adData.targetUrl.url);
                                    }
                                });
                                adDesc.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mBraveNewsController.onDisplayAdVisit(
                                                adData.uuid, adData.creativeInstanceId);
                                        openUrlInSameTabAndSavePosition(adData.targetUrl.url);
                                    }
                                });
                            }
                        });

                    } catch (Exception e) {
                        Log.e(TAG, "displayad Exception" + e.getMessage());
                    }

                    break;
                case CardType.DEALS:

                    View lineSeparator = new View(mActivity);
                    LinearLayout moreOffersLayout = new LinearLayout(mActivity);
                    TextView moreOffersText = new TextView(mActivity);
                    ImageView moreOffersArrow = new ImageView(mActivity);

                    LinearLayout.LayoutParams lineSeparatorParams = new LinearLayout.LayoutParams(
                            LinearLayout.LayoutParams.MATCH_PARENT, 2);
                    LinearLayout.LayoutParams moreOffersLayoutParams =
                            new LinearLayout.LayoutParams(
                                    LinearLayout.LayoutParams.MATCH_PARENT, 120);
                    LinearLayout.LayoutParams moreOffersTextParams = new LinearLayout.LayoutParams(
                            0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
                    LinearLayout.LayoutParams moreOffersArrowParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);

                    mLinearLayout.setOrientation(LinearLayout.VERTICAL);

                    linearLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    linearLayoutParams.setMargins(
                            mHorizontalMargin, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    mLinearLayout.setLayoutParams(linearLayoutParams);

                    tableLayoutTopNews.setLayoutParams(
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT));

                    tableParamsTopNews.setMargins(50, 5 * MARGIN_VERTICAL, 50, 5 * MARGIN_VERTICAL);
                    tableParamsTopNews.weight = 1;
                    tableParamsTopNews.height = TableLayout.LayoutParams.WRAP_CONTENT;

                    row1.setPadding(50, 0, 20, 0);

                    mLinearLayout.addView(tableLayoutTopNews);
                    tableLayoutTopNews.addView(rowTop);
                    rowTop.addView(topText);

                    // adds the More Offers bottom layout
                    lineSeparator.setBackgroundColor(
                            mActivity.getResources().getColor(R.color.news_time_color));
                    lineSeparator.setLayoutParams(lineSeparatorParams);
                    moreOffersLayout.setOrientation(LinearLayout.HORIZONTAL);
                    moreOffersLayoutParams.setMargins(50, 0, 50, 0);
                    moreOffersLayout.setPadding(50, 0, 50, 0);
                    moreOffersLayout.setLayoutParams(moreOffersLayoutParams);

                    moreOffersTextParams.gravity = Gravity.CENTER_VERTICAL;
                    moreOffersText.setText(mActivity.getResources().getString(
                            R.string.brave_news_more_offers_title));
                    moreOffersText.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 17 : 13);
                    moreOffersText.setTypeface(null, Typeface.BOLD);
                    moreOffersText.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    moreOffersText.setLayoutParams(moreOffersTextParams);

                    moreOffersArrow.setImageResource(R.drawable.ic_chevron_right);
                    moreOffersArrow.setColorFilter(
                            ContextCompat.getColor(mActivity, R.color.news_text_color));
                    moreOffersArrowParams.gravity = Gravity.CENTER_VERTICAL;
                    moreOffersArrow.setLayoutParams(moreOffersArrowParams);

                    moreOffersLayout.addView(moreOffersText);
                    moreOffersLayout.addView(moreOffersArrow);
                    moreOffersLayout.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            openUrlInSameTabAndSavePosition(BRAVE_OFFERS_URL);
                        }
                    });

                    // adds the items to the view layout
                    tableLayoutTopNews.addView(row1);
                    tableLayoutTopNews.addView(lineSeparator);
                    tableLayoutTopNews.addView(moreOffersLayout);

                    row1.addView(layout1);
                    row1.addView(layout2);
                    row1.addView(layout3);

                    rowTop.setLayoutParams(tableParamsTopNews);
                    setTextFromFeed(topText, DEALS, 0);
                    topText.setTextSize(17);
                    topText.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    topText.setTypeface(null, Typeface.BOLD);

                    layout1.setOrientation(LinearLayout.VERTICAL);
                    layout2.setOrientation(LinearLayout.VERTICAL);
                    layout3.setOrientation(LinearLayout.VERTICAL);

                    // adds each of the 3 cards
                    addElementsToSingleLayout(layout1, 0, type);
                    addElementsToSingleLayout(layout2, 1, type);
                    addElementsToSingleLayout(layout3, 2, type);

                    mLinearLayout.setBackground(
                            makeRound(CARD_LAYOUT, R.color.card_background, 30));

                    break;
                case CardType.PUBLISHER_GROUP: // THREE_ROWS_HEADLINES:

                    mLinearLayout.setOrientation(LinearLayout.VERTICAL);
                    tableLayoutTopNews.setLayoutParams(
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.MATCH_PARENT));
                    tableParamsTopNews.setMargins(30, 40, 30, 40);
                    linearLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    linearLayoutParams.setMargins(
                            mHorizontalMargin, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    mLinearLayout.setLayoutParams(linearLayoutParams);

                    mLinearLayout.addView(tableLayoutTopNews);
                    tableLayoutTopNews.addView(rowTop);
                    rowTop.addView(topText);

                    tableLayoutTopNews.addView(row1);
                    tableLayoutTopNews.addView(row2);
                    tableLayoutTopNews.addView(row3);
                    rowTop.setPadding(0, 0, 30, 0);

                    rowTop.setLayoutParams(tableParamsTopNews);

                    setTextFromFeed(topText, PUBLISHER, 0);
                    topText.setTextSize(20);
                    topText.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    topText.setTypeface(null, Typeface.BOLD);

                    addElementsToSingleLayout(row1, 0, type);

                    row2.setPadding(5, 5, 5, 5);
                    addElementsToSingleLayout(row2, 1, type);

                    TableRow.LayoutParams row3Params = new TableRow.LayoutParams(
                            TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
                    row3Params.bottomMargin = 5 * MARGIN_VERTICAL;
                    row3.setPadding(5, 5, 5, 5);
                    addElementsToSingleLayout(row3, 2, type);

                    mLinearLayout.setPadding(30, 10, 40, 10);
                    mLinearLayout.setBackground(
                            makeRound(CARD_LAYOUT, R.color.card_background, 30));
                    break;
                case CardType.CATEGORY_GROUP: // TOP_NEWS:
                    /*3 rows

                                   TOP NEWS

                                   Title           ---------
                                   Description     |       |
                                                   |       |
                                                   ---------

                                   Title           ---------
                                   Description     |       |
                                                   |       |
                                                   --------


                                   Title           ---------
                                   Description     |       |
                                                   |       |
                                                   --------


                                    */

                    mLinearLayout.setOrientation(LinearLayout.VERTICAL);
                    linearLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    float dpHeight =
                            ConfigurationUtils.getDpDisplayMetrics(mActivity).get("height");

                    LinearLayout.LayoutParams params =
                            new TableLayout.LayoutParams(TableLayout.LayoutParams.MATCH_PARENT,
                                    TableLayout.LayoutParams.WRAP_CONTENT);

                    params.setMargins(mHorizontalMargin, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    int height = mActivity.getResources().getDisplayMetrics().heightPixels;
                    mLinearLayout.setLayoutParams(params);

                    mLinearLayout.addView(tableLayoutTopNews);
                    tableLayoutTopNews.addView(rowTop);
                    rowTop.addView(topText);

                    tableLayoutTopNews.addView(row1);
                    tableLayoutTopNews.addView(row2);
                    tableLayoutTopNews.addView(row3);

                    mLinearLayout.setPadding(30, 30, 30, 10);
                    tableParamsTopNews.setMargins(30, 2 * MARGIN_VERTICAL, 30, 4 * MARGIN_VERTICAL);
                    tableParamsTopNews.weight = 3;
                    rowTop.setLayoutParams(tableParamsTopNews);

                    setTextFromFeed(topText, CATEGORY, 0);
                    topText.setTextSize(26);
                    topText.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    topText.setTypeface(null, Typeface.BOLD);

                    rowTableParams.bottomMargin = 2 * MARGIN_VERTICAL;
                    row1.setLayoutParams(rowTableParams);
                    row2.setLayoutParams(rowTableParams);
                    rowTableParams.bottomMargin = 4 * MARGIN_VERTICAL;
                    row3.setLayoutParams(rowTableParams);

                    addElementsToSingleLayout(row1, 0, type);
                    addElementsToSingleLayout(row2, 1, type);
                    addElementsToSingleLayout(row3, 2, type);
                    mLinearLayout.setBackground(
                            makeRound(CARD_LAYOUT, R.color.card_background, 30));

                    break;
                case CardType.HEADLINE_PAIRED: // HEADLINEPAIR:
                    /*headlinepair

                                 Image      Image
                               ---------    ---------
                               |       |    |       |
                               |       |    |       |
                               ---------    ---------
                                 Title        Title
                              Description    Description

                            */

                    mLinearLayout.setOrientation(LinearLayout.HORIZONTAL);

                    linearLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    linearLayoutParams.setMargins(
                            mHorizontalMargin - 20, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    mLinearLayout.setLayoutParams(linearLayoutParams);

                    LinearLayout.LayoutParams cellParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
                    cellParams.setMargins(0, 0, 20, 0);

                    cellParams.height = LinearLayout.LayoutParams.MATCH_PARENT;
                    LinearLayout layoutLeft = new LinearLayout(mActivity);
                    LinearLayout layoutRight = new LinearLayout(mActivity);
                    mLinearLayout.addView(layoutLeft);
                    layoutLeft.setLayoutParams(cellParams);
                    layoutLeft.setOrientation(LinearLayout.VERTICAL);

                    addElementsToSingleLayout(layoutLeft, 0, type);

                    mLinearLayout.addView(layoutRight);
                    cellParams.setMargins(20, 0, 0, 0);

                    cellParams.height = LinearLayout.LayoutParams.MATCH_PARENT;
                    layoutRight.setLayoutParams(cellParams);
                    layoutRight.setOrientation(LinearLayout.VERTICAL);

                    addElementsToSingleLayout(layoutRight, 1, type);

                    layoutLeft.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
                    layoutRight.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);

                    int maxHeight = Math.max(
                            layoutLeft.getMeasuredHeight(), layoutRight.getMeasuredHeight());

                    if (maxHeight > layoutLeft.getMeasuredHeight()) {
                        cellParams.height = maxHeight;
                        layoutLeft.setLayoutParams(cellParams);
                    } else if (maxHeight > layoutRight.getMeasuredHeight()) {
                        cellParams.height = maxHeight;
                        layoutRight.setLayoutParams(cellParams);
                    }

                    break;
            }
        } catch (Exception e) {
            Log.e(TAG, "cardfeedbuilder crashinvestigation exception:" + e.getMessage());
        }

        return mLinearLayout;
    }

    private void openUrlInSameTabAndSavePosition(String myUrl) {
        SharedPreferencesManager.getInstance().writeInt(
                Integer.toString(BraveActivity.getBraveActivity().getActivityTab().getId()),
                mPosition);
        TabUtils.openUrlInSameTab(myUrl);
    }

    private void addElementsToSingleLayout(ViewGroup view, int index, int itemType) {
        ImageView image = new ImageView(mActivity);
        ImageView logo = new ImageView(mActivity);
        TextView title = new TextView(mActivity);
        TextView publisher = new TextView(mActivity);
        TextView source = new TextView(mActivity);
        TextView desc = new TextView(mActivity);

        LinearLayout.LayoutParams linearLayoutParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
        LinearLayout.LayoutParams publisherParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        LinearLayout.LayoutParams imageParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
        LinearLayout.LayoutParams sourceParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
        LinearLayout.LayoutParams descParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
        LinearLayout.LayoutParams logoParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT, 1f);

        TableLayout.LayoutParams tableParamsTopNews = new TableLayout.LayoutParams(
                TableLayout.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

        TableRow.LayoutParams linearLayoutRowParams = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
        TableRow.LayoutParams linearLayoutRowParams1 = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
        TableRow.LayoutParams sourceRowParams = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
        TableRow.LayoutParams titleRowParams = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
        TableRow.LayoutParams descRowParams = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);
        TableRow.LayoutParams imageRowParams = new TableRow.LayoutParams(
                TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.MATCH_PARENT);

        LinearLayout layoutSingleCard = new LinearLayout(mActivity);
        publisher.setLetterSpacing(0.03f);
        title.setLetterSpacing(0.02f);

        try {
            LinearLayout layout = (LinearLayout) view;
            switch (itemType) {
                case CardType.DEALS:
                    LinearLayout layoutDeals = (LinearLayout) view;
                    layoutDeals.setOrientation(LinearLayout.VERTICAL);
                    linearLayoutRowParams1.width = 0;
                    linearLayoutRowParams1.weight = 1;

                    linearLayoutRowParams1.setMargins(0, 0, 30, 0);
                    layoutDeals.setLayoutParams(linearLayoutRowParams1);

                    setImage(image, "paired", index);
                    imageParams.height = ConfigurationUtils.isTablet(mActivity)
                            ? (int) (mDeviceWidth * 0.20)
                            : 230;

                    image.setScaleType(ImageView.ScaleType.CENTER_CROP);
                    image.setLayoutParams(imageParams);
                    layoutDeals.addView(image);

                    titleParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    title.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 18 : 14);
                    title.setTypeface(null, Typeface.BOLD);
                    title.setPadding(0, 10, 0, 0);
                    title.setLayoutParams(titleParams);
                    title.setMaxLines(2);
                    title.setEllipsize(TextUtils.TruncateAt.END);
                    layoutDeals.addView(title);

                    descParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    setTextFromFeed(desc, DESC, index);
                    desc.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 16 : 12);
                    desc.setMaxLines(3);
                    desc.setEllipsize(TextUtils.TruncateAt.END);

                    desc.setLayoutParams(descParams);

                    descParams.bottomMargin = MARGIN_VERTICAL;
                    desc.setPadding(0, 0, 30, 30);
                    layoutDeals.addView(desc);

                    break;
                case CardType.HEADLINE:
                    layout.setOrientation(LinearLayout.VERTICAL);
                    int topPosition = 0;
                    if (mPosition == 0) {
                        topPosition = ConfigurationUtils.isTablet(mActivity) ? 120 : 200;
                    }
                    linearLayoutParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    linearLayoutParams.setMargins(
                            mHorizontalMargin, topPosition, mHorizontalMargin, 5 * MARGIN_VERTICAL);

                    layout.setBackground(makeRound(CARD_LAYOUT, R.color.card_background, 30));
                    layout.setLayoutParams(linearLayoutParams);

                    layout.addView(image);
                    layout.addView(title);
                    layout.addView(desc);
                    layout.addView(publisher);

                    imageParams.bottomMargin = 2 * MARGIN_VERTICAL;
                    imageParams.height = ConfigurationUtils.isTablet(mActivity)
                            ? (int) (mDeviceWidth * 0.45)
                            : (int) (mDeviceWidth * 0.6);
                    image.setScaleType(ImageView.ScaleType.CENTER_CROP);
                    image.setLayoutParams(imageParams);

                    setImage(image, "image", index);

                    titleParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    title.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 21 : 17);
                    title.setTypeface(null, Typeface.BOLD);
                    title.setMaxLines(5);
                    title.setEllipsize(TextUtils.TruncateAt.END);
                    title.setPadding(50, 0, 50, 0);
                    titleParams.topMargin = 2 * MARGIN_VERTICAL;
                    titleParams.bottomMargin = 1 * MARGIN_VERTICAL;
                    title.setLayoutParams(titleParams);

                    descParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    setTextFromFeed(desc, TIME, index);
                    desc.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 15 : 11);
                    desc.setLayoutParams(descParams);
                    desc.setPadding(50, 0, 50, 30);

                    setTextFromFeed(publisher, PUBLISHER, 0);

                    publisher.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    publisher.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 16 : 12);
                    publisherParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    publisherParams.bottomMargin = 10;
                    publisher.setPadding(50, 0, 50, 0);

                    publisherParams.bottomMargin = 4 * MARGIN_VERTICAL;
                    ;
                    publisher.setLayoutParams(publisherParams);

                    break;
                case CardType.PROMOTED_ARTICLE:
                    layout.setOrientation(LinearLayout.VERTICAL);
                    linearLayoutParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    linearLayoutParams.setMargins(
                            mHorizontalMargin, 0, mHorizontalMargin, 5 * MARGIN_VERTICAL);
                    layout.setLayoutParams(linearLayoutParams);

                    TextView promoted = new TextView(mActivity);
                    LinearLayout promotedLogoLayout = new LinearLayout(mActivity);

                    imageParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    titleParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    descParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    publisherParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);

                    layout.addView(image);
                    layout.addView(title);
                    layout.addView(desc);
                    layout.addView(promotedLogoLayout);

                    imageParams.height = (int) (mDeviceWidth * 0.6);

                    imageParams.bottomMargin = 2 * MARGIN_VERTICAL;
                    image.setLayoutParams(imageParams);

                    setImage(image, "image", index);
                    image.setScaleType(ImageView.ScaleType.CENTER_CROP);

                    titleParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    titleParams.rightMargin = 20;
                    titleParams.bottomMargin = 2 * MARGIN_VERTICAL;
                    title.setTextSize(17);
                    title.setTypeface(null, Typeface.BOLD);
                    title.setMaxLines(5);
                    titleParams.bottomMargin = MARGIN_VERTICAL;
                    title.setEllipsize(TextUtils.TruncateAt.END);
                    title.setPadding(50, 30, 50, 0);
                    title.setLayoutParams(titleParams);

                    descParams.weight = LinearLayout.LayoutParams.WRAP_CONTENT;

                    setTextFromFeed(desc, TIME, index);
                    desc.setTextSize(11);
                    desc.setLayoutParams(descParams);
                    desc.setPadding(50, 0, 50, 30);

                    LinearLayout.LayoutParams promotedParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                                    LinearLayout.LayoutParams.MATCH_PARENT);
                    publisherParams = new LinearLayout.LayoutParams(
                            0, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
                    LinearLayout.LayoutParams promotedLayoutParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    promotedLayoutParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    promotedLogoLayout.setPadding(50, 0, 50, 0);

                    promotedParams.width = 0;
                    promotedParams.weight = 1;

                    promotedLogoLayout.setOrientation(LinearLayout.HORIZONTAL);
                    promoted.setPadding(20, 0, 0, 0);
                    promoted.setGravity(Gravity.CENTER);
                    promotedParams.gravity = Gravity.CENTER_VERTICAL | Gravity.END;
                    promotedLayoutParams.gravity = Gravity.CENTER;
                    publisher.setGravity(Gravity.CENTER_VERTICAL);
                    publisherParams.gravity = Gravity.CENTER_VERTICAL;

                    promoted.setCompoundDrawablesWithIntrinsicBounds(
                            R.drawable.ic_promoted, 0, 0, 0);
                    StringBuilder promotedTitle = new StringBuilder(" "
                            + mActivity.getResources().getString(
                                    R.string.brave_news_promoted_title));
                    promoted.setText(
                            mActivity.getResources().getString(R.string.brave_news_promoted_title));

                    promoted.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    promoted.setTextSize(12);
                    promoted.setLayoutParams(promotedParams);
                    promotedLayoutParams.bottomMargin = 4 * MARGIN_VERTICAL;

                    promoted.setBackground(
                            makeRound(BUTTON_LAYOUT, R.color.news_promoted_background_color, 15));
                    promotedLogoLayout.setLayoutParams(promotedLayoutParams);

                    setTextFromFeed(publisher, PUBLISHER, 0);
                    publisher.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    publisher.setTextSize(12);
                    publisherParams.width = 0;
                    publisherParams.weight = 2;

                    publisher.setLayoutParams(publisherParams);
                    promotedLogoLayout.addView(publisher);
                    promotedLogoLayout.addView(promoted);

                    break;
                case CardType.HEADLINE_PAIRED:

                    layout.setOrientation(LinearLayout.VERTICAL);

                    imageParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    titleParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    descParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);
                    publisherParams =
                            new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                    LinearLayout.LayoutParams.WRAP_CONTENT);

                    int height = ConfigurationUtils.isTablet(mActivity)
                            ? (int) (mDeviceWidth * 0.25)
                            : (int) (mDeviceWidth * 0.3);

                    imageParams.height = height;
                    image.setLayoutParams(imageParams);
                    setImage(image, "image", index);

                    image.setScaleType(ImageView.ScaleType.CENTER_CROP);
                    imageParams.bottomMargin = 2 * MARGIN_VERTICAL;
                    layout.addView(image);

                    title.setLayoutParams(titleParams);
                    title.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 17 : 13);
                    title.setTypeface(null, Typeface.BOLD);
                    title.setMaxLines(5);
                    titleParams.bottomMargin = MARGIN_VERTICAL;
                    title.setEllipsize(TextUtils.TruncateAt.END);
                    title.setPadding(50, 0, 50, 0);
                    layout.addView(title);

                    descParams.weight = 1;
                    descParams.height = 0;
                    desc.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 15 : 11);
                    setTextFromFeed(desc, TIME, index);
                    desc.setLayoutParams(descParams);
                    desc.setPadding(50, 0, 50, 0);
                    descParams.bottomMargin = MARGIN_VERTICAL;
                    layout.addView(desc);

                    setTextFromFeed(publisher, PUBLISHER, index);
                    publisher.setTextColor(
                            mActivity.getResources().getColor(R.color.news_text_color));
                    publisher.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 16 : 12);

                    publisherParams.gravity = Gravity.BOTTOM;
                    publisher.setGravity(Gravity.BOTTOM);
                    publisherParams.setMargins(0, 0, 0, 4 * MARGIN_VERTICAL);
                    publisher.setPadding(50, 0, 50, 0);
                    publisher.setLayoutParams(publisherParams);
                    layout.addView(publisher);

                    layout.setBackground(makeRound(CARD_LAYOUT, R.color.card_background, 30));

                    break;

                case CardType.PUBLISHER_GROUP: // 3 numbered rows no photos

                    TableLayout.LayoutParams layoutRowParams =
                            new TableLayout.LayoutParams(TableLayout.LayoutParams.MATCH_PARENT,
                                    ViewGroup.LayoutParams.MATCH_PARENT);
                    TableRow layoutRow = (TableRow) view;
                    TextView no = new TextView(mActivity);

                    layoutRow.removeAllViews();
                    layoutSingleCard.removeAllViews();

                    layoutRowParams.setMargins(0, 20, 0, 20);
                    layoutRowParams.weight = 1;
                    layoutRowParams.height = 0;
                    layoutRow.setPadding(30, 0, 30, 0);
                    layoutRow.setLayoutParams(layoutRowParams);

                    TableRow.LayoutParams noParams = new TableRow.LayoutParams(
                            TableRow.LayoutParams.WRAP_CONTENT, TableRow.LayoutParams.WRAP_CONTENT);
                    TableRow.LayoutParams layoutSingleCardParams = new TableRow.LayoutParams(
                            TableRow.LayoutParams.WRAP_CONTENT, TableRow.LayoutParams.WRAP_CONTENT);

                    no.setText(String.valueOf(index + 1));
                    no.setTextColor(mActivity.getResources().getColor(R.color.news_text_color));
                    noParams.width = 0;
                    noParams.weight = 1;
                    noParams.gravity = Gravity.CENTER_VERTICAL;
                    no.setLayoutParams(noParams);

                    layoutRow.addView(no);

                    layoutSingleCard.setOrientation(LinearLayout.VERTICAL);
                    layoutSingleCardParams.width = 0;
                    layoutSingleCardParams.weight = 8;

                    layoutSingleCard.setLayoutParams(layoutSingleCardParams);

                    titleRowParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    title.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 17 : 15);
                    titleRowParams.bottomMargin = MARGIN_VERTICAL;
                    title.setTypeface(null, Typeface.BOLD);
                    title.setLayoutParams(titleRowParams);
                    title.setMaxLines(5);
                    title.setEllipsize(TextUtils.TruncateAt.END);
                    layoutSingleCard.addView(title);

                    descRowParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    setTextFromFeed(desc, TIME, index);
                    descRowParams.bottomMargin = 20;
                    desc.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 14 : 12);
                    desc.setLayoutParams(descRowParams);
                    layoutSingleCard.addView(desc);

                    layoutRow.addView(layoutSingleCard);
                    break;
                case CardType.CATEGORY_GROUP: // TOP_NEWS
                    TableRow layoutRowPhotos = (TableRow) view;

                    tableParamsTopNews.setMargins(0, 0, 0, 3 * MARGIN_VERTICAL);
                    tableParamsTopNews.weight = 1;
                    tableParamsTopNews.height = 0;
                    layoutRowPhotos.setPadding(30, 0, 30, 0);
                    layoutRowPhotos.setLayoutParams(tableParamsTopNews);

                    layoutSingleCard.setOrientation(LinearLayout.VERTICAL);
                    linearLayoutRowParams.width = 0;
                    linearLayoutRowParams.weight =
                            ConfigurationUtils.isTablet(mActivity) ? 3.5f : 3f;
                    linearLayoutRowParams.gravity = Gravity.CENTER_VERTICAL;
                    linearLayoutRowParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;
                    layoutSingleCard.setGravity(Gravity.CENTER_VERTICAL);
                    layoutSingleCard.setLayoutParams(linearLayoutRowParams);

                    source.setLines(1);
                    sourceRowParams.gravity = Gravity.BOTTOM;
                    source.setPadding(0, 0, 0, 0);
                    sourceRowParams.height = TableRow.LayoutParams.WRAP_CONTENT;
                    sourceRowParams.setMargins(0, 0, 0, MARGIN_VERTICAL);
                    source.setLayoutParams(sourceRowParams);
                    source.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 16 : 12);
                    source.setTypeface(null, Typeface.BOLD);
                    setTextFromFeed(source, PUBLISHER, index);
                    source.setTextColor(mActivity.getResources().getColor(R.color.news_time_color));
                    layoutSingleCard.addView(source);

                    titleRowParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    titleRowParams.rightMargin = ConfigurationUtils.isTablet(mActivity) ? 8 : 13;
                    title.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 17 : 13);
                    title.setTypeface(null, Typeface.BOLD);
                    titleRowParams.bottomMargin = MARGIN_VERTICAL;
                    title.setLayoutParams(titleRowParams);
                    title.setMaxLines(4);
                    title.setEllipsize(TextUtils.TruncateAt.END);

                    layoutSingleCard.addView(title);

                    descRowParams.height = LinearLayout.LayoutParams.WRAP_CONTENT;

                    desc.setPadding(0, 0, 0, 0);
                    desc.setTextSize(ConfigurationUtils.isTablet(mActivity) ? 15 : 11);
                    descRowParams.bottomMargin = MARGIN_VERTICAL;
                    setTextFromFeed(desc, TIME, index);
                    desc.setLayoutParams(descRowParams);
                    layoutSingleCard.addView(desc);

                    layoutRowPhotos.addView(layoutSingleCard);

                    imageRowParams.width = 0;
                    imageRowParams.height = ConfigurationUtils.isTablet(mActivity)
                            ? 250
                            : (int) (mDeviceWidth * 0.2);
                    imageRowParams.weight = 1;
                    imageRowParams.setMargins(0, 20, 0, 0);
                    image.setScaleType(ImageView.ScaleType.CENTER_CROP);

                    setImage(image, "paired", index);

                    imageRowParams.gravity = Gravity.CENTER_VERTICAL;
                    image.setLayoutParams(imageRowParams);
                    layoutRowPhotos.addView(image);

                    break;
                default:

                    setTextFromFeed(title, TITLE, index);
                    setTextFromFeed(desc, DESC, index);
                    layout.addView(image);
                    layout.addView(title);
                    layout.addView(desc);
                    break;
            }

            title.setTextColor(mActivity.getResources().getColor(R.color.news_text_color));
            desc.setTextColor(mActivity.getResources().getColor(R.color.news_time_color));

            setTextFromFeed(title, TITLE, index);

            final FeedItemMetadata itemData = getItemData(index);

            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // @TODO alex refactor this with listener in BraveNewTabPageLayout
                    openUrlInSameTabAndSavePosition(itemData.url.url);
                }
            });

            title.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // @TODO alex refactor this with listener in BraveNewTabPageLayout
                    openUrlInSameTabAndSavePosition(itemData.url.url);
                }
            });

            title.setOnLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View v) {
                    showBottomSheetDialog(
                            itemData.url.url, itemData.publisherId, itemData.publisherName);
                    return true;
                }
            });

        } catch (Exception e) {
            Log.e(TAG, "Exception addElementsToSingleLayout: " + e.getMessage());
        }
    }

    private void showBottomSheetDialog(String urlString, String publisherId, String publisherName) {
        BraveNewsBottomSheetDialogFragment bottomSheetDialog =
                BraveNewsBottomSheetDialogFragment.newInstance();
        Bundle bundle = new Bundle();
        bundle.putString("url", urlString);
        bundle.putString("publisherId", publisherId);
        bundle.putString("publisherName", publisherName);
        bottomSheetDialog.setArguments(bundle);
        bottomSheetDialog.setController(mBraveNewsController);

        bottomSheetDialog.show(((ChromeActivity) mActivity).getSupportFragmentManager(),
                "brave_news_bottom_sheet_dialog_fragment");
    }

    private void setTextFromFeed(TextView textView, int type, int index) {
        try {
            FeedItemMetadata itemData = getItemData(index);
            if (itemData != null) {
                setText(itemData, textView, type);
                setListeners(textView, itemData.url.url, mCreativeInstanceId, mIsPromo);
            }

        } catch (Exception e) {
            Log.e(TAG, " exceptionscaught e:" + e.getMessage());
        }
    }

    private FeedItemMetadata getItemData(int index) {
        FeedItemMetadata itemData = null;
        try {
            List<FeedItemCard> feedItemsCard = mNewsItem.getFeedItems();
            if (feedItemsCard == null) {
                return null;
            }
            FeedItemCard feedItemCard = feedItemsCard.get(index);
            FeedItem feedItem = feedItemCard.getFeedItem();

            switch (feedItem.which()) {
                case FeedItem.Tag.Article:

                    Article article = feedItem.getArticle();
                    FeedItemMetadata articleData = article.data;
                    itemData = article.data;

                    break;
                case FeedItem.Tag.PromotedArticle:
                    PromotedArticle promotedArticle = feedItem.getPromotedArticle();
                    FeedItemMetadata promotedArticleData = promotedArticle.data;
                    mCreativeInstanceId = promotedArticle.creativeInstanceId;
                    itemData = promotedArticle.data;

                    mIsPromo = true;
                    break;
                case FeedItem.Tag.Deal:
                    Deal deal = feedItem.getDeal();
                    FeedItemMetadata dealData = deal.data;
                    mOffersCategory = deal.offersCategory;
                    itemData = deal.data;

                    break;
            }
        } catch (Exception e) {
            Log.e(TAG, " exceptionscaught getItemData e:" + e.getMessage());
        }

        final FeedItemMetadata itemDataFinal = itemData;

        return itemDataFinal;
    }

    private void setListeners(
            View view, String urlString, String creativeInstanceId, boolean isPromo) {
        DisplayAd displayAd = mNewsItem.getDisplayAd();
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isPromo) {
                    // Updates the no. of promotion cards visited
                    mBraveNewsController.onPromotedItemVisit(
                            mNewsItem.getUuid(), creativeInstanceId);
                } else if (displayAd != null) {
                    // Updates the no. of ads cards visited
                    mBraveNewsController.onDisplayAdVisit(
                            displayAd.uuid, displayAd.creativeInstanceId);
                } else {
                    // Brave News updates the no. of "normal" cards visited
                    int visitedNewsCardsCount = SharedPreferencesManager.getInstance().readInt(
                            BravePreferenceKeys.BRAVE_NEWS_CARDS_VISITED);
                    visitedNewsCardsCount++;
                    SharedPreferencesManager.getInstance().writeInt(
                            BravePreferenceKeys.BRAVE_NEWS_CARDS_VISITED, visitedNewsCardsCount);
                    if (visitedNewsCardsCount > 0) {
                        mBraveNewsController.onSessionCardVisitsCountChanged(
                                (short) visitedNewsCardsCount);
                    }
                }
            }
        });
    }

    private void setText(FeedItemMetadata itemData, TextView textView, int type) {
        switch (type) {
            case TITLE:
                textView.setText(itemData.title);
                break;
            case DESC:
                textView.setText(itemData.description);
                break;
            case PUBLISHER:
                textView.setText(itemData.publisherName);
                break;
            case TIME:
                textView.setText(itemData.relativeTimeDescription);
                break;
            case DEALS:
                textView.setText(mOffersCategory);
                break;
            case CATEGORY:
                textView.setText(itemData.categoryName);
                break;
        }
    }

    private void setDisplayAdImage(ImageView imageView, Image adDataImage, int index) {
        Url imageUrlTemp = null;

        switch (adDataImage.which()) {
            case Image.Tag.PaddedImageUrl:
                imageUrlTemp = adDataImage.getPaddedImageUrl();
                break;
            case Image.Tag.ImageUrl:
                imageUrlTemp = adDataImage.getImageUrl();
                break;
        }

        final Url adImageUrl = imageUrlTemp;
        mBraveNewsController.getImageData(adImageUrl, imageData -> {
            if (imageData != null) {
                Bitmap decodedByte = BitmapFactory.decodeByteArray(imageData, 0, imageData.length);
                Glide.with(mActivity)
                        .asBitmap()
                        .load(decodedByte)
                        .fitCenter()
                        .priority(Priority.IMMEDIATE)
                        .diskCacheStrategy(DiskCacheStrategy.ALL)
                        .into(new CustomTarget<Bitmap>() {
                            @Override
                            public void onResourceReady(@NonNull Bitmap resource,
                                    @Nullable Transition<? super Bitmap> transition) {
                                imageView.setImageBitmap(resource);
                            }
                            @Override
                            public void onLoadCleared(@Nullable Drawable placeholder) {}
                        });
                imageView.setClipToOutline(true);
            }
        });
    }

    private void setImage(ImageView imageView, String type, int index) {
        List<FeedItemCard> feedItemsCard = mNewsItem.getFeedItems();
        if (feedItemsCard != null) {
            FeedItemCard feedItemCard = feedItemsCard.get(index);
            FeedItem item = feedItemCard.getFeedItem();

            FeedItemMetadata itemMetaData = new FeedItemMetadata();
            switch (item.which()) {
                case FeedItem.Tag.Article:
                    Article article = item.getArticle();
                    itemMetaData = article.data;
                    break;
                case FeedItem.Tag.PromotedArticle:
                    PromotedArticle promotedArticle = item.getPromotedArticle();
                    itemMetaData = promotedArticle.data;
                    break;
                case FeedItem.Tag.Deal:
                    Deal deal = item.getDeal();
                    itemMetaData = deal.data;
                    break;
            }

            Url itemImageUrl = getImage(itemMetaData);
            mBraveNewsController.getImageData(itemImageUrl, imageData -> {
                if (imageData != null) {
                    Bitmap decodedByte =
                            BitmapFactory.decodeByteArray(imageData, 0, imageData.length);
                    GranularRoundedCorners radius = new GranularRoundedCorners(15, 15, 15, 15);
                    if (!type.equals("paired")) {
                        radius = new GranularRoundedCorners(30, 30, 0, 0);
                    }
                    RequestOptions requestOptions = new RequestOptions();
                    requestOptions =
                            requestOptions.centerInside().transform(new CenterCrop(), radius);
                    Glide.with(mActivity)
                            .asBitmap()
                            .load(decodedByte)
                            .centerCrop()
                            .apply(requestOptions)
                            .into(imageView);
                }
            });
        }
    }

    // used for logging the Glide implementation
    private RequestListener<Drawable> createLoggerListener(final String name) {
        return new RequestListener<Drawable>() {
            @Override
            public boolean onLoadFailed(@Nullable GlideException e, Object model, Target target,
                    boolean isFirstResource) {
                return false;
            }
            @Override
            public boolean onResourceReady(Drawable resource, Object model, Target target,
                    DataSource dataSource, boolean isFirstResource) {
                if (resource instanceof BitmapDrawable) {
                    Bitmap bitmap = ((BitmapDrawable) resource).getBitmap();
                    // Log.d(TAG,String.format(Locale.getDefault(), "Ready %s bitmap %,d bytes,
                    // size: %d x %d",
                    //         name,
                    //         bitmap.getByteCount(),
                    //         bitmap.getWidth(),
                    //         bitmap.getHeight()));
                }
                return false;
            }
        };
    }

    private LayerDrawable makeRound(int type, int background, int radius) {
        float[] outerRadii =
                new float[] {radius, radius, radius, radius, radius, radius, radius, radius};
        float[] innerRadii =
                new float[] {radius, radius, radius, radius, radius, radius, radius, radius};

        // Set the shape background card_background
        ShapeDrawable backgroundShape = null;
        new ShapeDrawable(new RoundRectShape(outerRadii, null, innerRadii));

        if (type == BUTTON_LAYOUT) {
            backgroundShape = new ShapeDrawable(new RoundRectShape(outerRadii, null, innerRadii));
            backgroundShape.getPaint().setColor(
                    mActivity.getResources().getColor(background)); // background color
            backgroundShape.getPaint().setStyle(Paint.Style.FILL); // Define background
            backgroundShape.getPaint().setAntiAlias(true);
            backgroundShape.setPadding(10, 10, 10, 10);

        } else if (type == CARD_LAYOUT) {
            backgroundShape = new ShapeDrawable(new RoundRectShape(outerRadii, null, innerRadii));
            backgroundShape.getPaint().setColor(
                    mActivity.getResources().getColor(background)); // background color
            backgroundShape.getPaint().setStyle(Paint.Style.FILL); // Define background
        } else if (type == ROUNDED_TOP_LAYOUT) {
            backgroundShape = new ShapeDrawable(new RoundRectShape(outerRadii, null, null));
            backgroundShape.getPaint().setColor(
                    mActivity.getResources().getColor(background)); // background color
            backgroundShape.getPaint().setStyle(Paint.Style.FILL); // Define background
        }

        // Initialize an array of drawables
        Drawable[] drawables = new Drawable[] {backgroundShape};
        // Initialize a layer drawable object
        LayerDrawable layerDrawable = new LayerDrawable(drawables);
        return layerDrawable;
    }
}
