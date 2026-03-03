/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.AutoCompleteTextView;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

import com.google.android.material.textfield.TextInputEditText;

import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabFavicon;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.url.GURL;
import org.chromium.webcompat_reporter.mojom.WebcompatReporterHandler;

/**
 * Handler for the unified Shields panel. Displays shields controls, advanced options, and
 * sub-panels for detailed settings (HTTPS, trackers, cookies, shred, report).
 */
@NullMarked
public class BraveUnifiedPanelHandler {
    private static final String TAG = "BraveUnifiedPanel";

    private final @Nullable Context mContext;
    private @Nullable PopupWindow mPopupWindow;
    private @Nullable View mPopupView;
    private @Nullable View mAnchorView;
    private @Nullable View mHardwareButtonMenuAnchor;
    private @Nullable GURL mUrl;
    private View mMainPanelContainer;
    private View mHttpsPanelContainer;
    private View mTrackersPanelContainer;
    private View mCookiesPanelContainer;
    private View mShredPanelContainer;
    private View mReportBrokenSitePanelContainer;

    // Shields content elements
    private ImageView mShieldIconUnified;
    private TextView mSiteTextUnified;
    private TextView mShieldsStatusText;
    private ImageView mShieldsToggleSwitch;
    private TextView mBlockCountNumber;
    private TextView mBlockCountText;
    private LinearLayout mBlockedItemsContainer;
    private @Nullable FaviconHelper mFaviconHelper;
    private LinearLayout mAdvancedOptionsButton;
    private @Nullable LinearLayout mAdvancedOptionsContent;
    private @Nullable ImageView mAdvancedOptionsArrow;
    private boolean mIsAdvancedOptionsExpanded;

    // Advanced options switches (no Forget Me in new design)
    private ImageView mBlockScriptsSwitch;
    private ImageView mFingerprintingSwitch;
    private boolean mIsUpdatingSwitches;

    // Observer for notifying toolbar of shields changes (icon update, page reload)
    private @Nullable BraveShieldsMenuObserver mMenuObserver;

    // Report broken site section
    private LinearLayout mReportBrokenSiteSection;
    private android.widget.Button mReportBrokenSiteButton;

    // Report broken site panel
    private @Nullable AutoCompleteTextView mReportCategoryDropdown;
    private @Nullable TextInputEditText mReportDetailsInput;
    private @Nullable TextInputEditText mReportContactInput;
    private @Nullable CheckBox mReportScreenshotCheckbox;
    private byte @Nullable [] mReportScreenshotBytes;
    private @Nullable String mSelectedReportCategory;
    private @Nullable WebcompatReporterHandler mWebcompatReporterHandler;

    // Current state
    private @Nullable Profile mProfile;
    private @Nullable Tab mCurrentTab;
    private @Nullable BraveShieldsHandler mLegacyShieldsHandler;

    private static @Nullable Context scanForActivity(@Nullable Context cont) {
        if (cont == null) {
            return null;
        } else if (cont instanceof Activity) {
            return cont;
        } else if (cont instanceof ContextWrapper) {
            return scanForActivity(((ContextWrapper) cont).getBaseContext());
        }
        return cont;
    }

    public BraveUnifiedPanelHandler(Context context) {
        Context contextCandidate = scanForActivity(context);
        mHardwareButtonMenuAnchor = null;
        mContext =
                (contextCandidate != null && (contextCandidate instanceof Activity))
                        ? contextCandidate
                        : null;

        if (mContext != null) {
            mHardwareButtonMenuAnchor = ((Activity) mContext).findViewById(R.id.menu_anchor_stub);
        }

        mLegacyShieldsHandler = null;
    }

    public void addObserver(BraveShieldsMenuObserver menuObserver) {
        mMenuObserver = menuObserver;
    }

    public void show(
            @Nullable View anchorView,
            @Nullable Tab tab,
            @Nullable BraveShieldsHandler shieldsHandler) {
        mLegacyShieldsHandler = shieldsHandler;
        if (mHardwareButtonMenuAnchor == null || mContext == null) {
            return;
        }

        if (tab == null || tab.getUrl() == null || tab.getWebContents() == null) {
            return;
        }

        mCurrentTab = tab;
        mUrl = tab.getUrl();
        mProfile = Profile.fromWebContents(tab.getWebContents());

        mPopupWindow = showPopupMenu(anchorView);
        if (mPopupWindow != null) {
            updateValues();
        }
    }

    private void updateValues() {
        if (mContext == null || mUrl == null) {
            return;
        }

        String siteName = UrlFormatter.formatUrlForDisplayOmitSchemePathAndTrivialSubdomains(mUrl);
        mSiteTextUnified.setText(siteName);

        boolean isShieldsUp =
                BraveShieldsContentSettings.getShields(
                        mProfile,
                        mUrl.getSpec(),
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
        mShieldsToggleSwitch.setSelected(isShieldsUp);
        mShieldsStatusText.setText(
                isShieldsUp
                        ? mContext.getString(R.string.shields_up_status)
                        : mContext.getString(R.string.shields_down_status));

        Bitmap favicon = mCurrentTab != null ? TabFavicon.getBitmap(mCurrentTab) : null;
        if (favicon == null && mUrl != null) {
            FaviconHelper.DefaultFaviconHelper helper = new FaviconHelper.DefaultFaviconHelper();
            favicon =
                    helper.getDefaultFaviconBitmap(
                            mContext,
                            mUrl,
                            /* useDarkIcon= */ true,
                            /* useIncognitoNtpIcon= */ false);
        }
        if (favicon != null) {
            mShieldIconUnified.setImageBitmap(favicon);
        }

        mBlockCountNumber.setText("0");
        mBlockCountText.setText(R.string.trackers_blocked_text);
    }

    private @Nullable PopupWindow showPopupMenu(@Nullable View anchorView) {
        assert (mContext != null);
        assert (mHardwareButtonMenuAnchor != null);

        int rotation = ((Activity) mContext).getWindowManager().getDefaultDisplay().getRotation();
        int displayHeight = mContext.getResources().getDisplayMetrics().heightPixels;
        int widthHeight = mContext.getResources().getDisplayMetrics().widthPixels;

        if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
            displayHeight = Math.max(displayHeight, widthHeight);
        } else if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
            displayHeight = Math.min(displayHeight, widthHeight);
        }

        if (anchorView == null) {
            Rect rect = new Rect();
            ((Activity) mContext).getWindow().getDecorView().getWindowVisibleDisplayFrame(rect);
            int statusBarHeight = rect.top;
            mHardwareButtonMenuAnchor.setY((displayHeight - statusBarHeight));
            anchorView = mHardwareButtonMenuAnchor;
        }

        LayoutInflater inflater =
                (LayoutInflater)
                        anchorView.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (inflater == null) {
            return null;
        }

        try {
            mPopupView = inflater.inflate(R.layout.brave_unified_panel_layout, null);
            setupViews();
        } catch (Exception e) {
            return null;
        }

        float density = mContext.getResources().getDisplayMetrics().density;
        int marginDp = 16;
        int marginPx = (int) (marginDp * density);
        int screenWidth = mContext.getResources().getDisplayMetrics().widthPixels;

        int width;
        if (ConfigurationUtils.isLandscape(mContext)) {
            width = (int) (screenWidth * 0.50);
        } else {
            width = screenWidth - (marginPx * 2);
        }
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;

        PopupWindow popupWindow = new PopupWindow(mPopupView, width, height, true);
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        popupWindow.setElevation(20);
        mPopupView.setClipToOutline(true);

        mAnchorView = anchorView;
        if (BottomToolbarConfiguration.isToolbarBottomAnchored()) {
            popupWindow.setAnimationStyle(R.style.AnchoredPopupAnimEndBottom);
        } else {
            popupWindow.setAnimationStyle(R.style.AnchoredPopupAnimEndTop);
        }

        try {
            if (BottomToolbarConfiguration.isToolbarBottomAnchored()) {
                int[] anchorLocation = new int[2];
                mAnchorView.getLocationOnScreen(anchorLocation);

                int screenHeight = mContext.getResources().getDisplayMetrics().heightPixels;
                int anchorTop = anchorLocation[1];
                int yOffset = screenHeight - anchorTop + (int) (8 * density);

                popupWindow.showAtLocation(
                        mAnchorView,
                        android.view.Gravity.BOTTOM | android.view.Gravity.CENTER_HORIZONTAL,
                        0,
                        yOffset);
            } else {
                popupWindow.showAsDropDown(mAnchorView);
            }
        } catch (Exception e) {
            return null;
        }

        return popupWindow;
    }

    @Initializer
    private void setupViews() {
        View popupView = assumeNonNull(mPopupView);

        mMainPanelContainer = popupView.findViewById(R.id.main_panel_container);
        mHttpsPanelContainer = popupView.findViewById(R.id.https_panel_container);
        mTrackersPanelContainer = popupView.findViewById(R.id.trackers_panel_container);
        mCookiesPanelContainer = popupView.findViewById(R.id.cookies_panel_container);
        mShredPanelContainer = popupView.findViewById(R.id.shred_panel_container);
        mReportBrokenSitePanelContainer =
                popupView.findViewById(R.id.report_broken_site_panel_container);

        // Shields content elements
        mShieldIconUnified = popupView.findViewById(R.id.shield_icon_unified);
        mSiteTextUnified = assumeNonNull(popupView.findViewById(R.id.site_text_unified));
        mShieldsStatusText = popupView.findViewById(R.id.shields_status_text);
        mShieldsToggleSwitch = popupView.findViewById(R.id.shields_toggle_switch);
        mBlockCountNumber = popupView.findViewById(R.id.block_count_number);
        mBlockCountText = popupView.findViewById(R.id.block_count_text);
        mBlockedItemsContainer = popupView.findViewById(R.id.blocked_items_container);

        mFaviconHelper = new FaviconHelper();
        mAdvancedOptionsButton =
                assumeNonNull(popupView.findViewById(R.id.advanced_options_button));
        mAdvancedOptionsContent = popupView.findViewById(R.id.advanced_options_content);
        mAdvancedOptionsArrow = popupView.findViewById(R.id.advanced_options_arrow);

        // Set up shields toggle
        mShieldsToggleSwitch.setOnClickListener(
                v -> {
                    boolean newState = !v.isSelected();
                    v.setSelected(newState);
                    onShieldsToggleChanged(newState);
                });

        mIsAdvancedOptionsExpanded = false;
        if (mAdvancedOptionsContent != null) {
            mAdvancedOptionsContent.setVisibility(View.GONE);
        }
        if (mAdvancedOptionsArrow != null) {
            mAdvancedOptionsArrow.setRotation(0f);
        }
    }

    private void onShieldsToggleChanged(boolean isChecked) {}

    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    public void hide() {
        if (mPopupWindow != null && mPopupWindow.isShowing()) {
            mPopupWindow.dismiss();
        }
    }

    public void destroy() {
        if (mFaviconHelper != null) {
            mFaviconHelper.destroy();
            mFaviconHelper = null;
        }
        if (mWebcompatReporterHandler != null) {
            mWebcompatReporterHandler.close();
            mWebcompatReporterHandler = null;
        }
    }

    // Delegate methods to legacy handler for stats tracking
    public void addStat(int tabId, @Nullable String blockType, @Nullable String subResource) {
        if (mLegacyShieldsHandler != null) {
            mLegacyShieldsHandler.addStat(tabId, blockType, subResource);
        }
    }

    public void removeStat(int tabId) {
        if (mLegacyShieldsHandler != null) {
            mLegacyShieldsHandler.removeStat(tabId);
        }
    }

    public void clearBraveShieldsCount(int tabId) {
        if (mLegacyShieldsHandler != null) {
            mLegacyShieldsHandler.clearBraveShieldsCount(tabId);
        }
    }
}
