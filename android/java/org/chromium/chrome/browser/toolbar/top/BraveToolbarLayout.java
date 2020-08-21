/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.util.Pair;

import androidx.appcompat.app.AlertDialog;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.MathUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.ThreadUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveActivity;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.dialogs.BraveAdsSignupDialog;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettingsObserver;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.AppearancePreferences;
import org.chromium.chrome.browser.shields.BraveShieldsHandler;
import org.chromium.chrome.browser.shields.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.ToolbarColors;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.ui.UiUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;
import org.chromium.ui.widget.Toast;
import org.chromium.chrome.browser.onboarding.SearchActivity;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.BraveStatsTable;
import org.chromium.chrome.browser.local_database.SavedBandwidthTable;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.ntp.BraveNewTabPageLayout;

import java.net.URL;
import java.util.List;
import java.util.Calendar;
import java.util.Date;

public abstract class BraveToolbarLayout extends ToolbarLayout implements OnClickListener,
  View.OnLongClickListener,
  BraveRewardsObserver,
  BraveRewardsNativeWorker.PublisherObserver {
  public static final String PREF_HIDE_BRAVE_REWARDS_ICON = "hide_brave_rewards_icon";

  private static final long MB_10 = 10000000;
  private static final long MINUTES_10 = 10 * 60 * 1000;

  private DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

  private ImageButton mBraveShieldsButton;
  private ImageButton mBraveRewardsButton;
  private HomeButton mHomeButton;
  private FrameLayout mShieldsLayout;
  private FrameLayout mRewardsLayout;
  private BraveShieldsHandler mBraveShieldsHandler;
  private TabModelSelectorTabObserver mTabModelSelectorTabObserver;
  private TabModelSelectorTabModelObserver mTabModelSelectorTabModelObserver;
  private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
  private BraveRewardsPanelPopup mRewardsPopup;
  private BraveShieldsContentSettings mBraveShieldsContentSettings;
  private BraveShieldsContentSettingsObserver mBraveShieldsContentSettingsObserver;
  private TextView mBraveRewardsNotificationsCount;
  private ImageView mBraveRewardsOnboardingIcon;
  private boolean mShieldsLayoutIsColorBackground;
  private int mCurrentToolbarColor;

  private boolean mIsPublisherVerified;
  private boolean mIsNotificationPosted;
  private boolean mIsInitialNotificationPosted; // initial red circle notification
  private boolean mIsRewardsEnabled;

  private PopupWindow mShieldsTooltipPopupWindow;

  public BraveToolbarLayout(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  @Override
  void destroy() {
    if (mBraveShieldsContentSettings != null) {
      mBraveShieldsContentSettings.removeObserver(mBraveShieldsContentSettingsObserver);
    }
    super.destroy();

    if (mBraveRewardsNativeWorker != null) {
      mBraveRewardsNativeWorker.RemoveObserver(this);
      mBraveRewardsNativeWorker.RemovePublisherObserver(this);
    }
  }

  @Override
  protected void onFinishInflate() {
    super.onFinishInflate();

    if (this instanceof ToolbarTablet) {
      ImageButton forwardButton = findViewById(R.id.forward_button);
      if (forwardButton != null) {
        final Drawable forwardButtonDrawable = UiUtils.getTintedDrawable(
            getContext(), R.drawable.btn_right_tablet, R.color.default_icon_color_tint_list);
        forwardButton.setImageDrawable(forwardButtonDrawable);
      }
    }

    mShieldsLayout = (FrameLayout) findViewById(R.id.brave_shields_button_layout);
    mRewardsLayout = (FrameLayout) findViewById(R.id.brave_rewards_button_layout);
    mBraveRewardsNotificationsCount = (TextView) findViewById(R.id.br_notifications_count);
    mBraveRewardsOnboardingIcon = findViewById(R.id.br_rewards_onboarding_icon);
    mBraveShieldsButton = (ImageButton) findViewById(R.id.brave_shields_button);
    mBraveRewardsButton = (ImageButton) findViewById(R.id.brave_rewards_button);
    mHomeButton = (HomeButton) findViewById(R.id.home_button);

    if (mHomeButton != null) {
      mHomeButton.setOnLongClickListener(this);
    }

    if (mBraveShieldsButton != null) {
      mBraveShieldsButton.setClickable(true);
      mBraveShieldsButton.setOnClickListener(this);
      mBraveShieldsButton.setOnLongClickListener(this);
    }

    if (mBraveRewardsButton != null) {
      mBraveRewardsButton.setClickable(true);
      mBraveRewardsButton.setOnClickListener(this);
      mBraveRewardsButton.setOnLongClickListener(this);
    }

    mBraveShieldsHandler = new BraveShieldsHandler(getContext());
    mBraveShieldsHandler.addObserver(new BraveShieldsMenuObserver() {
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
          mBraveShieldsHandler.updateValues(0, 0, 0, 0);
        }
      }
    });
    mBraveShieldsContentSettingsObserver = new BraveShieldsContentSettingsObserver() {
      @Override
      public void blockEvent(int tabId, String block_type, String subresource) {
        mBraveShieldsHandler.addStat(tabId, block_type, subresource);
        Tab currentTab = getToolbarDataProvider().getTab();
        if (currentTab == null || currentTab.getId() != tabId) {
          return;
        }
        mBraveShieldsHandler.updateValues(tabId);
        if (!isIncognito()
            && OnboardingPrefManager.getInstance().isBraveStatsEnabled()
            && (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS)
                || block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS))) {
          addStatsToDb(block_type, subresource, currentTab.getUrlString());
        }
        if (!OnboardingPrefManager.getInstance().hasShieldsTooltipShown()
            && PackageUtils.isFirstInstall(getContext())) {
          mShieldsTooltipPopupWindow = mBraveShieldsHandler.showPopupMenu(mBraveShieldsButton, true);
          OnboardingPrefManager.getInstance().setShieldsTooltipShown(true);
          mShieldsTooltipPopupWindow.getContentView().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
              mShieldsTooltipPopupWindow.dismiss();
              mShieldsTooltipPopupWindow = null;
              showShieldsMenu(mBraveShieldsButton);
            }
          });
        }
      }

      @Override
      public void savedBandwidth(long savings) {
        if (!isIncognito()
            && OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
          addSavedBandwidthToDb(savings);
        }
      }
    };
    // Initially show shields off image. Shields button state will be updated when tab is
    // shown and loading state is changed.
    updateBraveShieldsButtonState(null);
    if (this instanceof ToolbarPhone) {
      if (super.getMenuButtonWrapper() != null && BottomToolbarVariationManager.isMenuButtonOnBottom()) {
        super.getMenuButtonWrapper().setVisibility(View.GONE);
      }
    }
  }

  @Override
  void onNativeLibraryReady() {
    super.onNativeLibraryReady();
    mBraveShieldsContentSettings = BraveShieldsContentSettings.getInstance();
    mBraveShieldsContentSettings.addObserver(mBraveShieldsContentSettingsObserver);

    SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
    if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
        && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()
        && !sharedPreferences.getBoolean(
          AppearancePreferences.PREF_HIDE_BRAVE_REWARDS_ICON, false)
        && mRewardsLayout != null) {
      mRewardsLayout.setVisibility(View.VISIBLE);
    }
    if (mShieldsLayout != null) {
      updateShieldsLayoutBackground(!(mRewardsLayout != null && mRewardsLayout.getVisibility() == View.VISIBLE));
      mShieldsLayout.setVisibility(View.VISIBLE);
    }
    mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
    if (mBraveRewardsNativeWorker != null) {
      mBraveRewardsNativeWorker.AddObserver(this);
      mBraveRewardsNativeWorker.AddPublisherObserver(this);
      mBraveRewardsNativeWorker.GetAllNotifications();
    }
  }

  @Override
  public void setTabModelSelector(TabModelSelector selector) {
    // We might miss events before calling setTabModelSelector, so we need
    // to proactively update the shields button state here, otherwise shields
    // might sometimes show as disabled while it is actually enabled.
    updateBraveShieldsButtonState(getToolbarDataProvider().getTab());
    mTabModelSelectorTabObserver = new TabModelSelectorTabObserver(selector) {
      @Override
      public void onShown(Tab tab, @TabSelectionType int type) {
        // Update shields button state when visible tab is changed.
        updateBraveShieldsButtonState(tab);
      }

      @Override
      public void onPageLoadStarted(Tab tab, String url) {
        if (getToolbarDataProvider().getTab() == tab) {
          updateBraveShieldsButtonState(tab);
        }
        mBraveShieldsHandler.clearBraveShieldsCount(tab.getId());
        dismissShieldsTooltip();
      }

      @Override
      public void onPageLoadFinished(final Tab tab, String url) {
        if (getToolbarDataProvider().getTab() == tab) {
          mBraveShieldsHandler.updateHost(url);
          updateBraveShieldsButtonState(tab);

          Profile mProfile = Profile.getLastUsedRegularProfile();
          long trackersBlockedCount = BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
          long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
          long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(mProfile);
          long estimatedMillisecondsSaved = (trackersBlockedCount + adsBlockedCount) * BraveNewTabPageLayout.MILLISECONDS_PER_ITEM;

          if (!OnboardingPrefManager.getInstance().isAdsTrackersNotificationStarted()
              && (trackersBlockedCount + adsBlockedCount) > 250
              && PackageUtils.isFirstInstall(getContext())) {
            RetentionNotificationUtil.scheduleNotification(getContext(), RetentionNotificationUtil.BRAVE_STATS_ADS_TRACKERS);
            OnboardingPrefManager.getInstance().setAdsTrackersNotificationStarted(true);
          }

          if (!OnboardingPrefManager.getInstance().isDataSavedNotificationStarted()
              && dataSaved > MB_10
              && PackageUtils.isFirstInstall(getContext())) {
            RetentionNotificationUtil.scheduleNotification(getContext(), RetentionNotificationUtil.BRAVE_STATS_DATA);
            OnboardingPrefManager.getInstance().setDataSavedNotificationStarted(true);
          }

          if (!OnboardingPrefManager.getInstance().isTimeSavedNotificationStarted()
              && estimatedMillisecondsSaved > MINUTES_10
              && PackageUtils.isFirstInstall(getContext())) {
            RetentionNotificationUtil.scheduleNotification(getContext(), RetentionNotificationUtil.BRAVE_STATS_TIME);
            OnboardingPrefManager.getInstance().setTimeSavedNotificationStarted(true);
          }
        }
      }

      @Override
      public void onDidFinishNavigation(Tab tab, NavigationHandle navigation) {
        if (getToolbarDataProvider().getTab() == tab && mBraveRewardsNativeWorker != null
            && !tab.isIncognito()) {
          mBraveRewardsNativeWorker.OnNotifyFrontTabUrlChanged(tab.getId(), tab.getUrlString());
        }
      }

      @Override
      public void onDestroyed(Tab tab) {
        mBraveShieldsHandler.removeStat(tab.getId());
      }
    };

    mTabModelSelectorTabModelObserver = new TabModelSelectorTabModelObserver(selector) {
      @Override
      public void didSelectTab(Tab tab, @TabSelectionType int type, int lastId) {
        if (getToolbarDataProvider().getTab() == tab &&
            mBraveRewardsNativeWorker != null &&
            !tab.isIncognito()) {
          mBraveRewardsNativeWorker.OnNotifyFrontTabUrlChanged(tab.getId(), tab.getUrlString());
        }
      }
    };
  }

  public void dismissShieldsTooltip() {
    if (mShieldsTooltipPopupWindow != null) {
      mShieldsTooltipPopupWindow.dismiss();
    }
  }

  private void addSavedBandwidthToDb(long savings) {
    new AsyncTask<Void>() {
      @Override
      protected Void doInBackground() {
        try {
          SavedBandwidthTable savedBandwidthTable = new SavedBandwidthTable(savings, BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
          long rowId = mDatabaseHelper.insertSavedBandwidth(savedBandwidthTable);
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
    } .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
  }

  private void addStatsToDb(String statType, String statSite, String url) {
    new AsyncTask<Void>() {
      @Override
      protected Void doInBackground() {
        try {
          URL urlObject = new URL(url);
          URL siteObject = new URL(statSite);
          BraveStatsTable braveStatsTable = new BraveStatsTable(url, urlObject.getHost(), statType, statSite, siteObject.getHost(), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
          long rowId = mDatabaseHelper.insertStats(braveStatsTable);
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
    } .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
  }

  public void hideRewardsOnboardingIcon() {
    if (mBraveRewardsOnboardingIcon != null) {
      mBraveRewardsOnboardingIcon.setVisibility(View.GONE);
    }
    if (mBraveRewardsNotificationsCount != null) {
      mBraveRewardsNotificationsCount.setVisibility(View.GONE);
    }
    SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
    SharedPreferences.Editor editor = sharedPref.edit();
    editor.putBoolean(BraveRewardsPanelPopup.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, true);
    editor.apply();
  }

  @Override
  public void onClick(View v) {
    if (mBraveShieldsHandler == null) {
      assert false;
      return;
    }
    if (mBraveShieldsButton == v && mBraveShieldsButton != null) {
      showShieldsMenu(mBraveShieldsButton);
    } else if (mBraveRewardsButton == v && mBraveRewardsButton != null) {
      Context context = getContext();
      if (checkForRewardsOnboarding()) {
        OnboardingPrefManager.getInstance().showOnboarding(context);
        hideRewardsOnboardingIcon();
      } else {
        if (null != mRewardsPopup) {
          return;
        }
        mRewardsPopup = new BraveRewardsPanelPopup(v);
        mRewardsPopup.showLikePopDownMenu();
      }
      if (mBraveRewardsNotificationsCount.isShown()) {
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean(BraveRewardsPanelPopup.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, true);
        editor.apply();
        mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
        mIsInitialNotificationPosted = false;
      }
    }
  }

  private boolean checkForRewardsOnboarding() {
    return PackageUtils.isFirstInstall(getContext())
           && (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) && !UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.ENABLED))
           && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile())
           && !OnboardingPrefManager.getInstance().isOnboardingShown();
  }

  private void showShieldsMenu(View mBraveShieldsButton) {
    Tab currentTab = getToolbarDataProvider().getTab();
    if (currentTab == null) {
      return;
    }
    try {
      URL url = new URL(currentTab.getUrlString());
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
  public boolean onLongClick(View v) {
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
    }

    return Toast.showAnchoredToast(context, v, description);
  }

  @Override
  public void onUrlFocusChange(boolean hasFocus) {
    Context context = getContext();
    if (hasFocus
        && PackageUtils.isFirstInstall(context)
        && !OnboardingPrefManager.getInstance().hasSearchEngineOnboardingShown()
        && !BraveSearchEngineUtils.getDSEShortName(true).equals(OnboardingPrefManager.DUCKDUCKGO)) {
      Intent searchActivityIntent = new Intent(context, SearchActivity.class);
      context.startActivity(searchActivityIntent);
    }
    super.onUrlFocusChange(hasFocus);
  }

  public void populateUrlAnimatorSet(boolean hasFocus, int urlFocusToolbarButtonsDuration,
                                     int urlClearFocusTabStackDelayMs, int urlFocusToolbarButtonsTranslationXDP,
                                     List<Animator> animators) {
    if (mBraveShieldsButton != null) {
      Animator animator;
      if (hasFocus) {
        float density = getContext().getResources().getDisplayMetrics().density;
        boolean isRtl = getLayoutDirection() == LAYOUT_DIRECTION_RTL;
        float toolbarButtonTranslationX =
          MathUtils.flipSignIf(urlFocusToolbarButtonsTranslationXDP, isRtl) * density;
        animator = ObjectAnimator.ofFloat(
                     mBraveShieldsButton, TRANSLATION_X, toolbarButtonTranslationX);
        animator.setDuration(urlFocusToolbarButtonsDuration);
        animator.setInterpolator(BakedBezierInterpolator.FADE_OUT_CURVE);
        animators.add(animator);

        animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 0);
        animator.setDuration(urlFocusToolbarButtonsDuration);
        animator.setInterpolator(BakedBezierInterpolator.FADE_OUT_CURVE);
        animators.add(animator);
      } else {
        animator = ObjectAnimator.ofFloat(mBraveShieldsButton, TRANSLATION_X, 0);
        animator.setDuration(urlFocusToolbarButtonsDuration);
        animator.setStartDelay(urlClearFocusTabStackDelayMs);
        animator.setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE);
        animators.add(animator);

        animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 1);
        animator.setDuration(urlFocusToolbarButtonsDuration);
        animator.setStartDelay(urlClearFocusTabStackDelayMs);
        animator.setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE);
        animators.add(animator);
      }
    }
  }

  protected void updateModernLocationBarColor(int color) {
    mCurrentToolbarColor = color;
    if (mShieldsLayout != null) {
      mShieldsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
      if (mShieldsLayoutIsColorBackground) {
        mShieldsLayout.setBackgroundColor(ChromeColors.getDefaultThemeColor(getResources(), isIncognito()));
      }
    }
    if (mRewardsLayout != null) {
      mRewardsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
    }
  }

  protected int getBoundsAfterAccountingForRightButtons(
    ViewGroup toolbarButtonsContainer) {
    if (toolbarButtonsContainer == null ||
        mShieldsLayout == null) {
      assert false;
      return 0;
    }
    ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams)toolbarButtonsContainer.getLayoutParams();

    int rewardsLen = (mRewardsLayout == null || mRewardsLayout.getVisibility() == View.GONE) ? 0
                     : mRewardsLayout.getWidth();
    return toolbarButtonsContainer.getMeasuredWidth() -
           mShieldsLayout.getWidth() -
           rewardsLen +
           params.getMarginEnd();
  }

  /**
    * If |tab| is null, set disabled image to shields button and |urlString| is
    * ignored.
    * If |urlString| is null, url is fetched from |tab|.
    */
  private void updateBraveShieldsButtonState(Tab tab) {
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

    SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

    if (mRewardsLayout == null) return;
    if (isIncognito()) {
      mRewardsLayout.setVisibility(View.GONE);
      updateShieldsLayoutBackground(true);
    } else if (isNativeLibraryReady() &&
               ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) &&
               !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed() &&
               !sharedPreferences.getBoolean(
                 AppearancePreferences.PREF_HIDE_BRAVE_REWARDS_ICON, false)) {
      mRewardsLayout.setVisibility(View.VISIBLE);
      updateShieldsLayoutBackground(false);
    }
  }

  private boolean isShieldsOnForTab(Tab tab) {
    if (tab == null) {
      assert false;
      return false;
    }
    return BraveShieldsContentSettings.getShields(Profile.fromWebContents(((TabImpl)tab).getWebContents()), tab.getUrlString(),
           BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
  }

  private boolean isValidProtocolForShields(String protocol) {
    if (protocol.equals("http") || protocol.equals("https")) {
      return true;
    }

    return false;
  }

  public  void dismissRewardsPanel() {
    if (mRewardsPopup != null) {
      mRewardsPopup.dismiss();
      mRewardsPopup = null;
    }
  }

  public  void onRewardsPanelDismiss() {
    mRewardsPopup = null;
  }

  public void openRewardsPanel() {
    onClick(mBraveRewardsButton);
  }

  @Override
  public void OnWalletInitialized(int error_code) {
    if (error_code == BraveRewardsNativeWorker.SAFETYNET_ATTESTATION_FAILED) {
      BravePrefServiceBridge.getInstance().setSafetynetCheckFailed(true);
      if (mRewardsLayout != null && mShieldsLayout != null) {
        mRewardsLayout.setVisibility(View.GONE);
        updateShieldsLayoutBackground(true);
      }
      // Show message
      AlertDialog.Builder alert = new AlertDialog.Builder(getContext(), R.style.Theme_Chromium_AlertDialog);
      AlertDialog alertDialog = alert.setMessage(getResources().getString(R.string.brave_rewards_not_available))
                                .setPositiveButton(R.string.ok, null)
                                .create();
      alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
      alertDialog.show();
      // If current tab is rewards tab we close it, as it is not valid anymore
      Tab currentTab = getToolbarDataProvider().getTab();
      if (currentTab != null && getToolbarDataProvider().getCurrentUrl().equals(BraveActivity.REWARDS_SETTINGS_URL)) {
        if (getContext() instanceof BraveActivity) {
          BraveActivity activity = (BraveActivity)getContext();
          activity.getCurrentTabModel().closeTab(currentTab);
        }
      }
    } else if (error_code == BraveRewardsNativeWorker.WALLET_CREATED) { // Wallet created code
      // Make sure that flag is set as panel can be closed before wallet is created
      BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedRegularProfile());
      // Check and set flag to show Brave Rewards icon if enabled
      SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
      SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
      // Set preferences that Brave Rewards was turned On and that Brave Rewards icon is not hidden
      sharedPreferencesEditor.putBoolean(BraveRewardsPanelPopup.PREF_WAS_BRAVE_REWARDS_TURNED_ON, true);
      mIsRewardsEnabled = true;
      if (sharedPreferences.getBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, false)) {
        sharedPreferencesEditor.putBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, false);
        sharedPreferencesEditor.apply();
        BraveRelaunchUtils.askForRelaunch((ChromeActivity)getContext());
      }
      sharedPreferencesEditor.apply();
    }
  }

  @Override
  public void OnPublisherInfo(int tabId) {}

  @Override
  public void OnGetCurrentBalanceReport(double[] report) {}

  @Override
  public void OnNotificationAdded(String id, int type, long timestamp,
                                  String[] args) {
    if (mBraveRewardsNativeWorker == null) {
      return;
    }

    if (type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET) {
      mBraveRewardsNativeWorker.DeleteNotification(id);
    } else if (type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT) {
      // Set flag
      SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
      SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
      sharedPreferencesEditor.putBoolean(BraveRewardsPanelPopup.PREF_GRANTS_NOTIFICATION_RECEIVED, true);
      sharedPreferencesEditor.apply();
    }
    mBraveRewardsNativeWorker.GetAllNotifications();
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
  public void OnNotificationsCount(int count) {
    boolean rewardsEnabled = UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.ENABLED);
    if (mBraveRewardsNotificationsCount != null && rewardsEnabled) {
      if (count != 0) {
        String value = Integer.toString(count);
        if (count > 99) {
          mBraveRewardsNotificationsCount.setBackground(
            getResources().getDrawable(R.drawable.brave_rewards_rectangle));
          value = "99+";
        } else {
          mBraveRewardsNotificationsCount.setBackground(
            getResources().getDrawable(R.drawable.brave_rewards_circle));
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

    updateNotificationBadgeForNewInstall(rewardsEnabled);
    if (!PackageUtils.isFirstInstall(getContext())
        && !OnboardingPrefManager.getInstance().isAdsAvailable()) {
      mayShowBraveAdsOnboardingDialog();
    }

    if (checkForRewardsOnboarding()) {
      if (mBraveRewardsOnboardingIcon != null) {
        mBraveRewardsOnboardingIcon.setVisibility(View.VISIBLE);
      }
      if (mBraveRewardsNotificationsCount != null) {
        mBraveRewardsNotificationsCount.setVisibility(View.GONE);
      }
    }
  }

  @Override
  public void OnGetLatestNotification(String id, int type, long timestamp,
                                      String[] args) {}

  @Override
  public void OnNotificationDeleted(String id) {}

  @Override
  public void OnIsWalletCreated(boolean created) {}

  @Override
  public void OnGetPendingContributionsTotal(double amount) {}

  @Override
  public void OnGetRewardsMainEnabled(boolean enabled) {
    mIsRewardsEnabled = enabled;
  }

  @Override
  public void OnGetAutoContributeProperties() {}

  @Override
  public void OnGetReconcileStamp(long timestamp) {}

  @Override
  public void OnRecurringDonationUpdated() {}

  @Override
  public void OnResetTheWholeState(boolean success) {}

  @Override
  public void OnRewardsMainEnabled(boolean enabled) {

    Log.e("OnRewardsMainEnabled", "isEnabled : " + enabled);

    SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
    SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();

    boolean needRelaunch = false;
    if (enabled && sharedPreferences.getBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, false)) {
      sharedPreferencesEditor.putBoolean(PREF_HIDE_BRAVE_REWARDS_ICON, false);
      needRelaunch = true;
    }

    if (mBraveRewardsNotificationsCount != null) {
      String count = mBraveRewardsNotificationsCount.getText().toString();
      if (!count.isEmpty()) {
        mBraveRewardsNotificationsCount.setVisibility(enabled ? View.VISIBLE : View.GONE);
      }
    }

    if (needRelaunch) BraveRelaunchUtils.askForRelaunch(getContext());
    mIsRewardsEnabled = enabled;
    updateVerifiedPublisherMark();
  }

  private void updateNotificationBadgeForNewInstall(boolean rewardsEnabled) {
    SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
    boolean shownBefore = sharedPref.getBoolean(BraveRewardsPanelPopup.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, false);
    boolean shouldShow =
      mBraveRewardsNotificationsCount != null && !shownBefore && !rewardsEnabled;
    mIsInitialNotificationPosted = shouldShow; // initial notification

    if (!shouldShow) return;

    mBraveRewardsNotificationsCount.setText("");
    mBraveRewardsNotificationsCount.setBackground(
      getResources().getDrawable(R.drawable.brave_rewards_circle));
    mBraveRewardsNotificationsCount.setVisibility(View.VISIBLE);
  }

  @Override
  public void onThemeColorChanged(int color, boolean shouldAnimate) {
    final int textBoxColor = ToolbarColors.getTextBoxColorForToolbarBackgroundInNonNativePage(
                               getResources(), color, isIncognito());
    updateModernLocationBarColor(textBoxColor);
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
    } else if (!mIsRewardsEnabled) {
      mBraveRewardsNotificationsCount.setBackgroundResource(0);
      mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
    } else if (!mIsNotificationPosted) {
      if (mIsPublisherVerified) {
        mBraveRewardsNotificationsCount.setVisibility(View.VISIBLE);
        mBraveRewardsNotificationsCount.setBackground(
          getResources().getDrawable(R.drawable.bat_verified));
      } else {
        mBraveRewardsNotificationsCount.setBackgroundResource(0);
        mBraveRewardsNotificationsCount.setVisibility(View.INVISIBLE);
      }
    }
  }

  @Override
  public void onBottomToolbarVisibilityChanged(boolean isVisible) {
    if (this instanceof ToolbarPhone && super.getMenuButtonWrapper() != null) {
      super.getMenuButtonWrapper().setVisibility(isVisible ? View.GONE : View.VISIBLE);
    }
  }

  private void updateShieldsLayoutBackground(boolean rounded) {
    if (!(this instanceof ToolbarTablet) || (mShieldsLayout == null)) return;

    if (rounded) {
      mShieldsLayout.setBackgroundDrawable(ApiCompatibilityUtils.getDrawable(getContext().getResources(),
                                           R.drawable.modern_toolbar_background_grey_end_segment));
      mShieldsLayoutIsColorBackground = false;
    } else {
      mShieldsLayout.setBackgroundColor(ChromeColors.getDefaultThemeColor(getResources(), isIncognito()));
      mShieldsLayoutIsColorBackground = true;
    }
    updateModernLocationBarColor(mCurrentToolbarColor);
  }

  @Override
  View getMenuButtonWrapper() {
    if (this instanceof ToolbarPhone && BottomToolbarVariationManager.isMenuButtonOnBottom()) {
      return null;
    }
    return super.getMenuButtonWrapper();
  }
}
