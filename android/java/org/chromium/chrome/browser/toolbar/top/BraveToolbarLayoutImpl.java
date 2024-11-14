/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.widget.ImageViewCompat;

import com.brave.playlist.enums.PlaylistOptionsEnum;
import com.brave.playlist.listener.PlaylistOnboardingActionClickListener;
import com.brave.playlist.listener.PlaylistOptionsListener;
import com.brave.playlist.model.PlaylistOptionsModel;
import com.brave.playlist.model.SnackBarActionModel;
import com.brave.playlist.util.ConstantUtils;
import com.brave.playlist.util.PlaylistViewUtils;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.Log;
import org.chromium.base.MathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.crypto_wallet.controller.DAppsWalletController;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbar;
import org.chromium.chrome.browser.dialogs.BraveAdsSignupDialog;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.local_database.BraveStatsTable;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.SavedBandwidthTable;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.omnibox.BraveLocationBarCoordinator;
import org.chromium.chrome.browser.omnibox.LocationBarCoordinator;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.SearchActivity;
import org.chromium.chrome.browser.onboarding.v2.HighlightItem;
import org.chromium.chrome.browser.onboarding.v2.HighlightView;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettingsObserver;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.rewards.BraveRewardsPanel;
import org.chromium.chrome.browser.rewards.onboarding.RewardsOnboarding;
import org.chromium.chrome.browser.shields.BraveShieldsHandler;
import org.chromium.chrome.browser.shields.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.shields.BraveShieldsUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.chrome.browser.toolbar.home_button.HomeButton;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet.OfflineDownloader;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;
import org.chromium.ui.UiUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.interpolators.Interpolators;
import org.chromium.ui.util.ColorUtils;
import org.chromium.ui.widget.Toast;
import org.chromium.url.GURL;
import org.chromium.url.mojom.Url;

import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.function.BooleanSupplier;

public abstract class BraveToolbarLayoutImpl extends ToolbarLayout
        implements BraveToolbarLayout,
                OnClickListener,
                View.OnLongClickListener,
                BraveRewardsObserver,
                BraveRewardsNativeWorker.PublisherObserver,
                ConnectionErrorHandler,
                PlaylistServiceObserverImplDelegate {
    private static final String TAG = "BraveToolbar";

    private static final List<String> BRAVE_SEARCH_ENGINE_DEFAULT_REGIONS =
            Arrays.asList("CA", "DE", "FR", "GB", "US", "AT", "ES", "MX", "BR", "AR", "IN");
    private static final int URL_FOCUS_TOOLBAR_BUTTONS_TRANSLATION_X_DP = 10;

    private static final int DAYS_7 = 7;
    public static boolean mShouldShowPlaylistMenu;

    private PlaylistServiceObserverImpl mPlaylistServiceObserver;

    private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private ImageButton mBraveWalletButton;
    private ImageButton mBraveShieldsButton;
    private ImageButton mBraveRewardsButton;
    private HomeButton mHomeButton;
    private FrameLayout mWalletLayout;
    private FrameLayout mShieldsLayout;
    private FrameLayout mRewardsLayout;
    private BraveShieldsHandler mBraveShieldsHandler;

    // TabModelSelectorTabObserver setups observer at the ctor
    @SuppressWarnings("UnusedVariable")
    private TabModelSelectorTabObserver mTabModelSelectorTabObserver;

    // TabModelSelectorTabModelObserver setups observer at the ctor
    @SuppressWarnings("UnusedVariable")
    private TabModelSelectorTabModelObserver mTabModelSelectorTabModelObserver;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private BraveRewardsPanel mRewardsPopup;
    private DAppsWalletController mDAppsWalletController;
    private BraveShieldsContentSettings mBraveShieldsContentSettings;
    private BraveShieldsContentSettingsObserver mBraveShieldsContentSettingsObserver;
    private TextView mBraveRewardsNotificationsCount;
    private ImageView mBraveRewardsOnboardingIcon;
    private View mBraveWalletBadge;
    private ImageView mWalletIcon;
    private int mCurrentToolbarColor;

    private boolean mIsPublisherVerified;
    private boolean mIsNotificationPosted;
    private boolean mIsInitialNotificationPosted; // initial red circle notification

    private PopupWindowTooltip mShieldsPopupWindowTooltip;

    private boolean mIsBottomToolbarVisible;

    private ColorStateList mDarkModeTint;
    private ColorStateList mLightModeTint;

    // See comment at onUrlFocusChange
    // private SearchWidgetPromoPanel mSearchWidgetPromoPanel;

    private final Set<Integer> mTabsWithWalletIcon =
            Collections.synchronizedSet(new HashSet<Integer>());

    private PlaylistService mPlaylistService;

    private enum BigtechCompany { Google, Facebook, Amazon }

    public BraveToolbarLayoutImpl(Context context, AttributeSet attrs) {
        super(context, attrs);

        if (context instanceof BraveLeoActivity) {
            // Make sure initial state matches previously set flags.
            mIsBottomToolbarVisible =
                    BottomToolbarConfiguration.isBottomToolbarEnabled()
                            && BraveMenuButtonCoordinator.isMenuFromBottom();
        }
    }

    @Override
    public void destroy() {
        if (mBraveShieldsContentSettings != null) {
            mBraveShieldsContentSettings.removeObserver(mBraveShieldsContentSettingsObserver);
        }
        if (mPlaylistService != null) {
            mPlaylistService.close();
        }
        if (mPlaylistServiceObserver != null) {
            mPlaylistServiceObserver.close();
            mPlaylistServiceObserver.destroy();
            mPlaylistServiceObserver = null;
        }
        super.destroy();
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.removeObserver(this);
            mBraveRewardsNativeWorker.removePublisherObserver(this);
        }
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        if (BraveReflectionUtil.equalTypes(this.getClass(), ToolbarTablet.class)) {
            ImageButton forwardButton = findViewById(R.id.forward_button);
            if (forwardButton != null) {
                final Drawable forwardButtonDrawable =
                        UiUtils.getTintedDrawable(
                                getContext(),
                                R.drawable.btn_right_tablet,
                                R.color.default_icon_color_tint_list);
                forwardButton.setImageDrawable(forwardButtonDrawable);
            }
        }

        mWalletLayout = (FrameLayout) findViewById(R.id.brave_wallet_button_layout);
        mShieldsLayout = (FrameLayout) findViewById(R.id.brave_shields_button_layout);
        mRewardsLayout = (FrameLayout) findViewById(R.id.brave_rewards_button_layout);
        mBraveRewardsNotificationsCount = (TextView) findViewById(R.id.br_notifications_count);
        mBraveRewardsOnboardingIcon = findViewById(R.id.br_rewards_onboarding_icon);
        mBraveWalletButton = (ImageButton) findViewById(R.id.brave_wallet_button);
        mBraveShieldsButton = (ImageButton) findViewById(R.id.brave_shields_button);
        mBraveRewardsButton = (ImageButton) findViewById(R.id.brave_rewards_button);
        mHomeButton = (HomeButton) findViewById(R.id.home_button);
        mBraveWalletBadge = findViewById(R.id.wallet_notfication_badge);
        if (mWalletLayout != null) {
            mWalletIcon = mWalletLayout.findViewById(R.id.brave_wallet_button);
        }

        mDarkModeTint = ThemeUtils.getThemedToolbarIconTint(getContext(), false);
        mLightModeTint =
                ColorStateList.valueOf(ContextCompat.getColor(getContext(), R.color.brave_white));

        // See comment at onUrlFocusChange
        // mSearchWidgetPromoPanel = new SearchWidgetPromoPanel(getContext());

        if (mHomeButton != null) {
            mHomeButton.setOnLongClickListener(this);
        }

        if (mBraveShieldsButton != null) {
            mBraveShieldsButton.setClickable(true);
            mBraveShieldsButton.setOnClickListener(this);
            mBraveShieldsButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mBraveShieldsButton);
        }

        if (mBraveRewardsButton != null) {
            mBraveRewardsButton.setClickable(true);
            mBraveRewardsButton.setOnClickListener(this);
            mBraveRewardsButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mBraveRewardsButton);
        }

        if (mBraveWalletButton != null) {
            mBraveWalletButton.setClickable(true);
            mBraveWalletButton.setOnClickListener(this);
            mBraveWalletButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mBraveWalletButton);
        }

        mBraveShieldsHandler = new BraveShieldsHandler(getContext());
        mBraveShieldsHandler.addObserver(
                new BraveShieldsMenuObserver() {
                    @Override
                    public void onMenuTopShieldsChanged(boolean isOn, boolean isTopShield) {
                        Tab currentTab = getToolbarDataProvider().getTab();
                        if (currentTab == null) {
                            return;
                        }
                        if (isTopShield) {
                            updateBraveShieldsButtonState(currentTab);
                        }
                        if (currentTab.isLoading()) {
                            currentTab.stopLoading();
                        }
                        currentTab.reloadIgnoringCache();
                        if (null != mBraveShieldsHandler) {
                            // Clean the Bravery Panel
                            mBraveShieldsHandler.updateValues(0, 0, 0);
                        }
                    }
                });
        mBraveShieldsContentSettingsObserver = new BraveShieldsContentSettingsObserver() {
            @Override
            public void blockEvent(int tabId, String blockType, String subresource) {
                mBraveShieldsHandler.addStat(tabId, blockType, subresource);
                Tab currentTab = getToolbarDataProvider().getTab();
                if (currentTab == null || currentTab.getId() != tabId) {
                    return;
                }
                mBraveShieldsHandler.updateValues(tabId);
                if (!isIncognito() && OnboardingPrefManager.getInstance().isBraveStatsEnabled()
                        && (blockType.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS)
                                || blockType.equals(BraveShieldsContentSettings
                                                            .RESOURCE_IDENTIFIER_TRACKERS))) {
                    addStatsToDb(blockType, subresource, currentTab.getUrl().getSpec());
                }
            }

            @Override
            public void savedBandwidth(long savings) {
                if (!isIncognito() && OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                    addSavedBandwidthToDb(savings);
                }
            }
        };
        // Initially show shields off image. Shields button state will be updated when tab is
        // shown and loading state is changed.
        updateBraveShieldsButtonState(null);
        if (BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)) {
            if (getMenuButtonCoordinator() != null && isMenuButtonOnBottom()) {
                getMenuButtonCoordinator().setVisibility(false);
            }
        }

        if (BraveReflectionUtil.equalTypes(this.getClass(), CustomTabToolbar.class)) {
            LinearLayout customActionButtons = findViewById(R.id.action_buttons);
            assert customActionButtons != null : "Something has changed in the upstream!";
            if (customActionButtons != null && mBraveShieldsButton != null) {
                ViewGroup.MarginLayoutParams braveShieldsButtonLayout =
                        (ViewGroup.MarginLayoutParams) mBraveShieldsButton.getLayoutParams();
                ViewGroup.MarginLayoutParams actionButtonsLayout =
                        (ViewGroup.MarginLayoutParams) customActionButtons.getLayoutParams();
                actionButtonsLayout.setMarginEnd(actionButtonsLayout.getMarginEnd()
                        + braveShieldsButtonLayout.getMarginEnd());
                customActionButtons.setLayoutParams(actionButtonsLayout);
            }
        }
        updateShieldsLayoutBackground(isIncognito() || !NtpUtil.shouldShowRewardsIcon());
    }

    public String getLocationBarQuery() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            String query =
                    ((BraveLocationBarCoordinator) getLocationBar())
                            .getUrlBarTextWithoutAutocomplete();
            return query;
        }
        return "";
    }

    public void clearOmniboxFocus() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            ((BraveLocationBarCoordinator) getLocationBar()).clearOmniboxFocus();
        }
    }

    public boolean isUrlBarFocused() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            return ((BraveLocationBarCoordinator) getLocationBar()).isUrlBarFocused();
        }
        return false;
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (isPlaylistEnabledByPrefsAndFlags()) {
            mPlaylistService = null;
            initPlaylistService();
        }
    }

    private void initPlaylistService() {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (mPlaylistService != null || currentTab == null) {
            return;
        }

        if (currentTab.isIncognito()) {
            return;
        }

        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance()
                        .getPlaylistService(
                                Profile.fromWebContents(currentTab.getWebContents()), this);
    }

    @Override
    public void onTermsOfServiceUpdateAccepted() {
        showOrHideRewardsBadge(false);
    }

    private void showOrHideRewardsBadge(boolean shouldShow) {
        View rewardsBadge = findViewById(R.id.rewards_notfication_badge);
        rewardsBadge.setVisibility(shouldShow ? View.VISIBLE : View.GONE);
    }

    @Override
    protected void onNativeLibraryReady() {
        super.onNativeLibraryReady();
        if (isPlaylistEnabledByPrefsAndFlags()) {
            initPlaylistService();
            mPlaylistServiceObserver = new PlaylistServiceObserverImpl(this);
            mPlaylistService.addObserver(mPlaylistServiceObserver);
        }

        mBraveShieldsContentSettings = BraveShieldsContentSettings.getInstance();
        mBraveShieldsContentSettings.addObserver(mBraveShieldsContentSettingsObserver);

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.isSupported()
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()
                && NtpUtil.shouldShowRewardsIcon()
                && mRewardsLayout != null) {
            mRewardsLayout.setVisibility(View.VISIBLE);
        }
        if (mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.isRewardsEnabled()
                && mBraveRewardsNativeWorker.isSupported()
                && mBraveRewardsNativeWorker.isTermsOfServiceUpdateRequired()) {
            showOrHideRewardsBadge(true);
        }
        if (mShieldsLayout != null) {
            updateShieldsLayoutBackground(
                    !(mRewardsLayout != null && mRewardsLayout.getVisibility() == View.VISIBLE));
            mShieldsLayout.setVisibility(View.VISIBLE);
        }
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.addObserver(this);
            mBraveRewardsNativeWorker.addPublisherObserver(this);
            mBraveRewardsNativeWorker.triggerOnNotifyFrontTabUrlChanged();
            mBraveRewardsNativeWorker.getAllNotifications();
        }
    }

    public void setTabModelSelector(TabModelSelector selector) {
        // We might miss events before calling setTabModelSelector, so we need
        // to proactively update the shields button state here, otherwise shields
        // might sometimes show as disabled while it is actually enabled.
        updateBraveShieldsButtonState(getToolbarDataProvider().getTab());
        mTabModelSelectorTabObserver =
                new TabModelSelectorTabObserver(selector) {
                    @Override
                    protected void onTabRegistered(Tab tab) {
                        super.onTabRegistered(tab);
                        if (tab.isIncognito()) {
                            showWalletIcon(false);
                        }
                    }

                    @Override
                    public void onShown(Tab tab, @TabSelectionType int type) {
                        // Update shields button state when visible tab is changed.
                        updateBraveShieldsButtonState(tab);
                        // case when window.open is triggered from dapps site and new tab is in
                        // focus
                        if (type != TabSelectionType.FROM_USER) {
                            dismissWalletPanelOrDialog();
                        }
                        findMediaFiles();
                    }

                    @Override
                    public void onHidden(Tab tab, @TabHidingType int reason) {
                        hidePlaylistButton();
                    }

                    @Override
                    public void onPageLoadStarted(Tab tab, GURL url) {
                        showWalletIcon(false, tab);
                        if (getToolbarDataProvider().getTab() == tab) {
                            updateBraveShieldsButtonState(tab);
                        }
                        mBraveShieldsHandler.clearBraveShieldsCount(tab.getId());
                        dismissShieldsTooltip();
                        hidePlaylistButton();
                    }

                    @Override
                    public void onPageLoadFinished(final Tab tab, GURL url) {
                        if (getToolbarDataProvider().getTab() == tab) {
                            mBraveShieldsHandler.updateUrlSpec(url.getSpec());
                            updateBraveShieldsButtonState(tab);

                            if (mBraveShieldsButton != null
                                    && mBraveShieldsButton.isShown()
                                    && mBraveShieldsHandler != null
                                    && !mBraveShieldsHandler.isShowing()) {
                                checkForTooltip(tab);
                            }
                        }

                        String countryCode = Locale.getDefault().getCountry();
                        if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                                && url.domainIs(BraveConstants.YOUTUBE_DOMAIN)
                                && ChromeSharedPreferences.getInstance()
                                        .readBoolean(
                                                BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG,
                                                true)) {
                            ChromeSharedPreferences.getInstance()
                                    .writeBoolean(BravePreferenceKeys.BRAVE_OPENED_YOUTUBE, true);
                        }
                    }

                    @Override
                    public void onDidFinishNavigationInPrimaryMainFrame(
                            Tab tab, NavigationHandle navigation) {
                        if (getToolbarDataProvider().getTab() == tab
                                && mBraveRewardsNativeWorker != null
                                && !tab.isIncognito()) {
                            mBraveRewardsNativeWorker.onNotifyFrontTabUrlChanged(
                                    tab.getId(), tab.getUrl().getSpec());
                        }
                        if (PackageUtils.isFirstInstall(getContext())
                                && tab.getUrl().getSpec() != null
                                && tab.getUrl()
                                        .getSpec()
                                        .equals(BraveActivity.BRAVE_REWARDS_SETTINGS_URL)
                                && BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()
                                && mBraveRewardsNativeWorker != null
                                && !mBraveRewardsNativeWorker.isRewardsEnabled()
                                && mBraveRewardsNativeWorker.isSupported()) {
                            showOnBoarding();
                        }
                    }

                    @Override
                    public void onDestroyed(Tab tab) {
                        // Remove references for the ads from the Database. Tab is destroyed, they
                        // are not
                        // needed anymore.
                        new Thread() {
                            @Override
                            public void run() {
                                mDatabaseHelper.deleteDisplayAdsFromTab(tab.getId());
                            }
                        }.start();
                        mBraveShieldsHandler.removeStat(tab.getId());
                        mTabsWithWalletIcon.remove(tab.getId());
                    }
                };

        mTabModelSelectorTabModelObserver =
                new TabModelSelectorTabModelObserver(selector) {
                    @Override
                    public void didSelectTab(Tab tab, @TabSelectionType int type, int lastId) {
                        if (mBraveRewardsNativeWorker != null && !tab.isIncognito()) {
                            mBraveRewardsNativeWorker.onNotifyFrontTabUrlChanged(
                                    tab.getId(), tab.getUrl().getSpec());
                            Tab providerTab = getToolbarDataProvider().getTab();
                            if (providerTab != null && providerTab.getId() == tab.getId()) {
                                showWalletIcon(mTabsWithWalletIcon.contains(tab.getId()));
                            } else if (mWalletLayout != null) {
                                mWalletLayout.setVisibility(
                                        mTabsWithWalletIcon.contains(tab.getId())
                                                ? View.VISIBLE
                                                : View.GONE);
                            }
                        }
                    }
                };
    }

    private void showOnBoarding() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            int deviceWidth = ConfigurationUtils.getDisplayMetrics(activity).get("width");
            boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity);
            deviceWidth = (int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95));
            RewardsOnboarding panel = new RewardsOnboarding(mBraveRewardsButton, deviceWidth);
            panel.showLikePopDownMenu();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "RewardsOnboarding failed " + e);
        }
    }

    private boolean isPlaylistEnabledByPrefsAndFlags() {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (currentTab == null) {
            return false;
        }
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)
                && !currentTab.isIncognito();
    }

    private void hidePlaylistButton() {
        mShouldShowPlaylistMenu = false;
        try {
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity().getWindow().getDecorView().findViewById(
                            android.R.id.content);
            View playlistButton = viewGroup.findViewById(R.id.playlist_button_id);
            if (playlistButton != null && playlistButton.getVisibility() == View.VISIBLE) {
                playlistButton.setVisibility(View.GONE);
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "hidePlaylistButton " + e);
        }
    }

    private boolean isPlaylistButtonVisible() {
        try {
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity().getWindow().getDecorView().findViewById(
                            android.R.id.content);
            View playlistButton = viewGroup.findViewById(R.id.playlist_button_id);
            return playlistButton != null && playlistButton.getVisibility() == View.VISIBLE;
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "isPlaylistButtonVisible " + e);
            return false;
        }
    }

    private void findMediaFiles() {
        if (mPlaylistService != null && isPlaylistEnabledByPrefsAndFlags()) {
            hidePlaylistButton();
            mPlaylistService.findMediaFilesFromActiveTab();
        }
    }

    private void showPlaylistButton(PlaylistItem[] items) {
        try {
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity().getWindow().getDecorView().findViewById(
                            android.R.id.content);

            PlaylistOptionsListener playlistOptionsListener =
                    new PlaylistOptionsListener() {
                        @Override
                        public void onPlaylistOptionClicked(
                                PlaylistOptionsModel playlistOptionsModel) {
                            try {
                                if (playlistOptionsModel.getOptionType()
                                        == PlaylistOptionsEnum.ADD_MEDIA) {
                                    addMediaToPlaylist(items);
                                } else if (playlistOptionsModel.getOptionType()
                                        == PlaylistOptionsEnum.OPEN_PLAYLIST) {
                                    BraveActivity.getBraveActivity()
                                            .openPlaylistActivity(
                                                    getContext(), ConstantUtils.DEFAULT_PLAYLIST);
                                } else if (playlistOptionsModel.getOptionType()
                                        == PlaylistOptionsEnum.PLAYLIST_SETTINGS) {
                                    BraveActivity.getBraveActivity().openBravePlaylistSettings();
                                }
                            } catch (BraveActivity.BraveActivityNotFoundException e) {
                                Log.e(TAG, "showPlaylistButton onOptionClicked " + e);
                            }
                        }
                    };
            if (!isPlaylistButtonVisible()) {
                PlaylistOnboardingActionClickListener playlistOnboardingActionClickListener =
                        new PlaylistOnboardingActionClickListener() {
                            @Override
                            public void onOnboardingActionClick() {
                                addMediaToPlaylist(items);
                            }
                        };

                PlaylistViewUtils.showPlaylistButton(
                        BraveActivity.getBraveActivity(),
                        viewGroup,
                        playlistOptionsListener,
                        playlistOnboardingActionClickListener);
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showPlaylistButton " + e);
        }
    }

    private void addMediaToPlaylist(PlaylistItem[] items) {
        if (mPlaylistService == null) {
            return;
        }
        mPlaylistService.getPlaylist(
                ConstantUtils.DEFAULT_PLAYLIST,
                defaultPlaylist -> {
                    Set<String> pageSources = new HashSet<String>();
                    for (PlaylistItem defaultPlaylistItem : defaultPlaylist.items) {
                        pageSources.add(defaultPlaylistItem.pageSource.url);
                    }
                    List<PlaylistItem> playlistItems = new ArrayList();
                    for (PlaylistItem playlistItem : items) {
                        // Check for duplicates in default playlist
                        if (!pageSources.contains(playlistItem.pageSource.url)) {
                            playlistItems.add(playlistItem);
                        }
                    }
                    if (playlistItems.size() > 0) {
                        mPlaylistService.addMediaFiles(
                                playlistItems.toArray(new PlaylistItem[0]),
                                ConstantUtils.DEFAULT_PLAYLIST,
                                true,
                                addedItems -> {
                                    if (addedItems.length > 0) {
                                        showAddedToPlaylistSnackBar();
                                    }
                                });
                    } else {
                        showAlreadyAddedToPlaylistSnackBar();
                    }
                });
    }

    public void addMediaToPlaylist() {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (mPlaylistService == null || currentTab == null) {
            return;
        }
        mPlaylistService.addMediaFilesFromActiveTabToPlaylist(
                ConstantUtils.DEFAULT_PLAYLIST,
                true,
                addedItems -> {
                    if (addedItems.length > 0) {
                        showAddedToPlaylistSnackBar();
                    }
                });
    }

    private void showAddedToPlaylistSnackBar() {
        SnackBarActionModel snackBarActionModel =
                new SnackBarActionModel(
                        getContext().getResources().getString(R.string.view_action),
                        new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                try {
                                    BraveActivity.getBraveActivity()
                                            .openPlaylistActivity(
                                                    getContext(), ConstantUtils.DEFAULT_PLAYLIST);
                                } catch (BraveActivity.BraveActivityNotFoundException e) {
                                    Log.e(TAG, "showAddedToPlaylistSnackBar onClick ", e);
                                }
                            }
                        });
        try {
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity().getWindow().getDecorView().findViewById(
                            android.R.id.content);
            String playlistName =
                    getContext().getResources().getString(R.string.playlist_play_later);
            PlaylistViewUtils.showSnackBarWithActions(
                    viewGroup,
                    String.format(
                            getContext().getResources().getString(R.string.added_to_playlist),
                            playlistName),
                    snackBarActionModel);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showAddedToPlaylistSnackBar ", e);
        }
    }

    private void showAlreadyAddedToPlaylistSnackBar() {
        SnackBarActionModel snackBarActionModel =
                new SnackBarActionModel(
                        getContext().getResources().getString(R.string.close_text),
                        new View.OnClickListener() {
                            @Override
                            public void onClick(View v) {
                                // Do nothing
                            }
                        });
        try {
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity()
                            .getWindow()
                            .getDecorView()
                            .findViewById(android.R.id.content);
            PlaylistViewUtils.showSnackBarWithActions(
                    viewGroup,
                    getContext().getResources().getString(R.string.already_added_in_playlist),
                    snackBarActionModel);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showAlreadyAddedToPlaylistSnackBar " + e);
        }
    }

    private void checkForTooltip(Tab tab) {
        try {
            if (!BraveShieldsUtils.isTooltipShown
                    && !BraveActivity.getBraveActivity().mIsDeepLink) {
                if (!BraveShieldsUtils.hasShieldsTooltipShown(
                            BraveShieldsUtils.PREF_SHIELDS_TOOLTIP)
                        && mBraveShieldsHandler.getTrackersBlockedCount(tab.getId())
                                        + mBraveShieldsHandler.getAdsBlockedCount(tab.getId())
                                > 0) {
                    showTooltip(BraveShieldsUtils.PREF_SHIELDS_TOOLTIP, tab.getId());
                }
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "checkForTooltip " + e);
        }
    }

    private void showTooltip(String tooltipPref, int tabId) {
        try {
            HighlightView highlightView = new HighlightView(getContext(), null);
            highlightView.setColor(ContextCompat.getColor(
                    getContext(), R.color.onboarding_search_highlight_color));
            ViewGroup viewGroup =
                    BraveActivity.getBraveActivity().getWindow().getDecorView().findViewById(
                            android.R.id.content);
            float padding = (float) dpToPx(getContext(), 20);
            mShieldsPopupWindowTooltip =
                    new PopupWindowTooltip.Builder(getContext())
                            .anchorView(mBraveShieldsButton)
                            .arrowColor(ContextCompat.getColor(
                                    getContext(), R.color.onboarding_arrow_color))
                            .gravity(Gravity.BOTTOM)
                            .dismissOnOutsideTouch(true)
                            .dismissOnInsideTouch(false)
                            .backgroundDimDisabled(true)
                            .padding(padding)
                            .parentPaddingHorizontal(dpToPx(getContext(), 10))
                            .modal(true)
                            .onDismissListener(tooltip -> {
                                if (viewGroup != null && highlightView != null) {
                                    highlightView.stopAnimation();
                                    viewGroup.removeView(highlightView);
                                }
                            })
                            .contentView(R.layout.brave_shields_tooltip_layout)
                            .build();

            ArrayList<String> blockerNamesList = mBraveShieldsHandler.getBlockerNamesList(tabId);

            int adsTrackersCount = mBraveShieldsHandler.getTrackersBlockedCount(tabId)
                    + mBraveShieldsHandler.getAdsBlockedCount(tabId);

            String displayTrackerName = "";
            if (blockerNamesList.contains(BigtechCompany.Google.name())) {
                displayTrackerName = BigtechCompany.Google.name();
            } else if (blockerNamesList.contains(BigtechCompany.Facebook.name())) {
                displayTrackerName = BigtechCompany.Facebook.name();
            } else if (blockerNamesList.contains(BigtechCompany.Amazon.name())) {
                displayTrackerName = BigtechCompany.Amazon.name();
            }

            String trackerText = "";
            if (!displayTrackerName.isEmpty()) {
                if (adsTrackersCount - 1 == 0) {
                    trackerText =
                            String.format(getContext().getResources().getString(
                                                  R.string.shield_bigtech_tracker_only_blocked),
                                    displayTrackerName);

                } else {
                    trackerText = String.format(getContext().getResources().getString(
                                                        R.string.shield_bigtech_tracker_blocked),
                            displayTrackerName, String.valueOf(adsTrackersCount - 1));
                }
            } else {
                trackerText = String.format(
                        getContext().getResources().getString(R.string.shield_tracker_blocked),
                        String.valueOf(adsTrackersCount));
            }

            TextView tvBlocked = mShieldsPopupWindowTooltip.findViewById(R.id.tv_blocked);
            tvBlocked.setText(trackerText);

            if (mBraveShieldsButton != null && mBraveShieldsButton.isShown()) {
                viewGroup.addView(highlightView);
                HighlightItem item = new HighlightItem(mBraveShieldsButton);

                ImageButton braveShieldButton =
                        new ImageButton(getContext(), null, R.style.ToolbarButton);
                braveShieldButton.setImageResource(R.drawable.btn_brave);
                FrameLayout.LayoutParams braveShieldParams =
                        new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT,
                                FrameLayout.LayoutParams.WRAP_CONTENT);

                int[] location = new int[2];
                highlightView.getLocationOnScreen(location);
                braveShieldParams.leftMargin = item.getScreenLeft() + dpToPx(getContext(), 10);
                braveShieldParams.topMargin = item.getScreenTop()
                        + ((item.getScreenBottom() - item.getScreenTop()) / 4) - location[1];
                braveShieldButton.setLayoutParams(braveShieldParams);
                highlightView.addView(braveShieldButton);

                highlightView.setShouldShowHighlight(true);
                highlightView.setHighlightTransparent(true);
                highlightView.setHighlightItem(item);
                highlightView.initializeAnimators();
                highlightView.startAnimation();

                mShieldsPopupWindowTooltip.show();
                BraveShieldsUtils.setShieldsTooltipShown(tooltipPref, true);
                BraveShieldsUtils.isTooltipShown = true;
            }

        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showTooltip " + e);
        }
    }

    public void dismissShieldsTooltip() {
        if (mShieldsPopupWindowTooltip != null && mShieldsPopupWindowTooltip.isShowing()) {
            mShieldsPopupWindowTooltip.dismiss();
            mShieldsPopupWindowTooltip = null;
        }
    }

    public void reopenShieldsPanel() {
        if (mBraveShieldsHandler != null && mBraveShieldsHandler.isShowing()) {
            mBraveShieldsHandler.hideBraveShieldsMenu();
            showShieldsMenu(mBraveShieldsButton);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        dismissShieldsTooltip();
        reopenShieldsPanel();
        // TODO: show wallet panel
    }

    private void showBraveRewardsOnboardingModal() {
        Context context = getContext();
        final Dialog dialog = new Dialog(context);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.setCancelable(false);
        dialog.setContentView(R.layout.brave_rewards_onboarding_modal);
        dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);

        View braveRewardsOnboardingModalView =
                dialog.findViewById(R.id.brave_rewards_onboarding_modal_layout);

        String tosText =
                String.format(context.getResources().getString(R.string.brave_rewards_tos_text),
                        context.getResources().getString(R.string.terms_of_service),
                        context.getResources().getString(R.string.privacy_policy));
        int termsOfServiceIndex =
                tosText.indexOf(context.getResources().getString(R.string.terms_of_service));
        Spanned tosTextSpanned = BraveRewardsHelper.spannedFromHtmlString(tosText);
        SpannableString tosTextSS = new SpannableString(tosTextSpanned.toString());

        ClickableSpan tosClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(context, BraveActivity.BRAVE_TERMS_PAGE);
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        tosTextSS.setSpan(
                tosClickableSpan,
                termsOfServiceIndex,
                termsOfServiceIndex
                        + context.getResources().getString(R.string.terms_of_service).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(
                new ForegroundColorSpan(context.getColor(R.color.brave_rewards_modal_theme_color)),
                termsOfServiceIndex,
                termsOfServiceIndex
                        + context.getResources().getString(R.string.terms_of_service).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        ClickableSpan privacyProtectionClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(@NonNull View textView) {
                CustomTabActivity.showInfoPage(context, BraveActivity.BRAVE_PRIVACY_POLICY);
            }
            @Override
            public void updateDrawState(@NonNull TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(false);
            }
        };

        int privacyPolicyIndex =
                tosText.indexOf(context.getResources().getString(R.string.privacy_policy));
        tosTextSS.setSpan(privacyProtectionClickableSpan, privacyPolicyIndex,
                privacyPolicyIndex
                        + context.getResources().getString(R.string.privacy_policy).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(
                new ForegroundColorSpan(context.getColor(R.color.brave_rewards_modal_theme_color)),
                privacyPolicyIndex,
                privacyPolicyIndex
                        + context.getResources().getString(R.string.privacy_policy).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        TextView tosAndPpText = braveRewardsOnboardingModalView.findViewById(
                R.id.brave_rewards_onboarding_modal_tos_pp_text);
        tosAndPpText.setMovementMethod(LinkMovementMethod.getInstance());
        tosAndPpText.setText(tosTextSS);
        BraveTouchUtils.ensureMinTouchTarget(tosAndPpText);

        TextView takeQuickTourButton =
                braveRewardsOnboardingModalView.findViewById(R.id.take_quick_tour_button);
        takeQuickTourButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        BraveRewardsHelper.setShowBraveRewardsOnboardingOnce(true);
                        openRewardsPanel();
                        dialog.dismiss();
                    }
                });
        BraveTouchUtils.ensureMinTouchTarget(takeQuickTourButton);
        TextView btnBraveRewards =
                braveRewardsOnboardingModalView.findViewById(R.id.start_using_brave_rewards_text);
        btnBraveRewards.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        BraveRewardsHelper.setShowDeclareGeoModal(true);
                        openRewardsPanel();
                        dialog.dismiss();
                    }
                });

        dialog.show();
    }

    private void addSavedBandwidthToDb(long savings) {
        new AsyncTask<Void>() {
            @Override
            protected Void doInBackground() {
                try {
                    SavedBandwidthTable savedBandwidthTable =
                            new SavedBandwidthTable(
                                    savings, BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                    long unused_rowId = mDatabaseHelper.insertSavedBandwidth(savedBandwidthTable);
                } catch (Exception e) {
                    // Do nothing if url is invalid.
                    // Just return w/o showing shields popup.
                    return null;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void addStatsToDb(String statType, String statSite, String url) {
        new AsyncTask<Void>() {
            @Override
            protected Void doInBackground() {
                try {
                    URL urlObject = new URL(url);
                    URL siteObject = new URL(statSite);
                    BraveStatsTable braveStatsTable =
                            new BraveStatsTable(
                                    url,
                                    urlObject.getHost(),
                                    statType,
                                    statSite,
                                    siteObject.getHost(),
                                    BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                    long unused_rowId = mDatabaseHelper.insertStats(braveStatsTable);
                } catch (Exception e) {
                    // Do nothing if url is invalid.
                    // Just return w/o showing shields popup.
                    return null;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public boolean isWalletIconVisible() {
        if (mWalletLayout == null) {
            return false;
        }
        return mWalletLayout.getVisibility() == View.VISIBLE;
    }

    public void showWalletIcon(boolean show, Tab tab) {
        // The layout could be null in Custom Tabs layout
        if (mWalletLayout == null) {
            return;
        }
        Tab currentTab = tab;
        if (currentTab == null) {
            currentTab = getToolbarDataProvider().getTab();
            if (currentTab == null) {
                return;
            }
        }
        if (show) {
            mWalletLayout.setVisibility(View.VISIBLE);
            mTabsWithWalletIcon.add(currentTab.getId());
        } else {
            mWalletLayout.setVisibility(View.GONE);
            mTabsWithWalletIcon.remove(currentTab.getId());
        }
    }

    public void showWalletIcon(boolean show) {
        showWalletIcon(show, null);
    }

    public void hideRewardsOnboardingIcon() {
        if (mBraveRewardsOnboardingIcon != null) {
            mBraveRewardsOnboardingIcon.setVisibility(View.GONE);
        }
        if (mBraveRewardsNotificationsCount != null) {
            mBraveRewardsNotificationsCount.setVisibility(View.GONE);
        }
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BraveRewardsPanel.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, true);
    }

    @Override
    public void onClickImpl(View v) {
        if (mBraveShieldsHandler == null) {
            assert false;
            return;
        }
        if (mBraveShieldsButton == v && mBraveShieldsButton != null) {
            showShieldsMenu(mBraveShieldsButton);
        } else if (mBraveRewardsButton == v && mBraveRewardsButton != null) {
            if (null != mRewardsPopup) {
                return;
            }
            hideRewardsOnboardingIcon();
            OnboardingPrefManager.getInstance().setOnboardingShown(true);
            mRewardsPopup = new BraveRewardsPanel(v);
            mRewardsPopup.showLikePopDownMenu();
            if (mBraveRewardsNotificationsCount.isShown()) {
                ChromeSharedPreferences.getInstance()
                        .writeBoolean(
                                BraveRewardsPanel.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, true);
                mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
                mIsInitialNotificationPosted = false;
            }
        } else if (mHomeButton == v) {
            // Helps Brave News know how to behave on home button action
            try {
                BraveActivity.getBraveActivity().setComesFromNewTab(true);
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "HomeButton click " + e);
            }
        } else if (mBraveWalletButton == v && mBraveWalletButton != null) {
            maybeShowWalletPanel();
        }
    }

    private void maybeShowWalletPanel() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showWalletPanel(true);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "maybeShowWalletPanel " + e);
        }
    }

    private void showWalletPanelInternal(View v) {
        mDAppsWalletController =
                new DAppsWalletController(getContext(), v, dialog -> mDAppsWalletController = null);
        mDAppsWalletController.showWalletPanel();
    }

    public void showWalletPanel() {
        dismissWalletPanelOrDialog();
        showWalletPanelInternal(this);
    }

    @Override
    public void onClick(View v) {
        onClickImpl(v);
    }

    private boolean checkForRewardsOnboarding() {
        return PackageUtils.isFirstInstall(getContext())
                && mBraveRewardsNativeWorker != null
                && !mBraveRewardsNativeWorker.isRewardsEnabled()
                && mBraveRewardsNativeWorker.isSupported()
                && !OnboardingPrefManager.getInstance().isOnboardingShown()
                && (BraveRewardsHelper.getRewardsOnboardingIconInvisibleTiming() == 0
                        || (BraveRewardsHelper.getRewardsOnboardingIconInvisibleTiming() > 0
                                && System.currentTimeMillis()
                                        <= BraveRewardsHelper
                                                .getRewardsOnboardingIconInvisibleTiming()));
    }

    private void showShieldsMenu(View mBraveShieldsButton) {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (currentTab == null) {
            return;
        }
        try {
            URL url = new URL(currentTab.getUrl().getSpec());
            // Don't show shields popup if protocol is not valid for shields.
            if (!isValidProtocolForShields(url.getProtocol())) {
                return;
            }
            mBraveShieldsHandler.show(mBraveShieldsButton, currentTab);
        } catch (Exception e) {
            // Do nothing if url is invalid.
            // Just return w/o showing shields popup.
            return;
        }
    }

    @Override
    public boolean onLongClickImpl(View v) {
        // Use null as the default description since Toast.showAnchoredToast
        // will return false if it is null.
        String description = null;
        Context context = getContext();
        Resources resources = context.getResources();

        if (v == mBraveShieldsButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_shields);
        } else if (v == mBraveRewardsButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_rewards);
        } else if (v == mHomeButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_home);
        } else if (v == mBraveWalletButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_wallet);
        }

        return Toast.showAnchoredToast(context, v, description);
    }

    @Override
    public boolean onLongClick(View v) {
        return onLongClickImpl(v);
    }

    @Override
    public void onUrlFocusChange(boolean hasFocus) {
        Log.e("onUrlFocusChange", "hasFoucs : " + hasFocus);
        Context context = getContext();
        String countryCode = Locale.getDefault().getCountry();
        try {
            BraveActivity braveActivity = BraveActivity.getBraveActivity();
            if (hasFocus
                    && PackageUtils.isFirstInstall(context)
                    && braveActivity.getActivityTab() != null
                    && UrlUtilities.isNtpUrl(braveActivity.getActivityTab().getUrl().getSpec())
                    && !OnboardingPrefManager.getInstance().hasSearchEngineOnboardingShown()
                    && OnboardingPrefManager.getInstance().getUrlFocusCount() == 1
                    && !BRAVE_SEARCH_ENGINE_DEFAULT_REGIONS.contains(countryCode)) {
                Intent searchActivityIntent = new Intent(context, SearchActivity.class);
                searchActivityIntent.setAction(Intent.ACTION_VIEW);
                context.startActivity(searchActivityIntent);
            }

            // if (hasFocus) {
            //     Log.e("onUrlFocusChange", "getLocationBarQuery() : "+getLocationBarQuery());

            //     View rootView =
            //         braveActivity.getWindow().getDecorView().findViewById(
            //                 android.R.id.content);

            //     rootView.getViewTreeObserver()
            //             .addOnGlobalLayoutListener(
            //                     new ViewTreeObserver.OnGlobalLayoutListener() {
            //                         @Override
            //                         public void onGlobalLayout() {
            //                             Rect r = new Rect();
            //                             rootView.getWindowVisibleDisplayFrame(r);
            //                             int screenHeight = rootView.getRootView().getHeight();
            //                             int visibleHeight = r.bottom;
            //                             int heightDifference = screenHeight - visibleHeight;

            //                             boolean keyboardVisible = heightDifference > screenHeight
            // * 0.15;
            //                             if (keyboardVisible) {
            //
            // braveActivity.showQuickActionSearchEnginesView(heightDifference);
            //                             } else {
            //                                 braveActivity.removeQuickActionSearchEnginesView();
            //                             }
            //                         }
            //                     });
            // } else {
            //     braveActivity.removeQuickActionSearchEnginesView();
            // }
            // if (!hasFocus) {
            //     braveActivity.removeQuickActionSearchEnginesView();
            // }

        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onUrlFocusChange " + e);
        }

        // We need to enable the promo for later release.
        // Delay showing the panel. Otherwise there are ANRs on holding onUrlFocusChange
        /* PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            int appOpenCountForWidgetPromo = ChromeSharedPreferences.getInstance().readInt(
                    BravePreferenceKeys.BRAVE_APP_OPEN_COUNT_FOR_WIDGET_PROMO);
            if (hasFocus
                    && appOpenCountForWidgetPromo >= BraveActivity.APP_OPEN_COUNT_FOR_WIDGET_PROMO)
                mSearchWidgetPromoPanel.showIfNeeded(this);
        }); */

        if (OnboardingPrefManager.getInstance().getUrlFocusCount() == 0) {
            OnboardingPrefManager.getInstance().updateUrlFocusCount();
        }
        super.onUrlFocusChange(hasFocus);
    }

    @Override
    public void populateUrlAnimatorSetImpl(boolean showExpandedState,
            int urlFocusToolbarButtonsDuration, int urlClearFocusTabStackDelayMs,
            List<Animator> animators) {
        if (mBraveShieldsButton != null) {
            Animator animator;
            if (showExpandedState) {
                float density = getContext().getResources().getDisplayMetrics().density;
                boolean isRtl = getLayoutDirection() == LAYOUT_DIRECTION_RTL;
                float toolbarButtonTranslationX =
                        MathUtils.flipSignIf(URL_FOCUS_TOOLBAR_BUTTONS_TRANSLATION_X_DP, isRtl)
                        * density;
                animator = ObjectAnimator.ofFloat(
                        mBraveShieldsButton, TRANSLATION_X, toolbarButtonTranslationX);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setInterpolator(Interpolators.FAST_OUT_LINEAR_IN_INTERPOLATOR);
                animators.add(animator);

                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 0);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setInterpolator(Interpolators.FAST_OUT_LINEAR_IN_INTERPOLATOR);
                animators.add(animator);
            } else {
                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, TRANSLATION_X, 0);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setStartDelay(urlClearFocusTabStackDelayMs);
                animator.setInterpolator(Interpolators.FAST_OUT_SLOW_IN_INTERPOLATOR);
                animators.add(animator);

                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 1);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setStartDelay(urlClearFocusTabStackDelayMs);
                animator.setInterpolator(Interpolators.FAST_OUT_SLOW_IN_INTERPOLATOR);
                animators.add(animator);
            }
        }
    }

    @Override
    public void updateModernLocationBarColorImpl(int color) {
        mCurrentToolbarColor = color;
        if (mShieldsLayout != null) {
            mShieldsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
        if (mRewardsLayout != null) {
            mRewardsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
        if (mWalletLayout != null) {
            mWalletLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
    }

    /**
     * If |tab| is null, set disabled image to shields button and |urlString| is ignored. If
     * |urlString| is null, url is fetched from |tab|.
     */
    public void updateBraveShieldsButtonState(Tab tab) {
        if (mBraveShieldsButton == null) {
            assert false;
            return;
        }

        if (tab == null) {
            mBraveShieldsButton.setImageResource(R.drawable.btn_brave_off);
            return;
        }
        mBraveShieldsButton.setImageResource(
                isShieldsOnForTab(tab) ? R.drawable.btn_brave : R.drawable.btn_brave_off);

        if (mRewardsLayout == null) return;
        if (isIncognito()) {
            mRewardsLayout.setVisibility(View.GONE);
            updateShieldsLayoutBackground(true);
        } else if (isNativeLibraryReady()
                && mBraveRewardsNativeWorker != null
                && mBraveRewardsNativeWorker.isSupported()
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()
                && NtpUtil.shouldShowRewardsIcon()) {
            mRewardsLayout.setVisibility(View.VISIBLE);
            updateShieldsLayoutBackground(false);
        }
    }

    private boolean isShieldsOnForTab(Tab tab) {
        if (!isNativeLibraryReady()
                || tab == null
                || Profile.fromWebContents(tab.getWebContents()) == null) {
            return false;
        }

        return BraveShieldsContentSettings.getShields(
                Profile.fromWebContents(tab.getWebContents()),
                tab.getUrl().getSpec(),
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
    }

    private boolean isValidProtocolForShields(String protocol) {
        if (protocol.equals("http") || protocol.equals("https")) {
            return true;
        }

        return false;
    }

    public void dismissRewardsPanel() {
        if (mRewardsPopup != null) {
            mRewardsPopup.dismiss();
            mRewardsPopup = null;
        }
    }

    public void dismissWalletPanelOrDialog() {
        if (mDAppsWalletController != null) {
            mDAppsWalletController.dismiss();
            mDAppsWalletController = null;
        }
    }

    public void onRewardsPanelDismiss() {
        mRewardsPopup = null;
    }

    public void openRewardsPanel() {
        onClick(mBraveRewardsButton);
    }

    public boolean isShieldsTooltipShown() {
        if (mShieldsPopupWindowTooltip != null) {
            return mShieldsPopupWindowTooltip.isShowing();
        }
        return false;
    }

    @Override
    public void onCompleteReset(boolean success) {
        if (success) {
            BraveRewardsHelper.resetRewards();
            showOrHideRewardsBadge(false);
        }
    }

    @Override
    public void onNotificationAdded(String id, int type, long timestamp, String[] args) {
        if (mBraveRewardsNativeWorker == null) {
            return;
        }
        mBraveRewardsNativeWorker.getAllNotifications();
    }

    private boolean mayShowBraveAdsOnboardingDialog() {
        Context context = getContext();

        if (BraveAdsSignupDialog.shouldShowNewUserDialog(context)) {
            BraveAdsSignupDialog.showNewUserDialog(getContext());
            return true;
        } else if (BraveAdsSignupDialog.shouldShowNewUserDialogIfRewardsIsSwitchedOff(context)) {
            BraveAdsSignupDialog.showNewUserDialog(getContext());
            return true;
        } else if (BraveAdsSignupDialog.shouldShowExistingUserDialog(context)) {
            BraveAdsSignupDialog.showExistingUserDialog(getContext());
            return true;
        }

        return false;
    }

    @Override
    public void onNotificationsCount(int count) {
        if (mBraveRewardsNotificationsCount != null) {
            if (count != 0) {
                String value = Integer.toString(count);
                if (count > 99) {
                    mBraveRewardsNotificationsCount.setBackground(
                            ResourcesCompat.getDrawable(getContext().getResources(),
                                    R.drawable.brave_rewards_rectangle, /* theme= */ null));
                    value = "99+";
                } else {
                    mBraveRewardsNotificationsCount.setBackground(
                            ResourcesCompat.getDrawable(getContext().getResources(),
                                    R.drawable.brave_rewards_circle, /* theme= */ null));
                }
                mBraveRewardsNotificationsCount.setText(value);
                mBraveRewardsNotificationsCount.setVisibility(View.VISIBLE);
                mIsNotificationPosted = true;
            } else {
                mBraveRewardsNotificationsCount.setText("");
                mBraveRewardsNotificationsCount.setBackgroundResource(0);
                mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
                mIsNotificationPosted = false;
                updateVerifiedPublisherMark();
            }
        }

        if (!PackageUtils.isFirstInstall(getContext())
                && !OnboardingPrefManager.getInstance().isAdsAvailable()) {
            mayShowBraveAdsOnboardingDialog();
        }

        if (System.currentTimeMillis() > BraveRewardsHelper.getRewardsOnboardingIconTiming()
                && checkForRewardsOnboarding()) {
            if (mBraveRewardsOnboardingIcon != null) {
                mBraveRewardsOnboardingIcon.setVisibility(View.VISIBLE);
            }
            if (mBraveRewardsNotificationsCount != null) {
                mBraveRewardsNotificationsCount.setVisibility(View.GONE);
            }

            if (!BraveRewardsHelper.hasRewardsOnboardingIconInvisibleUpdated()) {
                Calendar calender = Calendar.getInstance();
                calender.setTime(new Date());
                calender.add(Calendar.DATE, DAYS_7);
                BraveRewardsHelper.setRewardsOnboardingIconInvisibleTiming(
                        calender.getTimeInMillis());
                BraveRewardsHelper.setRewardsOnboardingIconInvisible(true);
            }
        }
    }

    private boolean isCustomTab() {
        return BraveReflectionUtil.equalTypes(this.getClass(), CustomTabToolbar.class);
    }

    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
        if (mWalletIcon != null) {
            ImageViewCompat.setImageTintList(mWalletIcon,
                    !ColorUtils.shouldUseLightForegroundOnBackground(color) ? mDarkModeTint
                                                                            : mLightModeTint);
        }

        final int textBoxColor =
                ThemeUtils.getTextBoxColorForToolbarBackgroundInNonNativePage(
                        getContext(), color, isIncognito(), isCustomTab());
        updateModernLocationBarColorImpl(textBoxColor);
    }

    /**
     * BraveRewardsNativeWorker.PublisherObserver:
     *   Update a 'verified publisher' checkmark on url bar BAT icon only if
     *   no notifications are posted.
     */
    @Override
    public void onFrontTabPublisherChanged(boolean verified) {
        mIsPublisherVerified = verified;
        updateVerifiedPublisherMark();
    }

    private void updateVerifiedPublisherMark() {
        if (mBraveRewardsNotificationsCount == null) {
            // Most likely we are on a custom page
            return;
        }
        if (mIsInitialNotificationPosted) {
            return;
        } else if (!mIsNotificationPosted) {
            if (mIsPublisherVerified) {
                mBraveRewardsNotificationsCount.setVisibility(View.VISIBLE);
                mBraveRewardsNotificationsCount.setBackground(
                        ResourcesCompat.getDrawable(getContext().getResources(),
                                R.drawable.rewards_verified_tick_icon, /* theme= */ null));
            } else {
                mBraveRewardsNotificationsCount.setBackgroundResource(0);
                mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
            }
        }
    }

    public void onBottomToolbarVisibilityChanged(boolean isVisible) {
        mIsBottomToolbarVisible = isVisible;
        if (BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)
                && getMenuButtonCoordinator() != null) {
            getMenuButtonCoordinator().setVisibility(!isVisible);
            ToggleTabStackButton toggleTabStackButton = findViewById(R.id.tab_switcher_button);
            if (toggleTabStackButton != null) {
                toggleTabStackButton.setVisibility(isTabSwitcherOnBottom() ? GONE : VISIBLE);
            }
        }
    }

    private void updateShieldsLayoutBackground(boolean rounded) {
        if (mShieldsLayout == null) {
            return;
        }

        mShieldsLayout.setBackgroundDrawable(
                ApiCompatibilityUtils.getDrawable(getContext().getResources(),
                        rounded ? R.drawable.modern_toolbar_background_grey_end_segment
                                : R.drawable.modern_toolbar_background_grey_middle_segment));

        updateModernLocationBarColorImpl(mCurrentToolbarColor);
    }

    private boolean isTabSwitcherOnBottom() {
        return mIsBottomToolbarVisible && BottomToolbarVariationManager.isTabSwitcherOnBottom();
    }

    private boolean isMenuButtonOnBottom() {
        return mIsBottomToolbarVisible && BottomToolbarVariationManager.isMenuButtonOnBottom();
    }

    @Override
    public void initialize(
            ToolbarDataProvider toolbarDataProvider,
            ToolbarTabController tabController,
            MenuButtonCoordinator menuButtonCoordinator,
            ToggleTabStackButtonCoordinator tabSwitcherButtonCoordinator,
            HistoryDelegate historyDelegate,
            BooleanSupplier partnerHomepageEnabledSupplier,
            OfflineDownloader offlineDownloader,
            UserEducationHelper userEducationHelper,
            ObservableSupplier<Tracker> trackerSupplier) {
        super.initialize(
                toolbarDataProvider,
                tabController,
                menuButtonCoordinator,
                tabSwitcherButtonCoordinator,
                historyDelegate,
                partnerHomepageEnabledSupplier,
                offlineDownloader,
                userEducationHelper,
                trackerSupplier);

        BraveMenuButtonCoordinator.setMenuFromBottom(isMenuButtonOnBottom());
    }

    public void updateWalletBadgeVisibility(boolean visible) {
        assert mBraveWalletBadge != null;
        mBraveWalletBadge.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void updateMenuButtonState() {
        BraveMenuButtonCoordinator.setMenuFromBottom(mIsBottomToolbarVisible);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (BraveReflectionUtil.equalTypes(this.getClass(), CustomTabToolbar.class)
                || BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)) {
            updateMenuButtonState();
            Tab tab = getToolbarDataProvider() != null ? getToolbarDataProvider().getTab() : null;
            if (tab != null && tab.getWebContents() != null) {
                updateBraveShieldsButtonState(tab);
            }
        }
        super.onDraw(canvas);
    }

    @Override
    public boolean isLocationBarValid(LocationBarCoordinator locationBar) {
        return locationBar != null && locationBar.getPhoneCoordinator() != null
                && locationBar.getPhoneCoordinator().getViewForDrawing() != null;
    }

    @Override
    public void drawAnimationOverlay(ViewGroup toolbarButtonsContainer, Canvas canvas) {
        if (mWalletLayout != null && mWalletLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mWalletLayout, canvas);
            mWalletLayout.draw(canvas);
            canvas.restore();
        }
        if (mShieldsLayout != null && mShieldsLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mShieldsLayout, canvas);
            mShieldsLayout.draw(canvas);
            canvas.restore();
        }
        if (mRewardsLayout != null && mRewardsLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mRewardsLayout, canvas);
            mRewardsLayout.draw(canvas);
            canvas.restore();
        }
    }

    @Override
    public void onMediaFilesUpdated(Url pageUrl, PlaylistItem[] items) {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (currentTab == null || !pageUrl.url.equals(currentTab.getUrl().getSpec())) {
            return;
        }
        mShouldShowPlaylistMenu = true;
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.PREF_ADD_TO_PLAYLIST_BUTTON, true)) {
            showPlaylistButton(items);
        }
    }
}
