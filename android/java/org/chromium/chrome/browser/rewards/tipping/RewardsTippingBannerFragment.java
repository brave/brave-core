/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.res.ResourcesCompat;
import androidx.fragment.app.Fragment;

import com.bumptech.glide.Glide;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.rewards.BraveRewardsBannerInfo;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.ViewUtils;

import java.util.HashMap;
import java.util.Map;

public class RewardsTippingBannerFragment extends Fragment implements BraveRewardsObserver {
    public static final String TAB_ID_EXTRA = "currentTabId";
    public static final String TIP_MONTHLY_EXTRA = "tipMonthly";
    public static final String TIP_AMOUNT_EXTRA = "tipAmount";
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private static final String TAG = "TippingBanner";
    private int mCurrentTabId = -1;
    private BraveRewardsBannerInfo mBannerInfo;

    private static final String TWITTER = "twitter";
    private static final String YOUTUBE = "youtube";
    private static final String TWITCH = "twitch";
    private static final String GITHUB = "github";
    private static final String REDDIT = "reddit";
    private static final String VIMEO = "vimeo";
    private static final String DISCORD = "discord";
    private static final String FACEBOOK = "facebook";
    private static final String INSTAGRAM = "instagram";
    private static final int LOGO_RADIUS = 60;

    private static final String IMAGE_URL_PREFIX = "chrome://rewards-image/";
    private AppCompatActivity mActivity;

    public static RewardsTippingBannerFragment newInstance(int currentTabId) {
        RewardsTippingBannerFragment fragment = new RewardsTippingBannerFragment();
        Bundle args = new Bundle();
        args.putInt(TAB_ID_EXTRA, currentTabId);
        fragment.setArguments(args);
        return fragment;
    }

    private View mContentView;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.rewards_tipping_banner_fragment, container, false);
        mCurrentTabId = getArguments().getInt(TAB_ID_EXTRA);
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.addObserver(this);
        mBraveRewardsNativeWorker.getPublisherBanner(
                mBraveRewardsNativeWorker.getPublisherId(mCurrentTabId));
        mContentView = view;
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mActivity = (AppCompatActivity) getActivity();
        ((TextView) view.findViewById(R.id.tipping_details_description))
                .setMovementMethod(new ScrollingMovementMethod());

        clickOnVerifiedIcon();
        sendTipButtonClick();
        closeButtonClick();
    }

    private void closeButtonClick() {
        ImageView view = mContentView.findViewById(R.id.close_it);
        view.setOnClickListener(v -> { mActivity.finish(); });
    }

    private void clickOnVerifiedIcon() {
        ImageView view = mContentView.findViewById(R.id.verified_tick_mark);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TippingVerifiedCreatorToolTip toolTip =
                        new TippingVerifiedCreatorToolTip(mActivity);
                toolTip.show(view);
            }
        });
    }

    private void background() {
        if (mBannerInfo == null) return;
        String background = mBannerInfo.getBackground();
        if (!TextUtils.isEmpty(background)) {
            String prefix = IMAGE_URL_PREFIX;
            if (background.contains(prefix)) {
                background = background.substring(prefix.length());
                ImageView iv = (ImageView) mContentView.findViewById(
                        R.id.top_tipping_banner_ui_background_scenery);
                ImageLoader.downloadImage(background, Glide.with(mActivity), false, 1, iv, null);
            }
        }
    }

    @SuppressLint("SetTextI18n")
    private void setDescription() {
        if (mBannerInfo == null) return;
        String description = mBannerInfo.getDescription();
        if (!TextUtils.isEmpty(description)) {
            TextView descriptionTextView =
                    mContentView.findViewById(R.id.tipping_details_description);
            descriptionTextView.setText(
                    description + "\n\n"); // Adding two new line to show shadow if more text exist
        }
    }

    @Override
    public void onPublisherBanner(String jsonBannerInfo) {
        try {
            mBannerInfo = new BraveRewardsBannerInfo(jsonBannerInfo);
            setTitle();
            setDescription();
            setLogo();
            background();
            checkForShowSocialLinkIcons();
            mBraveRewardsNativeWorker.getExternalWallet();
        } catch (JSONException e) {
            Log.e(TAG, "TippingBanner -> CreatorPanel:onAttach JSONException error " + e);
        }
    }

    @Override
    public void onGetExternalWallet(String externalWallet) {
        if (!TextUtils.isEmpty(externalWallet)) {
            try {
                BraveRewardsExternalWallet braveRewardsExternalWallet =
                        new BraveRewardsExternalWallet(externalWallet);
                String custodianType = braveRewardsExternalWallet.getType();
                boolean isSolanaWallet =
                        (!TextUtils.isEmpty(custodianType)
                                && custodianType.equals(BraveWalletProvider.SOLANA));
                Button sendTipButton = mContentView.findViewById(R.id.send_tip_button);
                Button web3Button = mContentView.findViewById(R.id.use_web3_wallet_button);
                if (mBannerInfo != null) {
                    String web3Url = mBannerInfo.getWeb3Url();
                    if (!TextUtils.isEmpty(web3Url)
                            && !DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity())) {
                        web3Button.setVisibility(View.VISIBLE);
                        web3Button.setOnClickListener(
                                v -> {
                                    TabUtils.openUrlInNewTab(false, web3Url);
                                    dismissRewardsPanel();
                                });
                    } else {
                        if (isSolanaWallet) {
                            showWarningMessage(mContentView);
                        }
                    }
                }
                if (isSolanaWallet) {
                    sendTipButton.setVisibility(View.GONE);
                    web3Button.setBackgroundDrawable(
                            ResourcesCompat.getDrawable(
                                    getResources(),
                                    R.drawable.tipping_send_button_background,
                                    null));
                    web3Button.setTextColor(getActivity().getColor(android.R.color.white));
                }
            } catch (JSONException e) {
                Log.e(TAG, "TippingBanner -> OnGetExternalWallet " + e.getMessage());
            }
        }
    }

    private void checkForShowSocialLinkIcons() {
        if (mBannerInfo == null) return;
        HashMap<String, String> links = mBannerInfo.getLinks();
        if (links != null) {
            for (Map.Entry<String, String> element : links.entrySet()) {
                String name = element.getKey();
                String url = element.getValue();
                showSocialLinkIcon(name, url);
            }
        }
    }

    private void showSocialLinkIcon(String name, String url) {
        switch (name) {
            case TWITTER:
                ImageView twitterIcon = mContentView.findViewById(R.id.link_twitter);
                showAndSetListener(twitterIcon, url);
                break;

            case YOUTUBE:
                ImageView youtubeIcon = mContentView.findViewById(R.id.link_youtube);
                showAndSetListener(youtubeIcon, url);
                break;

            case TWITCH:
                ImageView twitchIcon = mContentView.findViewById(R.id.link_twitch);
                showAndSetListener(twitchIcon, url);
                break;

            case GITHUB:
                ImageView githubIcon = mContentView.findViewById(R.id.link_github);
                showAndSetListener(githubIcon, url);
                break;

            case REDDIT:
                ImageView redditIcon = mContentView.findViewById(R.id.link_reddit);
                showAndSetListener(redditIcon, url);
                break;

            case VIMEO:
                ImageView vimeoIcon = mContentView.findViewById(R.id.link_vimeo);
                showAndSetListener(vimeoIcon, url);
                break;

            case DISCORD:
                ImageView discordIcon = mContentView.findViewById(R.id.link_discord);
                showAndSetListener(discordIcon, url);
                break;

            case FACEBOOK:
                ImageView facebookIcon = mContentView.findViewById(R.id.link_facebook);
                showAndSetListener(facebookIcon, url);
                break;

            case INSTAGRAM:
                ImageView instagramIcon = mContentView.findViewById(R.id.link_instagram);
                showAndSetListener(instagramIcon, url);
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
        TabUtils.openUrlInNewTab(false, url);
        dismissRewardsPanel();
    }

    private void setLogo() {
        if (mBannerInfo == null) return;
        String logo = mBannerInfo.getLogo();
        if (!TextUtils.isEmpty(logo)) {
            String prefix = IMAGE_URL_PREFIX;
            if (logo.contains(prefix)) {
                logo = logo.substring(prefix.length());
                ImageView iv = (ImageView) mContentView.findViewById(R.id.publisher_favicon);
                int radius = ViewUtils.dpToPx(mActivity, LOGO_RADIUS);
                ImageLoader.downloadImage(logo, Glide.with(mActivity), false, radius, iv, null);
            }
        }
    }

    private void setTitle() {
        if (mBannerInfo == null) return;
        TextView titleTextView = mContentView.findViewById(R.id.tipping_publisher_name);
        String title = mBannerInfo.getName();
        if (!TextUtils.isEmpty(title)) {
            String provider = getProvider(mBannerInfo.getProvider());
            if (!TextUtils.isEmpty(provider)) {
                title = String.format(
                        getResources().getString(R.string.title_on_provider), title, provider);
            }
            titleTextView.setText(title);
        } else {
            title = mBannerInfo.getTitle();
            if (!TextUtils.isEmpty(title)) {
                titleTextView.setText(title);
            }
        }
    }

    private String getProvider(String provider) {
        if (TextUtils.isEmpty(provider)) {
            return "";
        }
        switch (provider) {
            case "twitter":
                return "Twitter";
            case "youtube":
                return "YouTube";
            case "twitch":
                return "Twitch";
            case "reddit":
                return "Reddit";
            case "vimeo":
                return "Vimeo";
            case "github":
                return "GitHub";
            default:
                return provider;
        }
    }

    public void sendTipButtonClick() {
        View sendTipButton = mContentView.findViewById(R.id.send_tip_button);
        sendTipButton.setOnClickListener(
                (v) -> {
                    RewardsTippingPanelBottomsheet.showTippingPanelBottomSheet(
                            (AppCompatActivity) mActivity, mCurrentTabId);
                });

    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.removeObserver(this);
        }
    }

    private void dismissRewardsPanel() {
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());

        if (isTablet) {
            try {
                BraveActivity braveActivity = BraveActivity.getBraveActivity();
                braveActivity.dismissRewardsPanel();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "setShareYourSupportClickListener " + e);
            }
        } else {
            mActivity.setResult(Activity.RESULT_OK);
            mActivity.finish();
        }
    }

    private void showWarningMessage(View view) {
        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity())) {
            return;
        }
        View warningLayout = view.findViewById(R.id.tipping_warning_message_layout);
        warningLayout.setVisibility(View.VISIBLE);
        warningLayout.setBackground(
                ResourcesCompat.getDrawable(
                        getResources(),
                        R.drawable.rewards_tipping_web3_error_background,
                        /* theme= */ null));
        ImageView tippingWarningIcon = view.findViewById(R.id.tipping_warning_icon);
        tippingWarningIcon.setVisibility(View.VISIBLE);
        TextView warningTitle = view.findViewById(R.id.tipping_warning_title_text);
        warningTitle.setText(getResources().getString(R.string.can_not_send_your_contribution));
        warningTitle.setTextColor(getActivity().getColor(R.color.tipping_web3_error_text_color));
        TextView warningDescription = view.findViewById(R.id.tipping_warning_description_text);
        warningDescription.setText(getResources().getString(R.string.creator_isnt_setup_web3));
        warningDescription.setTextColor(
                getActivity().getColor(R.color.tipping_web3_error_text_color));
    }
}
