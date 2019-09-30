/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.appmenu.BraveShieldsMenuHandler;
import org.chromium.chrome.browser.appmenu.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.preferences.AppearancePreferences;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettingsObserver;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.tabmodel.TabSelectionType;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

import java.net.URL;
import java.util.List;

public abstract class BraveToolbarLayout extends ToolbarLayout implements OnClickListener,
                                                           View.OnLongClickListener {
  private ImageButton mBraveShieldsButton;
  private ImageButton mBraveRewardsButton;
  private FrameLayout mShieldsLayout;
  private FrameLayout mRewardsLayout;
  private ChromeActivity mMainActivity;
  private BraveShieldsMenuHandler mBraveShieldsMenuHandler;
  private TabModelSelectorTabObserver mTabModelSelectorTabObserver;
  private BraveRewardsPanelPopup mRewardsPopup;
  private BraveShieldsContentSettings mBraveShieldsContentSettings;
  private BraveShieldsContentSettingsObserver mBraveShieldsContentSettingsObserver;

  public BraveToolbarLayout(Context context, AttributeSet attrs) {
      super(context, attrs);
  }

  @Override
  void destroy() {
      if (mBraveShieldsContentSettings != null) {
          mBraveShieldsContentSettings.destroy();
      }
      super.destroy();
  }

  @Override
  protected void onFinishInflate() {
      super.onFinishInflate();
      mShieldsLayout = (FrameLayout) findViewById(R.id.brave_shields_button_layout);
      mRewardsLayout = (FrameLayout) findViewById(R.id.brave_rewards_button_layout);
      mBraveShieldsButton = (ImageButton) findViewById(R.id.brave_shields_button);
      mBraveRewardsButton = (ImageButton) findViewById(R.id.brave_rewards_button);
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
      for (Activity ref : ApplicationStatus.getRunningActivities()) {
          if (!(ref instanceof ChromeActivity)) continue;
          mMainActivity = (ChromeActivity)ref;
      }
      mBraveShieldsMenuHandler = new BraveShieldsMenuHandler(mMainActivity, R.menu.brave_shields_menu);
      mBraveShieldsMenuHandler.addObserver(new BraveShieldsMenuObserver() {
          @Override
          public void onMenuTopShieldsChanged(boolean isOn, boolean isTopShield) {
              Tab currentTab = mMainActivity.getActivityTab();
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
              if (null != mBraveShieldsMenuHandler) {
                  // Clean the Bravery Panel
                  mBraveShieldsMenuHandler.updateValues(0, 0, 0, 0);
              }
          }
      });
      mBraveShieldsContentSettingsObserver = new BraveShieldsContentSettingsObserver() {
          @Override
          public void blockEvent(int tabId, String block_type, String subresource) {
              mBraveShieldsMenuHandler.addStat(tabId, block_type, subresource);
              Tab currentTab = mMainActivity.getActivityTab();
              if (currentTab == null || currentTab.getId() != tabId) {
                  return;
              }
              mBraveShieldsMenuHandler.updateValues(tabId);
          }
      };
      // Initially show shields off image. Shields button state will be updated when tab is
      // shown and loading state is changed.
      updateBraveShieldsButtonState(null);
  }

  @Override
  void onNativeLibraryReady() {
      super.onNativeLibraryReady();
      mBraveShieldsContentSettings = new BraveShieldsContentSettings(mBraveShieldsContentSettingsObserver);

      SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
      if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS)
              && !sharedPreferences.getBoolean(
                      AppearancePreferences.PREF_HIDE_BRAVE_REWARDS_ICON, false)) {
          if (mRewardsLayout != null) {
              mRewardsLayout.setVisibility(View.VISIBLE);
          }
      }
  }

  @Override
  public void setTabModelSelector(TabModelSelector selector) {
      mTabModelSelectorTabObserver = new TabModelSelectorTabObserver(selector) {
            @Override
            public void onShown(Tab tab, @TabSelectionType int type) {
                // Update shields button state when visible tab is changed.
                updateBraveShieldsButtonState(tab);
            }

            @Override
            public void onPageLoadStarted(Tab tab, String url) {
                if (mMainActivity.getActivityTab() == tab) {
                    updateBraveShieldsButtonState(tab);
                }
                mBraveShieldsMenuHandler.clearBraveShieldsCount(tab.getId());
            }

            @Override
            public void onPageLoadFinished(final Tab tab, String url) {
                if (mMainActivity.getActivityTab() == tab) {
                    mBraveShieldsMenuHandler.updateHost(url);
                    updateBraveShieldsButtonState(tab);
                }
            }

            @Override
            public void onDestroyed(Tab tab) {
                mBraveShieldsMenuHandler.removeStat(tab.getId());
            }
        };
  }

  @Override
  public void onClick(View v) {
      if (mMainActivity == null || mBraveShieldsMenuHandler == null) {
          assert false;
          return;
      }
      if (mBraveShieldsButton == v && mBraveShieldsButton != null) {
          if (mMainActivity.getFullscreenManager() != null
                  && mMainActivity.getFullscreenManager().getPersistentFullscreenMode()) {
              return;
          }
          Tab currentTab = mMainActivity.getActivityTab();
          if (currentTab == null) {
              return;
          }
          try {
              URL url = new URL(currentTab.getUrl());
              // Don't show shields popup if protocol is not valid for shields.
              if (!isValidProtocolForShields(url.getProtocol())) {
                  return;
              }
              mBraveShieldsMenuHandler.show(mBraveShieldsButton, currentTab.getUrl(),
                  url.getHost(), currentTab.getId(), currentTab.getProfile());
          } catch (Exception e) {
              // Do nothing if url is invalid.
              // Just return w/o showing shields popup.
              return;
          }
      } else if (mBraveRewardsButton == v && mBraveRewardsButton != null) {
          if (null != mRewardsPopup) {
              return;
          }
          mRewardsPopup = new BraveRewardsPanelPopup(v);
          mRewardsPopup.showLikePopDownMenu();
          // TODO
          // if (mBraveRewardsNotificationsCount.isShown()) {
              SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
              SharedPreferences.Editor editor = sharedPref.edit();
              editor.putBoolean(BraveRewardsPanelPopup.PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED, true);
              editor.apply();
              // TODO
              // mBraveRewardsNotificationsCount.setVisibility(View.GONE);
          // TODO
          // }
      }
  }

  @Override
  public boolean onLongClick(View v) {
      String description = "";
      Context context = getContext();
      Resources resources = context.getResources();

      if (v == mBraveShieldsButton) {
          description = description = resources.getString(R.string.accessibility_toolbar_btn_brave_shields);
      } /*else if (v == mBraveRewardsPanelButton) {
          description = resources.getString(R.string.accessibility_toolbar_btn_brave_rewards);
      }*/

      return AccessibilityUtil.showAccessibilityToast(context, v, description);
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
      mShieldsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
      mRewardsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
  }

  protected int getBoundsAfterAccountingForRightButtons(
    ViewGroup toolbarButtonsContainer) {
      if (toolbarButtonsContainer == null ||
        mShieldsLayout == null) {
          assert false;
          return 0;
      }
      ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams)toolbarButtonsContainer.getLayoutParams();

      return toolbarButtonsContainer.getMeasuredWidth() -
            mShieldsLayout.getWidth() -
            mRewardsLayout.getWidth() +
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
  }

  private boolean isShieldsOnForTab(Tab tab) {
      if (tab == null) {
          assert false;
          return false;
      }
      return BraveShieldsContentSettings.getShields(tab.getProfile(), tab.getUrl(),
          BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
  }

  private boolean isValidProtocolForShields(String protocol) {
      if (protocol.equals("http") || protocol.equals("https")) {
          return true;
      }

      return false;
  }
}
