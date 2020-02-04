/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ListPopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.graphics.Color;
import android.widget.Switch;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.util.SparseArray;

import org.chromium.base.Log;
import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.omaha.UpdateMenuItemHelper;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
//import org.chromium.chrome.browser.MixPanelWorker;
import org.chromium.ui.base.LocalizationUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;
import org.chromium.ui.widget.ChromeImageView;

import java.util.List;
import java.lang.NumberFormatException;

/**
 * ListAdapter to customize the view of items in the list.
 */
class BraveShieldsMenuAdapter extends BaseAdapter {
    /**
     * Regular Android menu item that contains a title and an icon if icon is specified.
     */
    private static final int STANDARD_MENU_ITEM = 0;

    /**
     * Menu item that has two buttons, the first one is a title and the second one is an icon.
     * It is different from the regular menu item because it contains two separate buttons.
     */
    private static final int TITLE_BUTTON_MENU_ITEM = 1;

    /**
     * Menu item that has four buttons. Every one of these buttons is displayed as an icon.
     */
    private static final int THREE_BUTTON_MENU_ITEM = 2;

    /**
     * Menu item that has four buttons. Every one of these buttons is displayed as an icon.
     */
    private static final int FOUR_BUTTON_MENU_ITEM = 3;

    /**
     * Menu item for updating Chrome; uses a custom layout.
     */
    private static final int UPDATE_MENU_ITEM = 4;

    /**
     * The number of view types specified above.  If you add a view type you MUST increment this.
     */
    private static final int VIEW_TYPE_COUNT = 5;

    /** MenuItem Animation Constants */
    private static final int ENTER_ITEM_DURATION_MS = 350;
    private static final int ENTER_ITEM_BASE_DELAY_MS = 80;
    private static final int ENTER_ITEM_ADDL_DELAY_MS = 30;
    private static final float ENTER_STANDARD_ITEM_OFFSET_Y_DP = -10.f;
    private static final float ENTER_STANDARD_ITEM_OFFSET_X_DP = 10.f;

    private static final String BRAVE_SHIELDS_GREY = "#F2F2F2";
    private static final String BRAVE_SHIELDS_WHITE = "#FFFFFF";
    private static final String BRAVE_GROUP_TITLE_COLOR = "#6B6B6B";
    private static final String ADS_AND_TRACKERS_COLOR = "#FB542B";
    private static final String HTTPS_UPDATES_COLOR = "#22C976";
    private static final String SCRIPTS_BLOCKED_COLOR = "#8236B9";
    private static final String FINGERPRINTS_BLOCKED_COLOR = "#818589";
    private static final double BRAVE_WEBSITE_TEXT_SIZE_INCREMENT = 1.2;
    private static final double BRAVE_SCREEN_FOR_PANEL = 0.8;
    private static final double BRAVE_MAX_PANEL_TEXT_SIZE_INCREMENT = 1.3;

    private final LayoutInflater mInflater;
    private final List<MenuItem> mMenuItems;
    private final float mDpToPx;
    private BraveShieldsMenuObserver mMenuObserver;
    private SparseArray<View> mPositionViews;

    private ListPopupWindow mPopup;
    private int mCurrentDisplayWidth;
    private String mHost;
    private String mTitle;

    private Switch mBraveShieldsAdsTrackingSwitch;
    private OnCheckedChangeListener mBraveShieldsAdsTrackingChangeListener;
    private Switch mBraveShieldsHTTPSEverywhereSwitch;
    private OnCheckedChangeListener mBraveShieldsHTTPSEverywhereChangeListener;
    private Switch mBraveShieldsBlocking3rdPartyCookiesSwitch;
    private OnCheckedChangeListener mBraveShieldsBlocking3rdPartyCookiesChangeListener;
    private Switch mBraveShieldsBlockingScriptsSwitch;
    private OnCheckedChangeListener mBraveShieldsBlockingScriptsChangeListener;
    private Switch mBraveShieldsFingerprintsSwitch;
    private OnCheckedChangeListener mBraveShieldsFingerprintsChangeListener;
    private Profile mProfile;


    public BraveShieldsMenuAdapter(String host,
            String title,
            List<MenuItem> menuItems,
            LayoutInflater inflater,
            BraveShieldsMenuObserver menuObserver,
            ListPopupWindow popup,
            int currentDisplayWidth,
            Profile profile) {
        mMenuItems = menuItems;
        mInflater = inflater;
        mDpToPx = inflater.getContext().getResources().getDisplayMetrics().density;
        mMenuObserver = menuObserver;
        mPositionViews = new SparseArray<View>();
        mPopup = popup;
        mCurrentDisplayWidth = currentDisplayWidth;
        mHost = host;
        mTitle = title;
        mProfile = profile;
    }

    public void updateHost(String host) {
        mHost = host;
    }

    @Override
    public boolean isEnabled(int position) {
        if (position >= 1 &&  position <= 8) {
            return false;
        }

        return true;
    }

    @Override
    public int getCount() {
        return mMenuItems.size();
    }

    @Override
    public int getViewTypeCount() {
        return VIEW_TYPE_COUNT;
    }

    @Override
    public int getItemViewType(int position) {
        MenuItem item = getItem(position);
        int viewCount = item.hasSubMenu() ? item.getSubMenu().size() : 1;

        if (item.getItemId() == R.id.update_menu_id) {
            return UPDATE_MENU_ITEM;
        } else if (viewCount == 4) {
            return FOUR_BUTTON_MENU_ITEM;
        } else if (viewCount == 3) {
            return THREE_BUTTON_MENU_ITEM;
        } else if (viewCount == 2) {
            return TITLE_BUTTON_MENU_ITEM;
        }
        return STANDARD_MENU_ITEM;
    }

    @Override
    public long getItemId(int position) {
        return getItem(position).getItemId();
    }

    @Override
    public MenuItem getItem(int position) {
        if (position == ListView.INVALID_POSITION) return null;
        assert position >= 0;
        assert position < mMenuItems.size();
        return mMenuItems.get(position);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final MenuItem item = getItem(position);
        StandardMenuItemViewHolder holder = null;
        if (getItemViewType(position) == STANDARD_MENU_ITEM) {
            View mapView = mPositionViews.get(position);
            if (mapView == null
                    || mapView != convertView
                    || !(convertView.getTag() instanceof StandardMenuItemViewHolder)) {
                holder = new StandardMenuItemViewHolder();
                if (0 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_switcher, parent, false);
                    setupSwitchClick((Switch)convertView.findViewById(R.id.brave_shields_switch));
                // We should set layouts for switch rows
                } else if (1 == position || 2 == position || 8 == position || 14 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_text_item, parent, false);
                    TextView text = (TextView) convertView.findViewById(R.id.brave_shields_text);
                    if (text != null) {
                        if (2 == position) {
                            text.setText(R.string.brave_shields_first_group_title);
                        } else if (8 == position) {
                            text.setText(R.string.brave_shields_second_group_title);
                        } else if (1 == position) {
                            text.setTextColor(convertView.getContext().getResources().getColor(
                                    R.color.standard_mode_tint));
                            text.setTextSize(20);
                            text.setText(mTitle);
                        } else {
                            // We need this item only for more space at the bottom of the menu
                            text.setTextSize(1);
                        }
                    }
                } else if (3 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_menu_item, parent, false);
                    TextView text = (TextView) convertView.findViewById(R.id.brave_shields_text);
                    if (text != null) {
                        text.setText(R.string.brave_shields_ads_and_trackers);
                    }
                    TextView number = (TextView) convertView.findViewById(R.id.brave_shields_number);
                    if (number != null) {
                        number.setTextColor(Color.parseColor(ADS_AND_TRACKERS_COLOR));
                        number.setText(item.getTitle());
                        number.setTag(R.string.brave_shields_ads_and_trackers);
                    }
                } else if (4 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_menu_item, parent, false);
                    TextView text = (TextView) convertView.findViewById(R.id.brave_shields_text);
                    if (text != null) {
                        text.setText(R.string.brave_shields_https_upgrades);
                    }
                    TextView number = (TextView) convertView.findViewById(R.id.brave_shields_number);
                    if (number != null) {
                        number.setTextColor(Color.parseColor(HTTPS_UPDATES_COLOR));
                        number.setText(item.getTitle());
                        number.setTag(R.string.brave_shields_https_upgrades);
                    }
                } else if (5 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_menu_item, parent, false);
                    TextView text = (TextView) convertView.findViewById(R.id.brave_shields_text);
                    if (text != null) {
                        text.setText(R.string.brave_shields_scripts_blocked);
                    }
                    TextView number = (TextView) convertView.findViewById(R.id.brave_shields_number);
                    if (number != null) {
                        number.setTextColor(Color.parseColor(SCRIPTS_BLOCKED_COLOR));
                        number.setText(item.getTitle());
                        number.setTag(R.string.brave_shields_scripts_blocked);
                    }
                } else if (6 == position) {
//                    convertView = mInflater.inflate(R.layout.brave_shields_menu_item, parent, false);
                    TextView text = (TextView) convertView.findViewById(R.id.brave_shields_text);
                    if (text != null) {
                        text.setText(R.string.brave_shields_fingerprint_methods);
                    }
                    TextView number = (TextView) convertView.findViewById(R.id.brave_shields_number);
                    if (number != null) {
                        number.setTextColor(Color.parseColor(FINGERPRINTS_BLOCKED_COLOR));
                        number.setText(item.getTitle());
                        number.setTag(R.string.brave_shields_fingerprint_methods);
                    }
                } else if (7 == position) {
                    convertView = mInflater.inflate(R.layout.menu_separator, parent, false);
                } else if (9 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_ads_tracking_switcher, parent, false);
                    mBraveShieldsAdsTrackingSwitch = (Switch)convertView.findViewById(R.id.brave_shields_ads_tracking_switch);
                    setupAdsTrackingSwitchClick(mBraveShieldsAdsTrackingSwitch);
                    // To make it more nice looking
                    /*TextView text = (TextView) convertView.findViewById(R.id.brave_shields_ads_tracking_text);
                    if (text != null && text.getText().toString().indexOf("and") != -1) {
                        String value = text.getText().toString().replaceFirst("and", "&");
                        text.setText(value);
                    }*/
                } else if (10 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_https_upgrade_switcher, parent, false);
                    mBraveShieldsHTTPSEverywhereSwitch = (Switch)convertView.findViewById(R.id.brave_shields_https_upgrade_switch);
                    setupHTTPSEverywhereSwitchClick(mBraveShieldsHTTPSEverywhereSwitch);
                } else if (11 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_3rd_party_cookies_blocked_switcher, parent, false);
                    mBraveShieldsBlocking3rdPartyCookiesSwitch = (Switch)convertView.findViewById(R.id.brave_shields_3rd_party_cookies_blocked_switch);
                    setup3rdPartyCookiesSwitchClick(mBraveShieldsBlocking3rdPartyCookiesSwitch);
                } else if (12 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_scripts_blocked_switcher, parent, false);
                    mBraveShieldsBlockingScriptsSwitch = (Switch)convertView.findViewById(R.id.brave_shields_scripts_blocked_switch);
                    setupBlockingScriptsSwitchClick(mBraveShieldsBlockingScriptsSwitch);
                } else if (13 == position) {
                    convertView = mInflater.inflate(R.layout.brave_shields_fingerprints_blocked_switcher, parent, false);
                    mBraveShieldsFingerprintsSwitch = (Switch)convertView.findViewById(R.id.brave_shields_fingerprints_blocked_switch);
                    setupFingerprintsSwitchClick(mBraveShieldsFingerprintsSwitch);
                    // To make it more nice looking
                    /*TextView text = (TextView) convertView.findViewById(R.id.brave_shields_fingerprinting_blocked_text);
                    if (text != null && text.getText().toString().indexOf(" ") != -1) {
                        String value = text.getText().toString().replaceFirst(" ", "\n");
                        text.setText(value);
                    }*/
                } else {
//                    convertView = mInflater.inflate(R.layout.menu_item, parent, false);
                    holder.text = (TextView) convertView.findViewById(R.id.menu_item_text);
                    //holder.image = (AppMenuItemIcon) convertView.findViewById(R.id.menu_item_icon);
                    convertView.setTag(holder);
                }
//                convertView.setTag(R.id.menu_item_enter_anim_id,
//                        buildStandardItemEnterAnimator(convertView, position));

                mPositionViews.append(position, convertView);
            } else {
                holder = (StandardMenuItemViewHolder) convertView.getTag();
                if (convertView != mapView) {
                    convertView = mapView;
                }
            }

            if (null != holder.text && null != holder.image) {
                setupStandardMenuItemViewHolder(holder, convertView, item);
            }
        } else {
            assert false : "Unexpected MenuItem type";
        }
        if (null != holder) {
            setPopupWidth(holder.text);
        }
        return convertView;
    }

    private void setPopupWidth(TextView view) {
        if (null == view) {
            return;
        }
        Paint textPaint = view.getPaint();
        float textWidth = textPaint.measureText(view.getText().toString());
        int sizeToCheck = (int)(textWidth * BRAVE_MAX_PANEL_TEXT_SIZE_INCREMENT);
        if (sizeToCheck > mCurrentDisplayWidth * BRAVE_SCREEN_FOR_PANEL) {
            mPopup.setWidth((int)(mCurrentDisplayWidth * BRAVE_SCREEN_FOR_PANEL));
            return;
        }
        if (mPopup.getWidth() < sizeToCheck) {
            mPopup.setWidth((int)(sizeToCheck));
        }
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

    private void setupFingerprintsSwitchClick(Switch braveShieldsFingerprintsSwitch) {
        if (null == braveShieldsFingerprintsSwitch) {
            return;
        }
        setupBlockingFingerprintsSwitch(braveShieldsFingerprintsSwitch, false);

        mBraveShieldsFingerprintsChangeListener = new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
              boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING, isChecked, false);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                    }
                }
            }
        };

        braveShieldsFingerprintsSwitch.setOnCheckedChangeListener(mBraveShieldsFingerprintsChangeListener);
    }

    private void setupBlockingFingerprintsSwitch(Switch braveShieldsFingerprintsSwitch, boolean fromTopSwitch) {
        if (null == braveShieldsFingerprintsSwitch) {
            return;
        }
        if (fromTopSwitch) {
            // Prevents to fire an event when top shields changed
            braveShieldsFingerprintsSwitch.setOnCheckedChangeListener(null);
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING)) {
                    braveShieldsFingerprintsSwitch.setChecked(true);
                } else {
                    braveShieldsFingerprintsSwitch.setChecked(false);
                }
                braveShieldsFingerprintsSwitch.setEnabled(true);
            } else {
                 braveShieldsFingerprintsSwitch.setChecked(false);
                 braveShieldsFingerprintsSwitch.setEnabled(false);
            }
        }
        if (fromTopSwitch) {
            braveShieldsFingerprintsSwitch.setOnCheckedChangeListener(mBraveShieldsFingerprintsChangeListener);
        }
    }

    private void setup3rdPartyCookiesSwitchClick(Switch braveShieldsBlocking3rdPartyCookiesSwitch) {
        if (null == braveShieldsBlocking3rdPartyCookiesSwitch) {
            return;
        }
        setupBlocking3rdPartyCookiesSwitch(braveShieldsBlocking3rdPartyCookiesSwitch, false);

        mBraveShieldsBlocking3rdPartyCookiesChangeListener = new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
              boolean isChecked) {
                if (0 != mHost.length()) {
                    BraveShieldsContentSettings.setShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES, isChecked, false);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, false);
                    }
                }
            }
        };

        braveShieldsBlocking3rdPartyCookiesSwitch.setOnCheckedChangeListener(mBraveShieldsBlocking3rdPartyCookiesChangeListener);
    }

    private void setupBlocking3rdPartyCookiesSwitch(Switch braveShieldsBlocking3rdPartyCookiesSwitch, boolean fromTopSwitch) {
        if (null == braveShieldsBlocking3rdPartyCookiesSwitch) {
            return;
        }
        if (fromTopSwitch) {
            // Prevents to fire an event when top shields changed
            braveShieldsBlocking3rdPartyCookiesSwitch.setOnCheckedChangeListener(null);
        }
        if (0 != mHost.length()) {
            if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
                if (BraveShieldsContentSettings.getShields(mProfile, mHost, BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES)) {
                    braveShieldsBlocking3rdPartyCookiesSwitch.setChecked(true);
                } else {
                    braveShieldsBlocking3rdPartyCookiesSwitch.setChecked(false);
                }
                braveShieldsBlocking3rdPartyCookiesSwitch.setEnabled(true);
            } else {
                 braveShieldsBlocking3rdPartyCookiesSwitch.setChecked(false);
                 braveShieldsBlocking3rdPartyCookiesSwitch.setEnabled(false);
            }
        }
        if (fromTopSwitch) {
            braveShieldsBlocking3rdPartyCookiesSwitch.setOnCheckedChangeListener(mBraveShieldsBlocking3rdPartyCookiesChangeListener);
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

    private void setupSwitchClick(Switch braveShieldsSwitch) {
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
                    setupAdsTrackingSwitch(mBraveShieldsAdsTrackingSwitch, true);
                    setupHTTPSEverywhereSwitch(mBraveShieldsHTTPSEverywhereSwitch, true);
                    setupBlockingScriptsSwitch(mBraveShieldsBlockingScriptsSwitch, true);
                    setupBlocking3rdPartyCookiesSwitch(mBraveShieldsBlocking3rdPartyCookiesSwitch, true);
                    setupBlockingFingerprintsSwitch(mBraveShieldsFingerprintsSwitch, true);
                    if (null != mMenuObserver) {
                        mMenuObserver.onMenuTopShieldsChanged(isChecked, true);
                    }
                }
            }
        });
    }

    private void setupStandardMenuItemViewHolder(StandardMenuItemViewHolder holder,
            View convertView, final MenuItem item) {
        // Set up the icon.
        Drawable icon = item.getIcon();
        holder.image.setImageDrawable(icon);
        holder.image.setVisibility(icon == null ? View.GONE : View.VISIBLE);
//        holder.image.setChecked(item.isChecked());
        holder.text.setText(item.getTitle());
        holder.text.setContentDescription(item.getTitleCondensed());

        boolean isEnabled = item.isEnabled();
        // Set the text color (using a color state list).
        holder.text.setEnabled(isEnabled);
        // This will ensure that the item is not highlighted when selected.
        convertView.setEnabled(isEnabled);

        /*convertView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mAppMenu.onItemClick(item);
            }
        });*/
    }

    /**
     * This builds an {@link Animator} for the enter animation of a standard menu item.  This means
     * it will animate the alpha from 0 to 1 and translate the view from -10dp to 0dp on the y axis.
     *
     * @param view     The menu item {@link View} to be animated.
     * @param position The position in the menu.  This impacts the start delay of the animation.
     * @return         The {@link Animator}.
     */
    private Animator buildStandardItemEnterAnimator(final View view, int position) {
        final float offsetYPx = ENTER_STANDARD_ITEM_OFFSET_Y_DP * mDpToPx;
        final int startDelay = ENTER_ITEM_BASE_DELAY_MS + ENTER_ITEM_ADDL_DELAY_MS * position;

        AnimatorSet animation = new AnimatorSet();
        animation.playTogether(
                ObjectAnimator.ofFloat(view, View.ALPHA, 0.f, 1.f),
                ObjectAnimator.ofFloat(view, View.TRANSLATION_Y, offsetYPx, 0.f));
        animation.setDuration(ENTER_ITEM_DURATION_MS);
        animation.setStartDelay(startDelay);
        animation.setInterpolator(BakedBezierInterpolator.FADE_IN_CURVE);

        animation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                view.setAlpha(0.f);
            }
        });
        return animation;
    }

    /**
     * This builds an {@link Animator} for the enter animation of icon row menu items.  This means
     * it will animate the alpha from 0 to 1 and translate the views from 10dp to 0dp on the x axis.
     *
     * @param views        The list if icons in the menu item that should be animated.
     * @return             The {@link Animator}.
     */
    private Animator buildIconItemEnterAnimator(final ImageView[] views) {
        final boolean rtl = LocalizationUtils.isLayoutRtl();
        final float offsetXPx = ENTER_STANDARD_ITEM_OFFSET_X_DP * mDpToPx * (rtl ? -1.f : 1.f);
        final int maxViewsToAnimate = views.length;

        AnimatorSet animation = new AnimatorSet();
        AnimatorSet.Builder builder = null;
        for (int i = 0; i < maxViewsToAnimate; i++) {
            final int startDelay = ENTER_ITEM_ADDL_DELAY_MS * i;

            Animator alpha = ObjectAnimator.ofFloat(views[i], View.ALPHA, 0.f, 1.f);
            Animator translate = ObjectAnimator.ofFloat(views[i], View.TRANSLATION_X, offsetXPx, 0);
            alpha.setStartDelay(startDelay);
            translate.setStartDelay(startDelay);
            alpha.setDuration(ENTER_ITEM_DURATION_MS);
            translate.setDuration(ENTER_ITEM_DURATION_MS);

            if (builder == null) {
                builder = animation.play(alpha);
            } else {
                builder.with(alpha);
            }
            builder.with(translate);
        }
        animation.setStartDelay(ENTER_ITEM_BASE_DELAY_MS);
        animation.setInterpolator(BakedBezierInterpolator.FADE_IN_CURVE);

        animation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                for (int i = 0; i < maxViewsToAnimate; i++) {
                    views[i].setAlpha(0.f);
                }
            }
        });
        return animation;
    }

    static class StandardMenuItemViewHolder {
        public TextView text;
        public /*AppMenuItemIcon*/ChromeImageView image;
    }

    static class CustomMenuItemViewHolder extends StandardMenuItemViewHolder {
        public TextView summary;
    }
}
