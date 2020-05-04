/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.view.Menu;
import android.view.View;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ImageSpan;
import android.widget.PopupMenu;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.view.ContextThemeWrapper;
import android.graphics.Point;
import android.graphics.Rect;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.graphics.drawable.Drawable;
import android.view.MenuItem;
import android.view.LayoutInflater;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.os.Build;
import android.view.ViewGroup;
import android.view.Surface;
import android.widget.TextView;
import android.widget.Switch;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.view.Gravity;
import android.view.MotionEvent;
import android.widget.LinearLayout;
import android.widget.ImageView;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.animation.AnimatorListenerAdapter;
import android.view.animation.TranslateAnimation;
import android.graphics.Bitmap;

import org.chromium.base.SysUtils;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.base.AnimationFrameTimeHistogram;
import org.chromium.chrome.browser.appmenu.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
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

    private final static float LAST_ITEM_SHOW_FRACTION = 0.5f;

    private final Context mContext;
    private final int mMenuResourceId;
    private PopupWindow mPopup;
    private AnimatorSet mMenuItemEnterAnimator;
    private AnimatorListener mAnimationHistogramRecorder = AnimationFrameTimeHistogram
            .getAnimatorRecorder("WrenchMenu.OpeningAnimationFrameTimes");
    private BraveShieldsMenuObserver mMenuObserver;
    private final View mHardwareButtonMenuAnchor;
    private final Map<Integer, BlockersInfo> mTabsStat =
            Collections.synchronizedMap(new HashMap<Integer, BlockersInfo>());

    private Switch mBraveShieldsBlockTrackersSwitch;
    private OnCheckedChangeListener mBraveShieldsAdsTrackingChangeListener;
    private Switch mBraveShieldsHTTPSEverywhereSwitch;
    private OnCheckedChangeListener mBraveShieldsHTTPSEverywhereChangeListener;
    private Switch mBraveShieldsBlockingScriptsSwitch;
    private OnCheckedChangeListener mBraveShieldsBlockingScriptsChangeListener;

    private LinearLayout secondaryLayout;
    private View popupView;
    private TextView siteBlockCounterText;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private BraveRewardsHelper mIconFetcher;

    private String mHost;
    private String mTitle;
    private int mTabId;
    private Profile mProfile;

    /**
     * Constructs a BraveShieldsHandler object.
     * @param context Context that is using the BraveShieldsMenu.
     * @param menuResourceId Resource Id that should be used as the source for the menu items.
     */
    public BraveShieldsHandler(Context context, int menuResourceId) {
        mContext = context;
        mMenuResourceId = menuResourceId;
        mHardwareButtonMenuAnchor = ((Activity)mContext).findViewById(R.id.menu_anchor_stub);
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

    public void show(View anchorView, String host, String title, int tabId,
            Profile profile) {

        mHost = host;
        mTitle = title;
        mTabId = tabId;
        mProfile = profile;

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mIconFetcher = new BraveRewardsHelper();

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

        LayoutInflater inflater = (LayoutInflater) anchorView.getContext().getSystemService(anchorView.getContext().LAYOUT_INFLATER_SERVICE);
        popupView = inflater.inflate(R.layout.brave_shields_main_layout, null);

        setUpViews(popupView);

        //Specify the length and width through constants
        int width = LinearLayout.LayoutParams.WRAP_CONTENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;

        //Make Inactive Items Outside Of PopupWindow
        boolean focusable = true;

        //Create a window with our parameters
        mPopup = new PopupWindow(popupView, width, height, focusable);
        mPopup.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mPopup.setElevation(20);
        }
        // mPopup.setBackgroundDrawable(mContext.getResources().getDrawable(android.R.drawable.picture_frame));
        //Set the location of the window on the screen
        mPopup.showAsDropDown(anchorView, 0, 0);
        mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
        mPopup.setAnimationStyle(R.style.OverflowMenuAnim);

        // Turn off window animations for low end devices, and on Android M, which has built-in menu
        // animations.
        if (SysUtils.isLowEndDevice() || Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mPopup.setAnimationStyle(0);
        }

        Rect bgPadding = new Rect();
        mPopup.getBackground().getPadding(bgPadding);

        int popupWidth = wrapper.getResources().getDimensionPixelSize(R.dimen.menu_width)
                + bgPadding.left + bgPadding.right;

        mPopup.setWidth(popupWidth);

        //Handler for clicking on the inactive zone of the window
        // popupView.setOnTouchListener(new View.OnTouchListener() {
        //     @Override
        //     public boolean onTouch(View v, MotionEvent event) {

        //         //Close the window when clicked
        //         mPopup.dismiss();
        //         return true;
        //     }
        // });

        updateValues(mTabId);
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

    public void updateValues(int adsAndTrackers, int httpsUpgrades, int scriptsBlocked, int fingerprintsBlocked) {
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
                    siteBlockCounterText.setText(String.valueOf(fadsAndTrackers 
                        + fhttpsUpgrades 
                        + fscriptsBlocked 
                        + ffingerprintsBlocked));
                }
                catch (NullPointerException exc) {
                    // It means that the Bravery Panel was destroyed during the update, we just do nothing
                }
            }
        });
    }

    public boolean isShowing() {
        if (null == mPopup) {
            return false;
        }

        return mPopup.isShowing();
    }

    public void hideBraveShieldsMenu() {
        if (isShowing()) {
            mPopup.dismiss();
            // mAdapter = null;
        }
    }

    private void setUpViews(View popupView) {
        boolean isNightMode = GlobalNightModeStateProviderHolder.getInstance().isInNightMode();
        LinearLayout mainLayout = popupView.findViewById(R.id.main_layout);
        Switch shieldMainSwitch = popupView.findViewById(R.id.site_switch);
        TextView siteText = popupView.findViewById(R.id.site_text);
        siteText.setText(mTitle);

        siteBlockCounterText = popupView.findViewById(R.id.site_block_count_text);

        final LinearLayout aboutLayout = popupView.findViewById(R.id.brave_shields_about_layout_id);

        final View.OnClickListener doneClickListener = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mPopup.dismiss();
            }
        };

        ClickableSpan clickableSpan = new ClickableSpan() {
            @Override
            public void onClick(View widget) {
                mainLayout.setVisibility(View.GONE);
                aboutLayout.setVisibility(View.VISIBLE);
                TextView aboutText = aboutLayout.findViewById(R.id.about_text);
                aboutText.setVisibility(View.VISIBLE);
                TextView optionTitle = aboutLayout.findViewById(R.id.option_title);
                optionTitle.setText(R.string.about_brave_shields_text);
                TextView optionText = aboutLayout.findViewById(R.id.option_text);
                optionText.setVisibility(View.GONE);
                RadioGroup optionGroup = aboutLayout.findViewById(R.id.options_radio_group);
                optionGroup.setVisibility(View.GONE);
                Button doneButton = aboutLayout.findViewById(R.id.done_button);
                doneButton.setOnClickListener(doneClickListener);
            }
        };

        TextView siteBlockText = popupView.findViewById(R.id.site_block_text);
        siteBlockText.setMovementMethod(LinkMovementMethod.getInstance());
        String text = mContext.getResources().getString(R.string.ads_and_other_things_blocked) + " ";
        SpannableString ss = new SpannableString(text);
        ImageSpan imageSpan = new ImageSpan(mContext, R.drawable.help);
        ss.setSpan(imageSpan, text.length() - 1, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        ss.setSpan(clickableSpan, ss.getSpanStart(imageSpan), ss.getSpanEnd(imageSpan), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        siteBlockText.setText(ss);

        secondaryLayout = popupView.findViewById(R.id.brave_shields_secondary_layout);
        TextView shieldsText = popupView.findViewById(R.id.brave_shields_text);
        shieldsText.setText(mTitle);

        LinearLayout toggleLayout = popupView.findViewById(R.id.brave_shields_toggle_layout_id);
        ImageView toggleIcon = toggleLayout.findViewById(R.id.site_favicon);
        toggleIcon.setColorFilter(mContext.getResources().getColor(R.color.shield_toggle_button_tint));
        toggleLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(!secondaryLayout.isShown()) {
                    secondaryLayout.setVisibility(View.VISIBLE);
                    toggleIcon.setImageResource(R.drawable.arrows_chevron_up);
                } else {
                    secondaryLayout.setVisibility(View.GONE);
                    toggleIcon.setImageResource(R.drawable.arrows_chevron_down);
                }
            }
        });

        LinearLayout blockTrackersLayout = popupView.findViewById(R.id.brave_shields_block_trackers_id);
        TextView blockTrackersText = blockTrackersLayout.findViewById(R.id.brave_shields_switch_text);
        mBraveShieldsBlockTrackersSwitch = blockTrackersLayout.findViewById(R.id.brave_shields_switch);
        blockTrackersText.setText(R.string.brave_shields_ads_and_trackers);
        setupAdsTrackingSwitchClick(mBraveShieldsBlockTrackersSwitch);

        LinearLayout upgradeHttpsLayout = popupView.findViewById(R.id.brave_shields_upgrade_https_id);
        TextView upgradeHttpsText = upgradeHttpsLayout.findViewById(R.id.brave_shields_switch_text);
        mBraveShieldsHTTPSEverywhereSwitch = upgradeHttpsLayout.findViewById(R.id.brave_shields_switch);
        upgradeHttpsText.setText(R.string.brave_shields_https_everywhere_switch);
        setupHTTPSEverywhereSwitchClick(mBraveShieldsHTTPSEverywhereSwitch);

        LinearLayout blockScriptsLayout = popupView.findViewById(R.id.brave_shields_block_scripts_id);
        TextView blockScriptsText = blockScriptsLayout.findViewById(R.id.brave_shields_switch_text);
        mBraveShieldsBlockingScriptsSwitch = blockScriptsLayout.findViewById(R.id.brave_shields_switch);
        blockScriptsText.setText(R.string.brave_shields_blocks_scripts_switch);
        setupBlockingScriptsSwitchClick(mBraveShieldsBlockingScriptsSwitch);

        LinearLayout blockCookiesLayout = popupView.findViewById(R.id.brave_shields_block_cookies_layout_id);
        TextView cookiesOptionTitle = blockCookiesLayout.findViewById(R.id.option_title);
        cookiesOptionTitle.setText(R.string.block_cookies);
        TextView cookiesOptionText = blockCookiesLayout.findViewById(R.id.option_text);
        cookiesOptionText.setText(R.string.block_cookies_text);
        RadioButton cookiesOption1 = blockCookiesLayout.findViewById(R.id.option1);
        cookiesOption1.setText(R.string.block_cookies_option_1);
        RadioButton cookiesOption2 = blockCookiesLayout.findViewById(R.id.option2);
        cookiesOption2.setText(R.string.block_cross_site_cookies);
        RadioButton cookiesOption3 = blockCookiesLayout.findViewById(R.id.option3);
        cookiesOption3.setText(R.string.block_cookies_option_3);
        Button cookiesDoneButton = blockCookiesLayout.findViewById(R.id.done_button);
        cookiesDoneButton.setOnClickListener(doneClickListener);

        LinearLayout cookiesLayout = popupView.findViewById(R.id.brave_shields_cookies_layout_id);
        cookiesLayout.setBackground(null);
        cookiesLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mainLayout.setVisibility(View.GONE);
                blockCookiesLayout.setVisibility(View.VISIBLE);
            }
        });
        ImageView cookiesIcon = cookiesLayout.findViewById(R.id.site_favicon);
        cookiesIcon.setImageResource(R.drawable.chevron_right);
        cookiesIcon.setColorFilter(mContext.getResources().getColor(R.color.standard_mode_tint));
        TextView cookiesText = cookiesLayout.findViewById(R.id.site_text);
        cookiesText.setText(R.string.block_cross_site_cookies);

        LinearLayout blockFingerPrintingLayout = popupView.findViewById(R.id.brave_shields_block_fingerprinting_layout_id);
        TextView fingerprintingOptionTitle = blockFingerPrintingLayout.findViewById(R.id.option_title);
        fingerprintingOptionTitle.setText(R.string.block_fingerprinting);
        TextView fingerprintingOptionText = blockFingerPrintingLayout.findViewById(R.id.option_text);
        fingerprintingOptionText.setText(R.string.block_fingerprinting_text);
        RadioButton fingerprintingOption1 = blockFingerPrintingLayout.findViewById(R.id.option1);
        fingerprintingOption1.setText(R.string.block_fingerprinting_option_1);
        RadioButton fingerprintingOption2 = blockFingerPrintingLayout.findViewById(R.id.option2);
        fingerprintingOption2.setText(R.string.block_cross_site_fingerprinting);
        RadioButton fingerprintingOption3 = blockFingerPrintingLayout.findViewById(R.id.option3);
        fingerprintingOption3.setText(R.string.block_fingerprinting_option_3);
        Button fingerprintingDoneButton = blockFingerPrintingLayout.findViewById(R.id.done_button);
        fingerprintingDoneButton.setOnClickListener(doneClickListener);

        LinearLayout fingerPrintingLayout = popupView.findViewById(R.id.brave_shields_fingerprinting_layout_id);
        fingerPrintingLayout.setBackground(null);
        fingerPrintingLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mainLayout.setVisibility(View.GONE);
                blockFingerPrintingLayout.setVisibility(View.VISIBLE);
            }
        });
        ImageView fingerPrintingIcon = fingerPrintingLayout.findViewById(R.id.site_favicon);
        fingerPrintingIcon.setImageResource(R.drawable.chevron_right);
        fingerPrintingIcon.setColorFilter(mContext.getResources().getColor(R.color.standard_mode_tint));
        TextView fingerPrintingText = fingerPrintingLayout.findViewById(R.id.site_text);
        fingerPrintingText.setText(R.string.block_cross_site_fingerprinting);

        setupMainSwitchClick(shieldMainSwitch);

        String favIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(mTabId);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveTab();
        String url = currentActiveTab.getUrl();
        final String favicon_url = (favIconURL.isEmpty()) ? url : favIconURL;
        mIconFetcher.retrieveLargeIcon(favicon_url,this);
    }

    private void setupAdsTrackingSwitchClick(Switch braveShieldsAdsTrackingSwitch) {
        if (null == braveShieldsAdsTrackingSwitch) {
            return;
        }
        setupAdsTrackingSwitch(braveShieldsAdsTrackingSwitch, false);

        mBraveShieldsAdsTrackingChangeListener = new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
              boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS_TRACKERS, isChecked, false);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                    }
                }
            }
        };

        braveShieldsAdsTrackingSwitch.setOnCheckedChangeListener(mBraveShieldsAdsTrackingChangeListener);
    }

    private void setupAdsTrackingSwitch(Switch braveShieldsAdsTrackingSwitch, boolean fromTopSwitch) {
        if (null == braveShieldsAdsTrackingSwitch) {
            return;
        }
        if (fromTopSwitch) {
            // Prevents to fire an event when top shields changed
            braveShieldsAdsTrackingSwitch.setOnCheckedChangeListener(null);
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS_TRACKERS)) {
                    braveShieldsAdsTrackingSwitch.setChecked(true);
                } else {
                    braveShieldsAdsTrackingSwitch.setChecked(false);
                }
                braveShieldsAdsTrackingSwitch.setEnabled(true);
            } else {
                 braveShieldsAdsTrackingSwitch.setChecked(false);
                 braveShieldsAdsTrackingSwitch.setEnabled(false);
            }
        }
        if (fromTopSwitch) {
            braveShieldsAdsTrackingSwitch.setOnCheckedChangeListener(mBraveShieldsAdsTrackingChangeListener);
        }
    }

    private void setupHTTPSEverywhereSwitchClick(Switch braveShieldsHTTPSEverywhereSwitch) {
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

    private void setupHTTPSEverywhereSwitch(Switch braveShieldsHTTPSEverywhereSwitch, boolean fromTopSwitch) {
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

    private void setupBlockingScriptsSwitchClick(Switch braveShieldsBlockingScriptsSwitch) {
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

    private void setupBlockingScriptsSwitch(Switch braveShieldsBlockingScriptsSwitch, boolean fromTopSwitch) {
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

    private void setupMainSwitchClick(Switch braveShieldsSwitch) {
        if (null == braveShieldsSwitch) {
            return;
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                braveShieldsSwitch.setChecked(true);
            } else {
                braveShieldsSwitch.setChecked(false);
            }
        }
        braveShieldsSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
              boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS, isChecked, false);
                    setupAdsTrackingSwitch(mBraveShieldsBlockTrackersSwitch, true);
                    setupHTTPSEverywhereSwitch(mBraveShieldsHTTPSEverywhereSwitch, true);
                    setupBlockingScriptsSwitch(mBraveShieldsBlockingScriptsSwitch, true);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, true);
                    }
                }
            }
        });
    }

    @Override
    public void onLargeIconReady(Bitmap icon){
        SetFavIcon(icon);
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null){
            ((Activity)mContext).runOnUiThread(
            new Runnable(){
                @Override
                public void run() {
                    ImageView iv = (ImageView) popupView.findViewById(R.id.site_favicon);
                    iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(bmp));
                }
            });
        }
    }
}