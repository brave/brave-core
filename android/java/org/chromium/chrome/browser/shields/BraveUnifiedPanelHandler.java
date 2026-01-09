/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.util.ConfigurationUtils;

/**
 * Handler for the unified Shields and Site Settings panel. This replaces the old
 * BraveShieldsHandler with a tabbed interface.
 */
@NullMarked
public class BraveUnifiedPanelHandler {
    private static final String TAG = "BraveUnifiedPanel";

    // These are nullable based on initialization state
    private final @Nullable Context mContext;
    private @Nullable PopupWindow mPopupWindow;
    private @Nullable View mPopupView;
    private @Nullable View mAnchorView;
    private @Nullable View mHardwareButtonMenuAnchor;
    private @Nullable String mUrlSpec;
    private @Nullable String mHost;

    // UI elements - initialized in setupViews() from known layout, non-null after setup
    private LinearLayout mShieldsTabButton;
    private LinearLayout mSiteSettingsTabButton;
    private LinearLayout mAdvancedOptionsButton;
    private @Nullable LinearLayout mAdvancedOptionsContent;
    private @Nullable ImageView mAdvancedOptionsArrow;

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
    }

    public void show(
            @Nullable View anchorView,
            @Nullable Tab tab,
            @Nullable BraveShieldsHandler shieldsHandler) {
        if (mHardwareButtonMenuAnchor == null || mContext == null) {
            return;
        }

        // Safety check: ensure tab and required data are valid
        if (tab == null || tab.getUrl() == null || tab.getWebContents() == null) {
            return;
        }

        mUrlSpec = tab.getUrl().getSpec();
        mHost = tab.getUrl().getHost();

        mPopupWindow = showPopupMenu(anchorView);
        if (mPopupWindow != null) {
            updateValues();
        }
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
            // Layout inflation or view setup failed
            return null;
        }

        // Figma design: panel takes nearly full width with 16dp margin on each side
        float density = mContext.getResources().getDisplayMetrics().density;
        int marginDp = 16; // 16dp margin on each side (32dp total)
        int marginPx = (int) (marginDp * density);
        int screenWidth = mContext.getResources().getDisplayMetrics().widthPixels;

        int width;
        if (ConfigurationUtils.isLandscape(mContext)) {
            // In landscape, use 50% of width to not be too wide
            width = (int) (screenWidth * 0.50);
        } else {
            // In portrait, use screen width minus margins (matching Figma)
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

        // Try to show the popup, but catch any exceptions (e.g., DeadObjectException)
        // that might occur if the window is being destroyed
        try {
            popupWindow.showAsDropDown(mAnchorView);
        } catch (Exception e) {
            // Handle any exceptions during popup display (e.g., DeadObjectException)
            // This can happen if the activity is being destroyed or window is invalid
            return null;
        }

        return popupWindow;
    }

    private void setupViews() {
        View popupView = assumeNonNull(mPopupView);

        // Tab buttons - these must exist in the layout
        mShieldsTabButton = assumeNonNull(popupView.findViewById(R.id.shields_tab_button));
        mSiteSettingsTabButton =
                assumeNonNull(popupView.findViewById(R.id.site_settings_tab_button));

        // Advanced options button
        mAdvancedOptionsButton =
                assumeNonNull(popupView.findViewById(R.id.advanced_options_button));
        mAdvancedOptionsContent = popupView.findViewById(R.id.advanced_options_content);
        mAdvancedOptionsArrow = popupView.findViewById(R.id.advanced_options_arrow);

        // Set up tab click listeners
        mShieldsTabButton.setOnClickListener(v -> switchToTab(0));
        mSiteSettingsTabButton.setOnClickListener(v -> switchToTab(1));

        // Set up advanced options (collapsible)
        mAdvancedOptionsButton.setOnClickListener(v -> toggleAdvancedOptions());

        // Reset advanced options state (collapsed by default when showing panel)
        if (mAdvancedOptionsContent != null) {
            mAdvancedOptionsContent.setVisibility(View.GONE);
        }
        if (mAdvancedOptionsArrow != null) {
            mAdvancedOptionsArrow.setRotation(0f);
        }

        // Set up advanced options item click listeners
        setupAdvancedOptionsListeners();
    }

    private void setupAdvancedOptionsListeners() {
        // Stub - implemented in Advanced Options commit
    }

    private void switchToTab(int tabIndex) {
        // Stub - full tab switching implemented in Display Favicons commit
    }

    private void updateValues() {
        // Stub - implemented in Display Favicons commit
    }

    private void toggleAdvancedOptions() {
        // Stub - implemented in Advanced Options commit
    }

    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    public void hide() {
        if (mPopupWindow != null && mPopupWindow.isShowing()) {
            mPopupWindow.dismiss();
        }
    }

    public void destroy() {
        // Cleanup handled in later commits
    }

    public void addStat(int tabId, @Nullable String blockType, @Nullable String subResource) {
        // Stub - implemented in Permissions Panel commit
    }

    public void removeStat(int tabId) {
        // Stub - implemented in Permissions Panel commit
    }

    public void clearBraveShieldsCount(int tabId) {
        // Stub - implemented in Permissions Panel commit
    }

    public void addObserver(@Nullable BraveShieldsMenuObserver observer) {
        // Stub - implemented in Permissions Panel commit
    }
}
