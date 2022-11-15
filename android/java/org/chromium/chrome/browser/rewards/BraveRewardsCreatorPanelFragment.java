/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.animation.TranslateAnimation;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.json.JSONException;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPublisher;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ledger.mojom.WalletStatus;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class BraveRewardsCreatorPanelFragment extends Fragment
        implements BraveRewardsHelper.LargeIconReadyCallback, BraveRewardsObserver {
    private int currentTabId_;
    private boolean monthlyContribution;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private final int PUBLISHER_ICON_SIDE_LEN = 50; // dp
    private BraveRewardsHelper mIconFetcher;
    private boolean mTippingInProgress; // flag preventing multiple tipping processes
    private BraveRewardsBannerInfo mBannerInfo;

    private final String TWITTER = "twitter";
    private final String YOUTUBE = "youtube";
    private final String TWITCH = "twitch";
    private final String GITHUB = "github";
    private final String REDDIT = "reddit";
    private final String VIMEO = "vimeo";

    private static final String TAG = "TippingBanner";

    public static BraveRewardsCreatorPanelFragment newInstance(
            int tabId, boolean isMonthlyContribution, String bannerInfo) {
        BraveRewardsCreatorPanelFragment fragment = new BraveRewardsCreatorPanelFragment();
        Bundle args = new Bundle();
        args.putInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, tabId);
        args.putBoolean(
                BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, isMonthlyContribution);
        args.putString(BraveRewardsSiteBannerActivity.BANNER_INFO_ARGS, bannerInfo);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (getArguments() != null) {
            currentTabId_ = getArguments().getInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA);
            monthlyContribution = getArguments().getBoolean(
                    BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            String bannerInfoJson =
                    getArguments().getString(BraveRewardsSiteBannerActivity.BANNER_INFO_ARGS);
            try {
                mBannerInfo = new BraveRewardsBannerInfo(bannerInfoJson);
            } catch (JSONException e) {
                Log.e(TAG, "TippingBanner -> CreatorPanel:onAttach JSONException error " + e);
            }
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_rewards_creator_panel, container, false);
        init(view);

        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mIconFetcher != null) {
            mIconFetcher.detach();
        }

        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }

    private void init(View view) {
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);
        mBraveRewardsNativeWorker.GetExternalWallet();

        checkForShowSocialLinkIcons(view);
        showVerifiedIcon(view);
        retrivePublisherName(view);
        retriveFavIcon();
        setDescription(view);
        setTitle(view);
    }

    private void checkForShowSocialLinkIcons(View view) {
        if (mBannerInfo == null) return;
        HashMap<String, String> links = mBannerInfo.getLinks();
        if (links != null) {
            for (Map.Entry<String, String> element : links.entrySet()) {
                String name = element.getKey();
                String url = element.getValue();
                showSocialLinkIcon(view, name, url);
            }
        }
    }

    private void showSocialLinkIcon(View view, String name, String url) {
        switch (name) {
            case TWITTER:
                ImageView twitterIcon = view.findViewById(R.id.twitterButton);
                showAndSetListener(twitterIcon, url);
                break;

            case YOUTUBE:
                ImageView youtubeIcon = view.findViewById(R.id.youtubeButton);
                showAndSetListener(youtubeIcon, url);
                break;

            case TWITCH:
                ImageView twitchIcon = view.findViewById(R.id.twitchButton);
                showAndSetListener(twitchIcon, url);
                break;
            case GITHUB:
                ImageView githubIcon = view.findViewById(R.id.githubButton);
                showAndSetListener(githubIcon, url);
                break;

            case REDDIT:
                ImageView redditIcon = view.findViewById(R.id.redditButton);
                showAndSetListener(redditIcon, url);
                break;

            case VIMEO:
                ImageView vimeoIcon = view.findViewById(R.id.vimeoButton);
                showAndSetListener(vimeoIcon, url);
                break;
        }
    }

    private void showAndSetListener(ImageView icon, String url) {
        icon.setVisibility(View.VISIBLE);
        icon.setTag(url);
        icon.setOnClickListener(v -> {
            String openUrl = (String) v.getTag();
            openLink(openUrl);
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
        String publisherFavIconURL =
                mBraveRewardsNativeWorker.GetPublisherFavIconURL(currentTabId_);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
        String url = currentActiveTab != null ? currentActiveTab.getUrl().getSpec() : "";
        String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;
        mIconFetcher = new BraveRewardsHelper(currentActiveTab);
        mIconFetcher.retrieveLargeIcon(favicon_url, this);
    }

    private void setDescription(View view) {
        if (mBannerInfo == null) return;
        String description = mBannerInfo.getDescription();
        if (description != null && !description.isEmpty()) {
            TextView descriptionTextView = view.findViewById(R.id.rewards_banner_description);
            descriptionTextView.setText(description);
        }
    }

    private void setTitle(View view) {
        if (mBannerInfo == null) return;
        String title = mBannerInfo.getTitle();
        if (title != null && !title.isEmpty()) {
            TextView titleTextView = view.findViewById(R.id.banner_title);
            titleTextView.setText(title);
        }
    }

    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null) {
            int nPx = BraveRewardsHelper.dp2px(PUBLISHER_ICON_SIDE_LEN);
            Bitmap resized = Bitmap.createScaledBitmap(bmp, nPx, nPx, true);
            getActivity().runOnUiThread(() -> {
                ImageView iv = (ImageView) getActivity().findViewById(R.id.publisher_favicon);
                View fadeout = getActivity().findViewById(R.id.publisher_favicon_update);
                BraveRewardsHelper.crossfade(
                        fadeout, iv, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
                iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(resized));
            });
        }
    }

    @Override
    public void onLargeIconReady(Bitmap icon) {
        SetFavIcon(icon);
    }

    @Override
    public void OnGetExternalWallet(String externalWallet) {
        int walletStatus = WalletStatus.NOT_CONNECTED;
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                BraveRewardsExternalWallet mExternalWallet =
                        new BraveRewardsExternalWallet(externalWallet);
                walletStatus = mExternalWallet.getStatus();
            } catch (JSONException e) {
                Log.e(TAG, "BraveRewardsCreatorPanelFragment" + e.getMessage());
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
        if (walletStatus == WalletStatus.NOT_CONNECTED) {
            if (pubStatus == BraveRewardsPublisher.CONNECTED
                    || pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                    || pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                    || pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED) {
                Log.d(TAG, "User is unverified and publisher is verified");
            } else {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_unverified_notice_text);
                Log.d(TAG, "User is unverified and publisher is unverified");
            }
        } else {
            if (pubStatus == BraveRewardsPublisher.NOT_VERIFIED) {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_unverified_notice_text);
                Log.d(TAG, "User is verified and publisher is unverified");
            } else if (pubStatus == BraveRewardsPublisher.CONNECTED
                    || (pubStatus == BraveRewardsPublisher.UPHOLD_VERIFIED
                            && !walletType.equals(BraveWalletProvider.UPHOLD))
                    || (pubStatus == BraveRewardsPublisher.BITFLYER_VERIFIED
                            && !walletType.equals(BraveWalletProvider.BITFLYER))
                    || (pubStatus == BraveRewardsPublisher.GEMINI_VERIFIED
                            && !walletType.equals(BraveWalletProvider.GEMINI))) {
                notePart1 = getResources().getString(
                        R.string.brave_ui_site_banner_different_verified_notice_text);
                Log.d(TAG,
                        "User is verified and publisher is verified but not with the same provider");
            }
        }

        if (!TextUtils.isEmpty(notePart1)) {
            getActivity().findViewById(R.id.not_verified_warning_text).setVisibility(View.VISIBLE);
            String notePart2 = getResources().getString(R.string.learn_more_tip);
            final StringBuilder sb1 = new StringBuilder();
            sb1.append("<b>");
            sb1.append(getResources().getString(R.string.note_text));
            sb1.append("</b>");
            sb1.append(" ");
            sb1.append(notePart1);
            sb1.append(" ");
            sb1.append("<b><font color=#00afff>");
            sb1.append(notePart2);
            sb1.append("</font></b>");
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(sb1.toString());
            TextView not_verified_warning_text =
                    (TextView) getActivity().findViewById(R.id.not_verified_warning_text);
            not_verified_warning_text.setText(toInsert);
            String full_note_str = toInsert.toString();

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
        }
    }
}
