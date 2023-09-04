/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.RequestOptions;

import org.json.JSONException;

import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.rewards.BraveRewardsBannerInfo;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.HashMap;
import java.util.Map;

public class RewardsTippingBannerActivity
        extends AsyncInitializationActivity implements BraveRewardsObserver {
    public static final String TAB_ID_EXTRA = "currentTabId";
    public static final String TIP_MONTHLY_EXTRA = "tipMonthly";
    public static final String TIP_AMOUNT_EXTRA = "tipAmount";
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private final int PUBLISHER_ICON_SIDE_LEN = 64; // dp
    private static final String TAG = "TippingBanner";
    private int currentTabId_ = -1;
    private BraveRewardsBannerInfo mBannerInfo;

    private BraveRewardsHelper mIconFetcher;

    private final String TWITTER = "twitter";
    private final String YOUTUBE = "youtube";
    private final String TWITCH = "twitch";
    private final String GITHUB = "github";
    private final String REDDIT = "reddit";
    private final String VIMEO = "vimeo";
    private final String DISCORD = "discord";
    private final String FACEBOOK = "facebook";
    private final String INSTAGRAM = "instagram";

    private static final String IMAGE_URL_PREFIX = "chrome://rewards-image/";

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_tipping_banner_mobile);
        showCustomUI();
        ((TextView) findViewById(R.id.tipping_details_description))
                .setMovementMethod(new ScrollingMovementMethod());
        currentTabId_ = IntentUtils.safeGetIntExtra(getIntent(), TAB_ID_EXTRA, -1);

        onInitialLayoutInflationComplete();
        clickOnVerifiedIcon();
        sendTipButtonClick();
        closeButtonClick();
    }

    private void closeButtonClick() {
        ImageView view = findViewById(R.id.close_it);
        view.setOnClickListener(v -> { finish(); });
    }

    private void clickOnVerifiedIcon() {
        ImageView view = findViewById(R.id.verified_tick_mark);
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TippingVerifiedCreatorToolTip toolTip =
                        new TippingVerifiedCreatorToolTip(RewardsTippingBannerActivity.this);
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
                ImageView iv =
                        (ImageView) findViewById(R.id.top_tipping_banner_ui_background_scenery);
                Glide.with(this)
                        .load(background)
                        .placeholder(R.drawable.tipping_default_background)
                        .error(R.drawable.tipping_default_background)
                        .into(iv);
            }
        }
    }

    private void setDescription() {
        if (mBannerInfo == null) return;
        String description = mBannerInfo.getDescription();
        if (!TextUtils.isEmpty(description)) {
            TextView descriptionTextView = findViewById(R.id.tipping_details_description);
            descriptionTextView.setText(description);
        }
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
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
            setWeb3WalletClick();
        } catch (JSONException e) {
            Log.e(TAG, "TippingBanner -> CreatorPanel:onAttach JSONException error " + e);
        }
    }

    private void setWeb3WalletClick() {
        if (mBannerInfo == null) return;
        String web3Url = mBannerInfo.getWeb3Url();
        if (!TextUtils.isEmpty(web3Url)) {
            View web3Button = findViewById(R.id.use_web3_wallet_button);
            web3Button.setVisibility(View.VISIBLE);
            web3Button.setOnClickListener(v -> {
                TabUtils.openUrlInNewTab(false, web3Url);
                setResult(Activity.RESULT_OK);
                finish();
            });
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
                ImageView twitterIcon = findViewById(R.id.link_twitter);
                showAndSetListener(twitterIcon, url);
                break;

            case YOUTUBE:
                ImageView youtubeIcon = findViewById(R.id.link_youtube);
                showAndSetListener(youtubeIcon, url);
                break;

            case TWITCH:
                ImageView twitchIcon = findViewById(R.id.link_twitch);
                showAndSetListener(twitchIcon, url);
                break;

            case GITHUB:
                ImageView githubIcon = findViewById(R.id.link_github);
                showAndSetListener(githubIcon, url);
                break;

            case REDDIT:
                ImageView redditIcon = findViewById(R.id.link_reddit);
                showAndSetListener(redditIcon, url);
                break;

            case VIMEO:
                ImageView vimeoIcon = findViewById(R.id.link_vimeo);
                showAndSetListener(vimeoIcon, url);
                break;

            case DISCORD:
                ImageView discordIcon = findViewById(R.id.link_discord);
                showAndSetListener(discordIcon, url);
                break;

            case FACEBOOK:
                ImageView facebookIcon = findViewById(R.id.link_facebook);
                showAndSetListener(facebookIcon, url);
                break;

            case INSTAGRAM:
                ImageView instagramIcon = findViewById(R.id.link_instagram);
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
        Intent intent = new Intent();
        intent.putExtra(BraveActivity.OPEN_URL, url);
        setResult(Activity.RESULT_OK, intent);
        finish();
    }

    private void setLogo() {
        if (mBannerInfo == null) return;
        String logo = mBannerInfo.getLogo();
        if (!TextUtils.isEmpty(logo)) {
            String prefix = IMAGE_URL_PREFIX;
            if (logo.contains(prefix)) {
                logo = logo.substring(prefix.length());
                ImageView iv = (ImageView) findViewById(R.id.publisher_favicon);
                Glide.with(this).load(logo).apply(RequestOptions.circleCropTransform()).into(iv);
            }
        }
    }

    private void setTitle() {
        if (mBannerInfo == null) return;
        String title = mBannerInfo.getName();
        if (!TextUtils.isEmpty(title)) {
            TextView titleTextView = findViewById(R.id.tipping_publisher_name);
            String provider = getProvider(mBannerInfo.getProvider());
            if (!TextUtils.isEmpty(provider)) {
                title = String.format(
                        getResources().getString(R.string.title_on_provider), title, provider);
            }
            titleTextView.setText(title);
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

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);
        mBraveRewardsNativeWorker.GetPublisherBanner(
                mBraveRewardsNativeWorker.GetPublisherId(currentTabId_));
    }

    private void showCustomUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
    }

    public void sendTipButtonClick() {
        View sendTipButton = findViewById(R.id.send_tip_button);
        sendTipButton.setOnClickListener((v) -> {
            RewardsTippingPanelFragment.showTippingPanelBottomSheet(
                    this, currentTabId_, mBannerInfo.getWeb3Url());
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }
}
