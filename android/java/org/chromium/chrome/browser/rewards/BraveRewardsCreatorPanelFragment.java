/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.os.Bundle;

import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;

import android.widget.ImageView;
import android.widget.TextView;
import org.chromium.chrome.browser.tab.Tab;
import android.content.Context;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.animation.TranslateAnimation;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import org.json.JSONException;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.Locale;
import java.util.HashMap;
import java.util.Map;


import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;







import android.graphics.Bitmap;

public class BraveRewardsCreatorPanelFragment extends Fragment implements
        BraveRewardsHelper.LargeIconReadyCallback, BraveRewardsObserver {

    private int currentTabId_;
    private boolean monthlyContribution;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private final int PUBLISHER_ICON_SIDE_LEN= 50; //dp
    private final int TOUCH_PADDING_MONTHLY= 32; //dp
    private final int ACTIVITY_HEADER_PADDING = 10; //dp
    private final int ACTIVITY_HEADER_CONTENT_SIDE_MARGIN = 22; //dp
    private BraveRewardsHelper mIconFetcher;
    private boolean mTippingInProgress; //flag preventing multiple tipping processes
    private BraveRewardsBannerInfo mBannerInfo;

    public static BraveRewardsCreatorPanelFragment newInstance(int tabId, 
        boolean isMonthlyContribution, BraveRewardsBannerInfo bannerInfo) {
        BraveRewardsCreatorPanelFragment fragment = new BraveRewardsCreatorPanelFragment();
        Bundle args = new Bundle();
        args.putInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, tabId);
        args.putBoolean(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, isMonthlyContribution);
        args.putParcelable(BraveRewardsSiteBannerActivity.BANNER_INFO_ARGS, bannerInfo);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (getArguments() != null) {
            currentTabId_ = getArguments().getInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA);
            monthlyContribution = getArguments().getBoolean(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            mBannerInfo = getArguments().getParcelable(BraveRewardsSiteBannerActivity.BANNER_INFO_ARGS);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view =  inflater.inflate(R.layout.brave_rewards_creator_panel, container, false);
        init(view);

        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mIconFetcher != null) {
            mIconFetcher.detach();
        }

        if (null != mBraveRewardsNativeWorker){
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }

    @Override
    public void onPublisherBanner(String jsonBannerInfo) {
        Log.e("SujitSujit", "onPublisherBanner jsonBannerInfo "+jsonBannerInfo);
    }

    private void init(View view) {
        Log.e("SujitSujit", "init");

        Log.e("SujitSujit", "onCreateView jsonBannerInfo name " + mBannerInfo.getName());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getTitle " + mBannerInfo.getTitle());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getDescription " + mBannerInfo.getDescription());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getBackground "+ mBannerInfo.getBackground());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getLogo "+ mBannerInfo.getLogo());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getProvider "+ mBannerInfo.getProvider());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getLinks "+ mBannerInfo.getLinks());
        Log.e("SujitSujit", "onCreateView jsonBannerInfo getStatus " + mBannerInfo.getStatus());
        
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);
        mBraveRewardsNativeWorker.GetExternalWallet();
        //mBraveRewardsNativeWorker.GetPublisherBanner(mBraveRewardsNativeWorker.GetPublisherId(currentTabId_));
        
        checkForShowSocialLinkIcons(view);
        showVerifiedIcon(view);
        retrivePublisherName(view);
        retriveFavIcon();
        setDescription(view);
        setTitle(view);
    }

    private void checkForShowSocialLinkIcons(View view) {
        if(mBannerInfo == null)
            return;
        HashMap<String, String> links = mBannerInfo.getLinks();
        if (links != null) {
            for (Map.Entry<String,String> element : links.entrySet()) {
                String name = element.getKey();
                String url = element.getValue();
                Log.e("SujitSujit", "onCreateView jsonBannerInfo links k " + name +" v "+url);
                showSocialLinkIcon(view, name, url);
            }
        }

    }

    private void showSocialLinkIcon(View view, String name, String url) {
        switch (name) {
            case "youtube" :
                ImageView youtubeIcon = view.findViewById(R.id.youtube_button);
                showAndSetListener(youtubeIcon, url);
                break;
            case "twitter" :
                ImageView twitterIcon = view.findViewById(R.id.twitter_button);
                showAndSetListener(twitterIcon, url);
                break;
            case "twitch" :
                ImageView twitchIcon = view.findViewById(R.id.twitch_button);
                showAndSetListener(twitchIcon, url);
                break;
            case "reddit" :
                ImageView redditIcon = view.findViewById(R.id.reddit_button);
                showAndSetListener(redditIcon, url);
                break;
            default:
                ImageView chatIcon = view.findViewById(R.id.chat_button);
                showAndSetListener(chatIcon, url);
                break;
        }

    }

    private void showAndSetListener(ImageView icon, String url) {
        icon.setVisibility(View.VISIBLE);
        icon.setTag(url);
        icon.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String url = (String)v.getTag();
                openLink(url);
            }
        });
    }


    private void openLink(String url) {
        Intent intent = new Intent();
        intent.putExtra(BraveActivity.OPEN_URL, url);
        getActivity().setResult(Activity.RESULT_OK, intent);
        getActivity().finish();
    }


    private void showVerifiedIcon(View view) {
        @PublisherStatus
        int pubStatus = mBraveRewardsNativeWorker.GetPublisherStatus(currentTabId_);

        if (pubStatus == BraveRewardsPublisher.CONNECTED
                || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                || pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                || pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED) {
            view.findViewById(R.id.publisher_favicon_verified).setVisibility(View.VISIBLE);
        }
    }

    private void retrivePublisherName(View view) {
        String publisherName = mBraveRewardsNativeWorker.GetPublisherName(currentTabId_);
        TextView publisher = (TextView) view.findViewById(R.id.publisher_name);
        publisher.setText(publisherName);
    }

    private void retriveFavIcon() {
        String publisherFavIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(currentTabId_);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
        String url = currentActiveTab.getUrl().getSpec();
        String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;
        mIconFetcher = new BraveRewardsHelper(currentActiveTab);
        mIconFetcher.retrieveLargeIcon(favicon_url, this);
    }

    private void setDescription(View view) {
        if(mBannerInfo == null)
            return;
        String description = mBannerInfo.getDescription();
        if(description != null && !description.isEmpty()) {
            TextView descriptionTextView = (TextView) view.findViewById(R.id.rewards_banner_description);
            descriptionTextView.setText(description);
        }
    }

    private void setTitle(View view) {
        if(mBannerInfo == null)
            return;
        String title = mBannerInfo.getTitle();
        if(title != null && !title.isEmpty()) {
            TextView titleTextView = (TextView) view.findViewById(R.id.banner_title);
            titleTextView.setText(title);
        }

    }

    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null){
            getActivity().runOnUiThread(
                new Runnable(){
                    @Override
                    public void run() {
                        ImageView iv = (ImageView) getActivity().findViewById(R.id.publisher_favicon);
                        int nPx = BraveRewardsHelper.dp2px(PUBLISHER_ICON_SIDE_LEN);
                        Bitmap resized = Bitmap.createScaledBitmap(bmp, nPx, nPx, true);

                        View fadeout  = getActivity().findViewById(R.id.publisher_favicon_update);
                        BraveRewardsHelper.crossfade(fadeout, iv, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
                        iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(resized));
                    }
                });
        }
    }

    @Override
    public void onLargeIconReady(Bitmap icon){
        Log.e("SujitSujit", "onLargeIconReady");

        SetFavIcon(icon);
    }

    @Override
    public void OnGetExternalWallet(int errorCode, String externalWallet) {
        Log.e("SujitSujit", "BraveRewardsCreatorPanelFragment OnGetExternalWallet");
        int walletStatus = BraveRewardsExternalWallet.NOT_CONNECTED;
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                BraveRewardsExternalWallet mExternalWallet =
                        new BraveRewardsExternalWallet(externalWallet);
                walletStatus = mExternalWallet.getStatus();
            } catch (JSONException e) {
                Log.e("BraveRewards", e.getMessage());
            }
        }
        @PublisherStatus
        int pubStatus = mBraveRewardsNativeWorker.GetPublisherStatus(currentTabId_);
        setPublisherNoteText(pubStatus, walletStatus);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void setPublisherNoteText(@PublisherStatus int pubStatus, int walletStatus) {
        String notePart1 = "";
        String walletType = BraveRewardsNativeWorker.getInstance().getExternalWalletType();
        if (walletStatus == BraveRewardsExternalWallet.NOT_CONNECTED) {
            if (pubStatus == BraveRewardsPublisher.CONNECTED
                    || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                    || pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                    || pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED) {
                Log.e("BraveRewards", "User is unverified and publisher is verified");
            } else {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_unverified_notice_text);
                Log.e("BraveRewards", "User is unverified and publisher is unverified");
            }
        } else {
            if (pubStatus == BraveRewardsPublisher.NOT_VERIFIED) {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_unverified_notice_text);
                Log.e("BraveRewards", "User is verified and publisher is unverified");
            } else if (pubStatus == BraveRewardsPublisher.CONNECTED
                    || (pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                            && !walletType.equals(BraveWalletProvider.UPHOLD))
                    || (pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                            && !walletType.equals(BraveWalletProvider.BITFLYER))
                    || (pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED
                            && !walletType.equals(BraveWalletProvider.GEMINI))) {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_different_verified_notice_text);
                Log.e("BraveRewards",
                        "User is verified and publisher is verified but not with the same provider");
            }
        }

        if (!TextUtils.isEmpty(notePart1)) {
            getActivity().findViewById(R.id.not_verified_warning_text ).setVisibility(View.VISIBLE);
            String notePart2 = getResources().getString(R.string.learn_more);
            final StringBuilder sb1 = new StringBuilder();
            sb1.append(notePart1);
            sb1.append(" <font color=#00afff>");
            sb1.append(notePart2);
            sb1.append("</font>");
             Log.e("SujitSujit", "BraveRewardsCreatorPanelFragment OnGetExternalWallet isEmpty");
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(sb1.toString());
            TextView not_verified_warning_text = (TextView ) getActivity().findViewById(R.id.not_verified_warning_text );
            not_verified_warning_text.setText(toInsert);
            String full_note_str = toInsert.toString();
             Log.e("SujitSujit", "BraveRewardsCreatorPanelFragment OnGetExternalWallet isEmpty2");

            not_verified_warning_text.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View view, MotionEvent motionEvent) {
                    boolean event_consumed = false;
                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                        int offset = not_verified_warning_text.getOffsetForPosition(
                                motionEvent.getX(), motionEvent.getY());

                        if (BraveRewardsHelper.subtextAtOffset(full_note_str, notePart2, offset)) {
                            event_consumed = true;
                            openLink(BraveActivity.REWARDS_LEARN_MORE_URL);
                        }
                    }
                    return event_consumed;
                }
            });
             Log.e("SujitSujit", "BraveRewardsCreatorPanelFragment OnGetExternalWallet isEmpty3");

        }
    }
}