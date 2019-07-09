/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.FrameLayout;
import android.widget.ImageView;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.appmenu.BraveShieldsMenuHandler;
import org.chromium.chrome.browser.appmenu.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

import java.net.URL;
import java.util.List;

public abstract class BraveToolbarLayout extends ToolbarLayout implements OnClickListener,
                                                           View.OnLongClickListener {
  private ImageView mBraveShieldsButton;
  private FrameLayout mShieldsLayout;
  private ChromeActivity mMainActivity;
  private BraveShieldsMenuHandler mBraveShieldsMenuHandler;
  private TabModelSelectorTabObserver mTabModelSelectorTabObserver;

  public BraveToolbarLayout(Context context, AttributeSet attrs) {
      super(context, attrs);
  }

  @Override
  protected void onFinishInflate() {
      super.onFinishInflate(); 
      mShieldsLayout = (FrameLayout) findViewById(R.id.brave_shields_button_layout);
      mBraveShieldsButton = (ImageView) findViewById(R.id.brave_shields_button);
      if (mBraveShieldsButton != null) {
          mBraveShieldsButton.setClickable(true);
          mBraveShieldsButton.setOnClickListener(this);
          mBraveShieldsButton.setOnLongClickListener(this);
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
                  if (isOn) {
                      setBraveShieldsColored();
                  } else {
                      setBraveShieldsBlackAndWhite();
                  }
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
  }

  @Override
  public void setTabModelSelector(TabModelSelector selector) {
      mTabModelSelectorTabObserver = new TabModelSelectorTabObserver(selector) {
            @Override
            public void onPageLoadStarted(Tab tab, String url) {
                // TODO
                // ChromeApplication app = (ChromeApplication)ContextUtils.getBaseApplicationContext();
                // if ((null != app) && (null != app.getShieldsConfig())) {
                //     app.getShieldsConfig().setTabModelSelectorTabObserver(mTabModelSelectorTabObserver);
                // }
                if (mMainActivity.getActivityTab() == tab) {
                    try {
                        URL urlCheck = new URL(url);
                        setBraveShieldsColor(tab.isIncognito(), urlCheck.getHost());
                    } catch (Exception e) {
                        setBraveShieldsBlackAndWhite();
                    }
                }
                // TODO
                // tab.clearBraveShieldsCount();
            }

            @Override
            public void onPageLoadFinished(final Tab tab, String url) {
                // TODO
                // mAppIndexingUtil.extractCopylessPasteMetadata(tab);
                if (mMainActivity.getActivityTab() == tab) {
                    try {
                        URL urlCheck = new URL(url);
                        setBraveShieldsColor(tab.isIncognito(), urlCheck.getHost());
                    } catch (Exception e) {
                        setBraveShieldsBlackAndWhite();
                    }
                }
            }

            // TODO
            // @Override
            // public void onBraveShieldsCountUpdate(String url, int adsAndTrackers, int httpsUpgrades,
            //         int scriptsBlocked, int fingerprintsBlocked) {
            //     List<Tab> tabsList = new ArrayList<>();
            //     for (int i = 0; i < getCurrentTabModel().getCount(); i++) {
            //         Tab tab = getCurrentTabModel().getTabAt(i);
            //         if (null != tab) {
            //             String tabUrl = tab.getUrl();
            //             if (tabUrl.equals(url)) {
            //                 tabsList.add(tab);
            //             }
            //         }
            //     }
            //     if (0 == tabsList.size()) {
            //         return;
            //     }

            //     Tab tabToUpdate = null;
            //     for (Tab currentTab : tabsList) {
            //         if (null == tabToUpdate) {
            //             tabToUpdate = currentTab;
            //             continue;
            //         }
            //         if (0 != adsAndTrackers) {
            //             if (tabToUpdate.getAdsAndTrackers() > currentTab.getAdsAndTrackers()) {
            //                 tabToUpdate = currentTab;
            //             }
            //         } else if (0 != httpsUpgrades) {
            //             if (tabToUpdate.getHttpsUpgrades() > currentTab.getHttpsUpgrades()) {
            //                 tabToUpdate = currentTab;
            //             }
            //         } else if (0 != scriptsBlocked) {
            //           if (tabToUpdate.getScriptsBlocked() > currentTab.getScriptsBlocked()) {
            //               tabToUpdate = currentTab;
            //           }
            //         } else if (0 != fingerprintsBlocked) {
            //           if (tabToUpdate.getFingerprintsBlocked() > currentTab.getFingerprintsBlocked()) {
            //               tabToUpdate = currentTab;
            //           }
            //         }
            //     }
            //     if (null != tabToUpdate) {
            //         tabToUpdate.braveShieldsCountUpdate(adsAndTrackers, httpsUpgrades, scriptsBlocked, fingerprintsBlocked);
            //         if (getActivityTab() == tabToUpdate) {
            //             updateBraveryPanelCounts(tabToUpdate.getAdsAndTrackers(), tabToUpdate.getHttpsUpgrades(),
            //                     tabToUpdate.getScriptsBlocked(), tabToUpdate.getFingerprintsBlocked());
            //         }
            //     }
            // }
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
              assert false;
              return;
          }
          try {
              URL url = new URL(currentTab.getUrl());
              URL protocolHost = new URL(url.getProtocol(), url.getHost(), "");

              setBraveShieldsColor(currentTab.isIncognito(), protocolHost.toString());
              mBraveShieldsMenuHandler.show(mBraveShieldsButton
                , currentTab.isIncognito()
                , protocolHost.toString()
                , url.getHost()
                // TODO
                , 0, 0, 0, 0);
                //, currentTab.getAdsAndTrackers()
                //, currentTab.getHttpsUpgrades()
                //, currentTab.getScriptsBlocked()
                //, currentTab.getFingerprintsBlocked());
          } catch (Exception e) {
              // TODO
              setBraveShieldsBlackAndWhite();
          }
      }
  }

  private void setBraveShieldsBlackAndWhite() {
      if (null != mBraveShieldsButton) {
          mBraveShieldsButton.setImageResource(R.drawable.btn_brave_off);
      }
  }

  protected void setBraveShieldsColored() {
      if (null != mBraveShieldsButton) {
          mBraveShieldsButton.setImageResource(R.drawable.btn_brave);
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
            //mRewardsLayout.getWidth() +
            params.getMarginEnd();
  }

  private void setBraveShieldsColor(boolean incognitoTab, String url) {
      if (BraveShieldsContentSettings.getShields(incognitoTab, url, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
          // Set Brave Shields button in color if we have a valid URL
          setBraveShieldsColored();
      } else {
          setBraveShieldsBlackAndWhite();
      }
  }
}
