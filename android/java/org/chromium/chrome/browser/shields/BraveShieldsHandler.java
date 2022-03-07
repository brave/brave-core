/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.Manifest;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ImageSpan;
import android.text.style.StyleSpan;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListPopupWindow;
import android.widget.PopupMenu;
import android.widget.PopupWindow;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.app.ActivityCompat;
import androidx.core.widget.TextViewCompat;

import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.shields.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.shields.BraveShieldsUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ConfigurationUtils;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Object responsible for handling the creation, showing, hiding of the BraveShields menu.
 */
public class BraveShieldsHandler implements BraveRewardsHelper.LargeIconReadyCallback {

    private static class BlockersInfo {
        public BlockersInfo() {
            mAdsBlocked = 0;
            mTrackersBlocked = 0;
            mHTTPSUpgrades = 0;
            mScriptsBlocked = 0;
            mFingerprintsBlocked = 0;
        }

        public int mAdsBlocked;
        public int mTrackersBlocked;
        public int mHTTPSUpgrades;
        public int mScriptsBlocked;
        public int mFingerprintsBlocked;
    }

    private final Context mContext;
    private PopupWindow mPopupWindow;
    private AnimatorSet mMenuItemEnterAnimator;
    private BraveShieldsMenuObserver mMenuObserver;
    private View mHardwareButtonMenuAnchor;
    private final Map<Integer, BlockersInfo> mTabsStat =
        Collections.synchronizedMap(new HashMap<Integer, BlockersInfo>());

    private OnCheckedChangeListener mBraveShieldsAdsTrackingChangeListener;
    private SwitchCompat mBraveShieldsHTTPSEverywhereSwitch;
    private OnCheckedChangeListener mBraveShieldsHTTPSEverywhereChangeListener;
    private SwitchCompat mBraveShieldsBlockingScriptsSwitch;
    private OnCheckedChangeListener mBraveShieldsBlockingScriptsChangeListener;

    private View mPopupView;
    private LinearLayout mMainLayout;
    private LinearLayout mSecondaryLayout;
    private LinearLayout mAboutLayout;
    private LinearLayout mToggleLayout;
    private LinearLayout mThankYouLayout;
    private LinearLayout mPrivacyReportLayout;
    private LinearLayout mReportBrokenSiteLayout;
    private TextView mSiteBlockCounterText;
    private TextView mShieldsDownText;
    private TextView mSiteBrokenWarningText;
    private View mBottomDivider;
    private ImageView mToggleIcon;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private BraveRewardsHelper mIconFetcher;

    private String mHost;
    private String mTitle;
    private int mTabId;
    private Profile mProfile;

    private static Context scanForActivity(Context cont) {
        if (cont == null)
            return null;
        else if (cont instanceof Activity)
            return cont;
        else if (cont instanceof ContextWrapper)
            return scanForActivity(((ContextWrapper)cont).getBaseContext());

        return cont;
    }

    /**
     * Constructs a BraveShieldsHandler object.
     * @param context Context that is using the BraveShieldsMenu.
     */
    public BraveShieldsHandler(Context context) {
        Context contextCandidate = scanForActivity(context);
        mHardwareButtonMenuAnchor = null;
        mContext = (contextCandidate != null && (contextCandidate instanceof Activity))
                ? contextCandidate
                : null;

        if (mContext != null) {
            mHardwareButtonMenuAnchor = ((Activity)mContext).findViewById(R.id.menu_anchor_stub);
        }
    }

    public void addStat(int tabId, String block_type, String subresource) {
        if (!mTabsStat.containsKey(tabId)) {
            mTabsStat.put(tabId, new BlockersInfo());
        }
        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS)) {
            blockersInfo.mAdsBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS)) {
            blockersInfo.mTrackersBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES)) {
            blockersInfo.mHTTPSUpgrades++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            blockersInfo.mScriptsBlocked++;
        } else if (block_type.equals(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            blockersInfo.mFingerprintsBlocked++;
        }
    }

    public void removeStat(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return;
        }
        mTabsStat.remove(tabId);
    }

    public void clearBraveShieldsCount(int tabId) {
        mTabsStat.put(tabId, new BlockersInfo());
    }

    public void addObserver(BraveShieldsMenuObserver menuObserver) {
        mMenuObserver = menuObserver;
    }

    public void show(View anchorView, Tab tab) {
        if (mHardwareButtonMenuAnchor == null) return;

        mHost = tab.getUrl().getSpec();
        mTitle = tab.getUrl().getHost();
        mTabId = tab.getId();
        mProfile = Profile.fromWebContents(tab.getWebContents());

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mIconFetcher = new BraveRewardsHelper(tab);
        mPopupWindow = showPopupMenu(anchorView);

        updateValues(mTabId);
    }

    public PopupWindow showPopupMenu(View anchorView) {
        if (mContext == null) return null;

        int rotation = ((Activity)mContext).getWindowManager().getDefaultDisplay().getRotation();
        // This fixes the bug where the bottom of the menu starts at the top of
        // the keyboard, instead of overlapping the keyboard as it should.
        int displayHeight = mContext.getResources().getDisplayMetrics().heightPixels;
        int widthHeight = mContext.getResources().getDisplayMetrics().widthPixels;
        int currentDisplayWidth = widthHeight;

        // In appcompat 23.2.1, DisplayMetrics are not updated after rotation change. This is a
        // workaround for it. See crbug.com/599048.
        // TODO(ianwen): Remove the rotation check after we roll to 23.3.0.
        if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
            currentDisplayWidth = Math.min(displayHeight, widthHeight);
            displayHeight = Math.max(displayHeight, widthHeight);
        } else if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
            currentDisplayWidth = Math.max(displayHeight, widthHeight);
            displayHeight = Math.min(displayHeight, widthHeight);
        } else {
            assert false : "Rotation unexpected";
        }
        if (anchorView == null) {
            Rect rect = new Rect();
            ((Activity)mContext).getWindow().getDecorView().getWindowVisibleDisplayFrame(rect);
            int statusBarHeight = rect.top;
            mHardwareButtonMenuAnchor.setY((displayHeight - statusBarHeight));

            anchorView = mHardwareButtonMenuAnchor;
        }

        ContextThemeWrapper wrapper = new ContextThemeWrapper(mContext, R.style.OverflowMenuThemeOverlay);
        Point pt = new Point();
        ((Activity)mContext).getWindowManager().getDefaultDisplay().getSize(pt);
        // Get the height and width of the display.
        Rect appRect = new Rect();
        ((Activity)mContext).getWindow().getDecorView().getWindowVisibleDisplayFrame(appRect);

        // Use full size of window for abnormal appRect.
        if (appRect.left < 0 && appRect.top < 0) {
            appRect.left = 0;
            appRect.top = 0;
            appRect.right = ((Activity)mContext).getWindow().getDecorView().getWidth();
            appRect.bottom = ((Activity)mContext).getWindow().getDecorView().getHeight();
        }

        LayoutInflater inflater = (LayoutInflater) anchorView.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        mPopupView = inflater.inflate(R.layout.brave_shields_main_layout, null);
        setUpViews();

        //Specify the length and width through constants
        int width;
        if (ConfigurationUtils.isLandscape(mContext)) {
            width = (int) ((mContext.getResources().getDisplayMetrics().widthPixels) * 0.50);
        } else {
            width = (int) ((mContext.getResources().getDisplayMetrics().widthPixels) * 0.75);
        }
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;

        //Make Inactive Items Outside Of PopupWindow
        boolean focusable = true;

        //Create a window with our parameters
        PopupWindow popupWindow = new PopupWindow(mPopupView, width, height, focusable);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            popupWindow.setElevation(20);
        }
        // mPopup.setBackgroundDrawable(mContext.getResources().getDrawable(android.R.drawable.picture_frame));
        //Set the location of the window on the screen
        popupWindow.showAsDropDown(anchorView, 0, 0);
        popupWindow.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
        popupWindow.setAnimationStyle(R.style.EndIconMenuAnim);

        // Turn off window animations for low end devices, and on Android M, which has built-in menu
        // animations.
        if (SysUtils.isLowEndDevice() || Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            popupWindow.setAnimationStyle(0);
        }

        Rect bgPadding = new Rect();
        int popupWidth = wrapper.getResources().getDimensionPixelSize(R.dimen.menu_width)
                         + bgPadding.left + bgPadding.right;
        popupWindow.setWidth(popupWidth);

        return popupWindow;
    }

    public void updateHost(String host) {
        mHost = host;
    }

    public void updateValues(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return;
        }
        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        updateValues(blockersInfo.mAdsBlocked + blockersInfo.mTrackersBlocked,
                     blockersInfo.mHTTPSUpgrades, blockersInfo.mScriptsBlocked, blockersInfo.mFingerprintsBlocked);
    }

    public int getAdsBlockedCount(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return 0;
        }

        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        return blockersInfo.mAdsBlocked;
    }

    public int getTrackersBlockedCount(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return 0;
        }

        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        return blockersInfo.mTrackersBlocked;
    }

    public int getHttpsUpgradeCount(int tabId) {
        if (!mTabsStat.containsKey(tabId)) {
            return 0;
        }

        BlockersInfo blockersInfo = mTabsStat.get(tabId);
        return blockersInfo.mHTTPSUpgrades;
    }

    public void updateValues(int adsAndTrackers, int httpsUpgrades, int scriptsBlocked, int fingerprintsBlocked) {
        if (mContext == null) {
            return;
        }
        final int fadsAndTrackers = adsAndTrackers;
        final int fhttpsUpgrades = httpsUpgrades;
        final int fscriptsBlocked = scriptsBlocked;
        final int ffingerprintsBlocked = fingerprintsBlocked;
        ((Activity)mContext).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!isShowing()) {
                    return;
                }
                try {
                    mSiteBlockCounterText.setText(String.valueOf(fadsAndTrackers
                                                  + fhttpsUpgrades
                                                  + fscriptsBlocked
                                                  + ffingerprintsBlocked));
                } catch (NullPointerException exc) {
                    // It means that the Bravery Panel was destroyed during the update, we just do nothing
                }
            }
        });
    }

    public boolean isShowing() {
        if (null == mPopupWindow) {
            return false;
        }

        return mPopupWindow.isShowing();
    }

    public void hideBraveShieldsMenu() {
        if (isShowing()) {
            mPopupWindow.dismiss();
        }
    }

    private void initViews() {
        mMainLayout = mPopupView.findViewById(R.id.main_layout);
        mSecondaryLayout = mPopupView.findViewById(R.id.brave_shields_secondary_layout_id);
        mAboutLayout = mPopupView.findViewById(R.id.brave_shields_about_layout_id);
        mToggleLayout = mPopupView.findViewById(R.id.brave_shields_toggle_layout_id);
        mSiteBlockCounterText = mPopupView.findViewById(R.id.site_block_count_text);
        mShieldsDownText = mPopupView.findViewById(R.id.shield_down_text);
        mSiteBrokenWarningText = mPopupView.findViewById(R.id.site_broken_warning_text);

        mReportBrokenSiteLayout = mPopupView.findViewById(R.id.brave_shields_report_site_layout_id);
        mThankYouLayout = mPopupView.findViewById(R.id.brave_shields_thank_you_layout_id);
        mPrivacyReportLayout = mPopupView.findViewById(R.id.brave_shields_privacy_report_layout_id);

        mBottomDivider = mToggleLayout.findViewById(R.id.bottom_divider);
        mToggleIcon = mToggleLayout.findViewById(R.id.toggle_favicon);
    }

    private void setUpMainLayout() {
        if (mContext == null) return;

        String favIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(mTabId);
        Tab currentActiveTab = mIconFetcher.getTab();
        String url = currentActiveTab.getUrl().getSpec();
        final String favicon_url = (favIconURL.isEmpty()) ? url : favIconURL;
        mIconFetcher.retrieveLargeIcon(favicon_url, this);

        TextView mSiteText = mMainLayout.findViewById(R.id.site_text);
        mSiteText.setText(mTitle.replaceFirst("^(http[s]?://www\\.|http[s]?://|www\\.)", ""));

        SwitchCompat mShieldMainSwitch = mMainLayout.findViewById(R.id.site_switch);

        ImageView helpImage = (ImageView) mMainLayout.findViewById(R.id.help);
        ImageView shareImage = (ImageView) mMainLayout.findViewById(R.id.share);

        helpImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mMainLayout.setVisibility(View.GONE);
                mAboutLayout.setVisibility(View.VISIBLE);
                setUpAboutLayout();
            }
        });

        shareImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mMainLayout.setVisibility(View.GONE);
                if (BraveStatsUtil.hasWritePermission(BraveActivity.getBraveActivity())) {
                    BraveStatsUtil.shareStats(R.layout.brave_stats_share_layout);
                }
            }
        });

        mToggleIcon.setColorFilter(mContext.getResources().getColor(R.color.shield_toggle_button_tint));
        mToggleLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setToggleView(!mSecondaryLayout.isShown());
            }
        });

        ImageView mPrivacyReportIcon = mPrivacyReportLayout.findViewById(R.id.toggle_favicon);
        mPrivacyReportIcon.setImageResource(R.drawable.ic_arrow_forward);
        mPrivacyReportIcon.setColorFilter(
                mContext.getResources().getColor(R.color.default_icon_color_tint_list));
        TextView mViewPrivacyReportText = mPrivacyReportLayout.findViewById(R.id.toggle_text);
        mViewPrivacyReportText.setText(R.string.view_full_privacy_report);
        mPrivacyReportLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                BraveStatsUtil.showBraveStats();
                hideBraveShieldsMenu();
            }
        });
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            mPrivacyReportLayout.setVisibility(View.VISIBLE);
        } else {
            mPrivacyReportLayout.setVisibility(View.GONE);
        }

        setUpSecondaryLayout();

        setupMainSwitchClick(mShieldMainSwitch);
    }

    private void shareStats() {
        View shareStatsLayout = BraveStatsUtil.getLayout(R.layout.brave_stats_share_layout);
        BraveStatsUtil.updateBraveShareStatsLayoutAndShare(shareStatsLayout);
    }

    private void setToggleView(boolean shouldShow) {
        if (shouldShow) {
            mSecondaryLayout.setVisibility(View.VISIBLE);
            mBottomDivider.setVisibility(View.VISIBLE);
            mToggleIcon.setImageResource(R.drawable.ic_toggle_up);
        } else {
            mSecondaryLayout.setVisibility(View.GONE);
            mBottomDivider.setVisibility(View.GONE);
            mToggleIcon.setImageResource(R.drawable.ic_toggle_down);
        }
    }

    private void setUpSecondaryLayout() {
        TextView shieldsText = mSecondaryLayout.findViewById(R.id.brave_shields_text);
        shieldsText.setText(mTitle.replaceFirst("^(http[s]?://www\\.|http[s]?://|www\\.)", ""));

        setUpSwitchLayouts();

        setupDetailsLayouts();
    }

    private void setupDetailsLayouts() {
        if (mContext == null) return;

        ArrayList<String> detailsLayouts = new ArrayList<>();
        detailsLayouts.add(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS);
        detailsLayouts.add(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING);
        detailsLayouts.add(BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES);

        int layoutId = 0;
        int mSecondaryLayoutId = 0;
        int titleStringId = 0;
        int subtitleStringId = 0;
        int option1StringId = 0;
        int option2StringId = 0;
        int option3StringId = 0;

        for (final String layout : detailsLayouts) {
            switch (layout) {
                case BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS:
                    layoutId = R.id.brave_shields_block_cross_trackers_layout_id;
                    mSecondaryLayoutId = R.id.brave_shields_cross_site_trackers_layout_id;
                    titleStringId = R.string.block_trackers_ads_title;
                    subtitleStringId = R.string.block_trackers_ads_summary;
                    option1StringId = R.string.block_trackers_ads_option_1;
                    option2StringId = R.string.block_trackers_ads_option_2;
                    option3StringId = R.string.block_trackers_ads_option_3;
                    break;
                case BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING:
                    layoutId = R.id.brave_shields_block_fingerprinting_layout_id;
                    mSecondaryLayoutId = R.id.brave_shields_fingerprinting_layout_id;
                    titleStringId = R.string.block_fingerprinting;
                    subtitleStringId = R.string.block_fingerprinting_text;
                    option1StringId = R.string.block_fingerprinting_option_1;
                    option2StringId = R.string.block_fingerprinting_option_2;
                    option3StringId = R.string.block_fingerprinting_option_3;
                    break;
                case BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES:
                    layoutId = R.id.brave_shields_block_cookies_layout_id;
                    mSecondaryLayoutId = R.id.brave_shields_cookies_layout_id;
                    titleStringId = R.string.block_cookies;
                    subtitleStringId = R.string.block_cookies_text;
                    option1StringId = R.string.block_cookies_option_1;
                    option2StringId = R.string.block_cross_site_cookies;
                    option3StringId = R.string.block_cookies_option_3;
                    break;
            }

            LinearLayout mBlockShieldsLayout = mPopupView.findViewById(layoutId);
            TextView mBlockShieldsOptionTitle = mBlockShieldsLayout.findViewById(R.id.option_title);
            mBlockShieldsOptionTitle.setText(titleStringId);
            TextView mBlockShieldsOptionText = mBlockShieldsLayout.findViewById(R.id.option_text);
            mBlockShieldsOptionText.setText(subtitleStringId);
            RadioButton mBlockShieldsOption1 = mBlockShieldsLayout.findViewById(R.id.option1);
            mBlockShieldsOption1.setText(option1StringId);
            RadioButton mBlockShieldsOption2 = mBlockShieldsLayout.findViewById(R.id.option2);
            mBlockShieldsOption2.setText(option2StringId);
            RadioButton mBlockShieldsOption3 = mBlockShieldsLayout.findViewById(R.id.option3);
            mBlockShieldsOption3.setText(option3StringId);
            Button mBlockShieldsDoneButton = mBlockShieldsLayout.findViewById(R.id.done_button);
            mBlockShieldsDoneButton.setOnClickListener(mDoneClickListener);
            ImageView mBlockShieldsBackButton = mBlockShieldsLayout.findViewById(R.id.back_button);
            mBlockShieldsBackButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mBlockShieldsLayout.setVisibility(View.GONE);
                    mMainLayout.setVisibility(View.VISIBLE);
                }
            });

            LinearLayout mShieldsLayout = mSecondaryLayout.findViewById(mSecondaryLayoutId);
            mShieldsLayout.setBackground(null);
            mShieldsLayout.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    mMainLayout.setVisibility(View.GONE);
                    mBlockShieldsLayout.setVisibility(View.VISIBLE);
                }
            });
            ImageView mBlockShieldsIcon = mShieldsLayout.findViewById(R.id.toggle_favicon);
            mBlockShieldsIcon.setImageResource(R.drawable.ic_chevron_right);
            mBlockShieldsIcon.setColorFilter(
                    mContext.getResources().getColor(R.color.default_icon_color_baseline));
            TextView mBlockShieldsText = mShieldsLayout.findViewById(R.id.toggle_text);
            mBlockShieldsText.setText(titleStringId);

            String settingOption =
                    BraveShieldsContentSettings.getShieldsValue(mProfile, mHost, layout);
            if (settingOption.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
                mBlockShieldsOption1.setChecked(true);
            } else if (settingOption.equals(
                               BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE)) {
                mBlockShieldsOption2.setChecked(true);
            } else if (settingOption.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
                mBlockShieldsOption3.setChecked(true);
            }

            RadioGroup mBlockShieldsOptionGroup =
                    mBlockShieldsLayout.findViewById(R.id.options_radio_group);
            mBlockShieldsOptionGroup.setOnCheckedChangeListener(
                    new RadioGroup.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(RadioGroup group, int checkedId) {
                            RadioButton checkedRadioButton =
                                    (RadioButton) group.findViewById(checkedId);
                            boolean isChecked = checkedRadioButton.isChecked();
                            if (isChecked) {
                                if (checkedId == R.id.option1) {
                                    BraveShieldsContentSettings.setShieldsValue(mProfile, mHost,
                                            layout, BraveShieldsContentSettings.BLOCK_RESOURCE,
                                            false);
                                } else if (checkedId == R.id.option2) {
                                    BraveShieldsContentSettings.setShieldsValue(mProfile, mHost,
                                            layout,
                                            BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE,
                                            false);
                                } else if (checkedId == R.id.option3) {
                                    BraveShieldsContentSettings.setShieldsValue(mProfile, mHost,
                                            layout, BraveShieldsContentSettings.ALLOW_RESOURCE,
                                            false);
                                }
                                if (null != mMenuObserver) {
                                    mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                                }
                            }
                        }
                    });
        }
    }

    private void setUpSwitchLayouts() {

        LinearLayout mUpgradeHttpsLayout = mSecondaryLayout.findViewById(R.id.brave_shields_upgrade_https_id);
        TextView mUpgradeHttpsText = mUpgradeHttpsLayout.findViewById(R.id.brave_shields_switch_text);
        mBraveShieldsHTTPSEverywhereSwitch = mUpgradeHttpsLayout.findViewById(R.id.brave_shields_switch);
        mUpgradeHttpsText.setText(R.string.brave_shields_https_everywhere_switch);
        setupHTTPSEverywhereSwitchClick(mBraveShieldsHTTPSEverywhereSwitch);

        LinearLayout mBlockScriptsLayout = mSecondaryLayout.findViewById(R.id.brave_shields_block_scripts_id);
        TextView mBlockScriptsText = mBlockScriptsLayout.findViewById(R.id.brave_shields_switch_text);
        mBraveShieldsBlockingScriptsSwitch = mBlockScriptsLayout.findViewById(R.id.brave_shields_switch);
        mBlockScriptsText.setText(R.string.brave_shields_blocks_scripts_switch);
        setupBlockingScriptsSwitchClick(mBraveShieldsBlockingScriptsSwitch);
    }

    private void setUpAboutLayout() {
        TextView mAboutText = mAboutLayout.findViewById(R.id.about_text);
        mAboutText.setVisibility(View.VISIBLE);
        TextView mOptionTitle = mAboutLayout.findViewById(R.id.option_title);
        mOptionTitle.setText(R.string.about_brave_shields_text);
        TextView mOptionText = mAboutLayout.findViewById(R.id.option_text);
        mOptionText.setVisibility(View.GONE);
        RadioGroup mOptionGroup = mAboutLayout.findViewById(R.id.options_radio_group);
        mOptionGroup.setVisibility(View.GONE);
        Button mDoneButton = mAboutLayout.findViewById(R.id.done_button);
        mDoneButton.setOnClickListener(mDoneClickListener);
        ImageView mBackButton = mAboutLayout.findViewById(R.id.back_button);
        mBackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mAboutLayout.setVisibility(View.GONE);
                mMainLayout.setVisibility(View.VISIBLE);
            }
        });
    }

    private void setUpReportBrokenSiteLayout() {
        TextView mReportSiteUrlText = mReportBrokenSiteLayout.findViewById(R.id.report_site_url);
        mReportSiteUrlText.setText(mTitle);

        Button mCancelButton = mReportBrokenSiteLayout.findViewById(R.id.btn_cancel);
        mCancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                hideBraveShieldsMenu();
            }
        });

        Button mSubmitButton = mReportBrokenSiteLayout.findViewById(R.id.btn_submit);
        mSubmitButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                BraveShieldsUtils.BraveShieldsWorkerTask mWorkerTask = new BraveShieldsUtils.BraveShieldsWorkerTask(mTitle);
                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                mReportBrokenSiteLayout.setVisibility(View.GONE);
                mThankYouLayout.setVisibility(View.VISIBLE);
            }
        });
    }

    private void setUpMainSwitchLayout(boolean isChecked) {
        if (mContext == null) return;

        TextView mShieldDownText = mMainLayout.findViewById(R.id.shield_down_text);
        Button mReportBrokenSiteButton = mMainLayout.findViewById(R.id.btn_report_broken_site);
        mReportBrokenSiteButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mMainLayout.setVisibility(View.GONE);
                mReportBrokenSiteLayout.setVisibility(View.VISIBLE);
                setUpReportBrokenSiteLayout();
            }
        });

        LinearLayout mSiteBlockLayout = mMainLayout.findViewById(R.id.site_block_layout);
        TextView mSiteBrokenWarningText = mMainLayout.findViewById(R.id.site_broken_warning_text);

        TextView mShieldsUpText = mMainLayout.findViewById(R.id.shield_up_text);
        String mBraveShieldsText = mContext.getResources().getString(R.string.brave_shields_onboarding_title);

        if (isChecked) {
            mShieldDownText.setVisibility(View.GONE);
            mReportBrokenSiteButton.setVisibility(View.GONE);

            mSiteBlockLayout.setVisibility(View.VISIBLE);
            mSiteBrokenWarningText.setVisibility(View.VISIBLE);
            mToggleLayout.setVisibility(View.VISIBLE);

            String mUpText = mContext.getResources().getString(R.string.up);
            SpannableString mSpanString = new SpannableString(mBraveShieldsText + " " + mUpText);
            mSpanString.setSpan(new StyleSpan(Typeface.BOLD), mSpanString.length() - mUpText.length(), mSpanString.length(), 0);
            mShieldsUpText.setText(mSpanString);
        } else {
            mShieldDownText.setVisibility(View.VISIBLE);
            mReportBrokenSiteButton.setVisibility(View.VISIBLE);

            mSiteBlockLayout.setVisibility(View.GONE);
            mSiteBrokenWarningText.setVisibility(View.GONE);
            mToggleLayout.setVisibility(View.GONE);
            setToggleView(false);

            String mDownText = mContext.getResources().getString(R.string.down);
            SpannableString mSpanString = new SpannableString(mBraveShieldsText + " " + mDownText);
            mSpanString.setSpan(new StyleSpan(Typeface.BOLD), mSpanString.length() - mDownText.length(), mSpanString.length(), 0);
            mShieldsUpText.setText(mSpanString);
        }
    }

    private void setUpViews() {
        boolean isNightMode = GlobalNightModeStateProviderHolder.getInstance().isInNightMode();

        initViews();

        setUpMainLayout();
    }

    private void setupHTTPSEverywhereSwitchClick(SwitchCompat braveShieldsHTTPSEverywhereSwitch) {
        if (null == braveShieldsHTTPSEverywhereSwitch) {
            return;
        }
        setupHTTPSEverywhereSwitch(braveShieldsHTTPSEverywhereSwitch, false);

        mBraveShieldsHTTPSEverywhereChangeListener = new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES, isChecked, false);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                    }
                }
            }
        };

        braveShieldsHTTPSEverywhereSwitch.setOnCheckedChangeListener(mBraveShieldsHTTPSEverywhereChangeListener);
    }

    private void setupHTTPSEverywhereSwitch(
            SwitchCompat braveShieldsHTTPSEverywhereSwitch, boolean fromTopSwitch) {
        if (null == braveShieldsHTTPSEverywhereSwitch) {
            return;
        }
        if (fromTopSwitch) {
            // Prevents to fire an event when top shields changed
            braveShieldsHTTPSEverywhereSwitch.setOnCheckedChangeListener(null);
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES)) {
                    braveShieldsHTTPSEverywhereSwitch.setChecked(true);
                } else {
                    braveShieldsHTTPSEverywhereSwitch.setChecked(false);
                }
                braveShieldsHTTPSEverywhereSwitch.setEnabled(true);
            } else {
                braveShieldsHTTPSEverywhereSwitch.setChecked(false);
                braveShieldsHTTPSEverywhereSwitch.setEnabled(false);
            }
        }
        if (fromTopSwitch) {
            braveShieldsHTTPSEverywhereSwitch.setOnCheckedChangeListener(mBraveShieldsHTTPSEverywhereChangeListener);
        }
    }

    private void setupBlockingScriptsSwitchClick(SwitchCompat braveShieldsBlockingScriptsSwitch) {
        if (null == braveShieldsBlockingScriptsSwitch) {
            return;
        }
        setupBlockingScriptsSwitch(braveShieldsBlockingScriptsSwitch, false);

        mBraveShieldsBlockingScriptsChangeListener = new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS, isChecked, false);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                    }
                }
            }
        };

        braveShieldsBlockingScriptsSwitch.setOnCheckedChangeListener(mBraveShieldsBlockingScriptsChangeListener);
    }

    private void setupBlockingScriptsSwitch(
            SwitchCompat braveShieldsBlockingScriptsSwitch, boolean fromTopSwitch) {
        if (null == braveShieldsBlockingScriptsSwitch) {
            return;
        }
        if (fromTopSwitch) {
            // Prevents to fire an event when top shields changed
            braveShieldsBlockingScriptsSwitch.setOnCheckedChangeListener(null);
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
                    braveShieldsBlockingScriptsSwitch.setChecked(true);
                } else {
                    braveShieldsBlockingScriptsSwitch.setChecked(false);
                }
                braveShieldsBlockingScriptsSwitch.setEnabled(true);
            } else {
                braveShieldsBlockingScriptsSwitch.setChecked(false);
                braveShieldsBlockingScriptsSwitch.setEnabled(false);
            }
        }
        if (fromTopSwitch) {
            braveShieldsBlockingScriptsSwitch.setOnCheckedChangeListener(mBraveShieldsBlockingScriptsChangeListener);
        }
    }

    private void setupMainSwitchClick(SwitchCompat braveShieldsSwitch) {
        if (null == braveShieldsSwitch) {
            return;
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                braveShieldsSwitch.setChecked(true);
                setUpMainSwitchLayout(true);
            } else {
                braveShieldsSwitch.setChecked(false);
                setUpMainSwitchLayout(false);
            }
        }
        braveShieldsSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS, isChecked, false);
                    setupHTTPSEverywhereSwitch(mBraveShieldsHTTPSEverywhereSwitch, true);
                    setupBlockingScriptsSwitch(mBraveShieldsBlockingScriptsSwitch, true);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, true);
                    }
                }

                setUpMainSwitchLayout(isChecked);
            }
        });
    }

    @Override
    public void onLargeIconReady(Bitmap icon) {
        SetFavIcon(icon);
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null && mContext != null) {
            ((Activity)mContext).runOnUiThread(
            new Runnable() {
                @Override
                public void run() {
                    ImageView iv = (ImageView) mPopupView.findViewById(R.id.site_favicon);
                    if (iv != null) iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(bmp));
                }
            });
        }
    }

    private View.OnClickListener mDoneClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            hideBraveShieldsMenu();
        }
    };
}
