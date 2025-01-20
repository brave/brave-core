/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.os.Bundle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.widget.SwitchCompat;
import androidx.core.content.ContextCompat;

import com.airbnb.lottie.LottieAnimationView;
import com.airbnb.lottie.LottieProperty;
import com.airbnb.lottie.model.KeyPath;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.Channel;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.components.browser_ui.settings.FragmentSettingsNavigation;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.List;

public class BraveNewsPreferencesV2 extends BravePreferenceFragment
        implements BraveNewsPreferencesDataListener,
                ConnectionErrorHandler,
                FragmentSettingsNavigation {
    public static final String PREF_SHOW_OPTIN = "show_optin";

    private LinearLayout mParentLayout;
    private LinearLayout mOptinLayout;
    private SwitchCompat mSwitchShowNews;
    private TextView mTvSearch;
    private TextView mTvFollowingCount;
    private Button mBtnTurnOnNews;
    private Button mBtnLearnMore;
    private View mLayoutSwitch;
    private View mDivider;
    private View mLayoutPopularSources;
    private View mLayoutSuggestions;
    private View mLayoutChannels;
    private View mLayoutFollowing;

    private boolean mIsSuggestionAvailable;
    private boolean mIsChannelAvailable;
    private boolean mIsPublisherAvailable;
    private BraveNewsController mBraveNewsController;

    // SettingsNavigation injected from main Settings Activity.
    private SettingsNavigation mSettingsLauncher;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_news_settings, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mPageTitle.set(getString(R.string.brave_news_title));

        super.onActivityCreated(savedInstanceState);

        initBraveNewsController();

        View view = getView();
        if (view != null) {
            mParentLayout = (LinearLayout) view.findViewById(R.id.layout_parent);
            mOptinLayout = (LinearLayout) view.findViewById(R.id.layout_optin_card);
            mSwitchShowNews = (SwitchCompat) view.findViewById(R.id.switch_show_news);
            mDivider = view.findViewById(R.id.divider);
            mLayoutSwitch = view.findViewById(R.id.layout_switch);
            mBtnTurnOnNews = (Button) view.findViewById(R.id.btn_turn_on_news);
            mBtnLearnMore = (Button) view.findViewById(R.id.btn_learn_more);
            mTvSearch = (TextView) view.findViewById(R.id.tv_search);
            mTvFollowingCount = (TextView) view.findViewById(R.id.tv_following_count);
            mLayoutPopularSources = (View) view.findViewById(R.id.layout_popular_sources);
            mLayoutSuggestions = (View) view.findViewById(R.id.layout_suggestions);
            mLayoutChannels = (View) view.findViewById(R.id.layout_channels);
            mLayoutFollowing = (View) view.findViewById(R.id.layout_following);

            BraveTouchUtils.ensureMinTouchTarget(mBtnTurnOnNews);
            BraveTouchUtils.ensureMinTouchTarget(mLayoutChannels);
            BraveTouchUtils.ensureMinTouchTarget(mLayoutFollowing);
            BraveTouchUtils.ensureMinTouchTarget(mLayoutPopularSources);
            BraveTouchUtils.ensureMinTouchTarget(mLayoutSuggestions);
            BraveTouchUtils.ensureMinTouchTarget(mTvSearch);

            setData();
            onClickViews();
        }
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setData() {
        if (!GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                && getView() != null) {
            LottieAnimationView lottieAnimationVIew =
                    (LottieAnimationView) getView().findViewById(R.id.animation_view);

            try {
                lottieAnimationVIew.addValueCallback(new KeyPath("newspaper", "**"),
                        LottieProperty.COLOR_FILTER,
                        frameInfo
                        -> new PorterDuffColorFilter(ContextCompat.getColor(getActivity(),
                                                             R.color.news_settings_optin_color),
                                PorterDuff.Mode.SRC_ATOP));
            } catch (Exception exception) {
                // if newspaper keypath changed in animation json
            }
        }

        if (BraveNewsUtils.getLocale() != null
                && BraveNewsUtils.getSuggestionsPublisherList().size() > 0) {
            mIsSuggestionAvailable = true;
        }

        boolean isNewsEnable = BraveNewsUtils.shouldDisplayNewsFeed();
        mSwitchShowNews.setChecked(isNewsEnable);
        onShowNewsToggle(isNewsEnable);
    }

    @Override
    public void onResume() {
        super.onResume();

        if (BraveNewsUtils.getLocale() != null && mSwitchShowNews.isChecked()) {
            updateFollowerCount();

            if (!mIsSuggestionAvailable) {
                PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
                    if (mBraveNewsController != null) {
                        BraveNewsUtils.getSuggestionsSources(mBraveNewsController, this);
                    }
                });
            }
        }
    }

    private void onClickViews() {
        mBtnTurnOnNews.setOnClickListener(view -> { mSwitchShowNews.setChecked(true); });

        mBtnLearnMore.setOnClickListener(view -> {
            CustomTabActivity.showInfoPage(getActivity(), BraveConstants.BRAVE_NEWS_LEARN_MORE_URL);
        });

        mSwitchShowNews.setOnCheckedChangeListener((compoundButton, b) -> { onShowNewsToggle(b); });

        mTvSearch.setOnClickListener(
                view -> { openBraveNewsPreferencesDetails(BraveNewsPreferencesType.Search); });

        mLayoutPopularSources.setOnClickListener(view -> {
            openBraveNewsPreferencesDetails(BraveNewsPreferencesType.PopularSources);
        });

        mLayoutSuggestions.setOnClickListener(
                view -> { openBraveNewsPreferencesDetails(BraveNewsPreferencesType.Suggestions); });

        mLayoutChannels.setOnClickListener(
                view -> {
                    openBraveNewsPreferencesDetails(BraveNewsPreferencesType.Channels);
                });

        mLayoutFollowing.setOnClickListener(
                view -> {
                    List<Publisher> followingPublisherList =
                            BraveNewsUtils.getFollowingPublisherList();
                    List<Channel> followingChannelList = BraveNewsUtils.getFollowingChannelList();
                    assert followingPublisherList != null;
                    assert followingChannelList != null;
                    if ((followingPublisherList != null && followingPublisherList.size() > 0)
                            || (followingChannelList != null && followingChannelList.size() > 0)) {
                        openBraveNewsPreferencesDetails(BraveNewsPreferencesType.Following);
                    }
                });
    }

    private void onShowNewsToggle(boolean isEnable) {
        BravePrefServiceBridge.getInstance().setShowNews(isEnable);

        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_NEWS_PREF_SHOW_NEWS, isEnable);

        FrameLayout.LayoutParams parentLayoutParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);

        if (isEnable) {
            parentLayoutParams.gravity = Gravity.NO_GRAVITY;
            mParentLayout.setLayoutParams(parentLayoutParams);
            mOptinLayout.setVisibility(View.GONE);
            mLayoutSwitch.setVisibility(View.VISIBLE);
            mDivider.setVisibility(View.VISIBLE);
            if (BraveNewsUtils.getChannelIcons().size() == 0) {
                BraveNewsUtils.setChannelIcons();
            }
            if (BraveNewsUtils.getLocale() == null && mBraveNewsController != null) {
                BraveNewsUtils.getBraveNewsSettingsData(mBraveNewsController, this);
            } else {
                mTvSearch.setVisibility(View.VISIBLE);
                mLayoutPopularSources.setVisibility(View.VISIBLE);
                mLayoutChannels.setVisibility(View.VISIBLE);
                mLayoutFollowing.setVisibility(View.VISIBLE);
                updateFollowerCount();
            }

            BravePrefServiceBridge.getInstance().setNewsOptIn(true);
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(BraveNewsPreferencesV2.PREF_SHOW_OPTIN, false);
            sharedPreferencesEditor.apply();

            if (mIsSuggestionAvailable) {
                mLayoutSuggestions.setVisibility(View.VISIBLE);
            }

        } else {
            parentLayoutParams.gravity = Gravity.CENTER_VERTICAL;
            mParentLayout.setLayoutParams(parentLayoutParams);
            mOptinLayout.setVisibility(View.VISIBLE);
            mLayoutSwitch.setVisibility(View.GONE);
            mDivider.setVisibility(View.GONE);
            mTvSearch.setVisibility(View.GONE);
            mLayoutPopularSources.setVisibility(View.GONE);
            mLayoutSuggestions.setVisibility(View.GONE);
            mLayoutChannels.setVisibility(View.GONE);
            mLayoutFollowing.setVisibility(View.GONE);
        }
    }

    private void openBraveNewsPreferencesDetails(
            BraveNewsPreferencesType braveNewsPreferencesType) {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(
                BraveConstants.BRAVE_NEWS_PREFERENCES_TYPE, braveNewsPreferencesType.toString());
        mSettingsLauncher.startSettings(
                getActivity(), BraveNewsPreferencesDetails.class, fragmentArgs);
    }

    private void initBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        mBraveNewsController =
                BraveNewsControllerFactory.getInstance().getBraveNewsController(this);
    }

    private void updateFollowerCount() {
        List<Publisher> followingPublisherList = BraveNewsUtils.getFollowingPublisherList();
        List<Channel> followingChannelList = BraveNewsUtils.getFollowingChannelList();
        int followingPublisherCount =
                followingPublisherList != null ? followingPublisherList.size() : 0;
        int followingChannelCount = followingChannelList != null ? followingChannelList.size() : 0;
        int followingCount = followingPublisherCount + followingChannelCount;
        if (mLayoutFollowing != null && mTvFollowingCount != null) {
            mTvFollowingCount.setText(String.valueOf(followingCount));
            mLayoutFollowing.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onChannelReceived() {
        if (mSwitchShowNews != null && mSwitchShowNews.isChecked()) {
            if (mLayoutChannels != null) {
                mLayoutChannels.setVisibility(View.VISIBLE);
            }

            mIsChannelAvailable = true;
            if (mIsPublisherAvailable) {
                if (mTvSearch != null) {
                    mTvSearch.setVisibility(View.VISIBLE);
                }
                updateFollowerCount();
            }
        }
    }

    @Override
    public void onPublisherReceived() {
        if (mSwitchShowNews != null && mSwitchShowNews.isChecked()) {
            if (mLayoutPopularSources != null) {
                mLayoutPopularSources.setVisibility(View.VISIBLE);
            }
            mIsPublisherAvailable = true;
            if (mIsChannelAvailable) {
                if (mTvSearch != null) {
                    mTvSearch.setVisibility(View.VISIBLE);
                }
                updateFollowerCount();
            }
        }
    }

    @Override
    public void onSuggestionsReceived() {
        if (mSwitchShowNews != null && mSwitchShowNews.isChecked()
                && BraveNewsUtils.getSuggestionsPublisherList().size() > 0) {
            if (mLayoutSuggestions != null) {
                mLayoutSuggestions.setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    public void setSettingsNavigation(SettingsNavigation settingsLauncher) {
        mSettingsLauncher = settingsLauncher;
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
        initBraveNewsController();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
    }
}
