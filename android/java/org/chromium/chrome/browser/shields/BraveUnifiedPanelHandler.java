/* Copyright (c) 2025 The Brave Authors. All rights reserved.
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
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.appcompat.widget.SwitchCompat;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.cosmetic_filters.BraveCosmeticFiltersUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabFavicon;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

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
    private View mShieldsTabIndicator;
    private View mSiteSettingsTabIndicator;
    private ImageView mShieldsTabIcon;
    private ImageView mSiteSettingsTabIcon;
    private TextView mShieldsTabText;
    private TextView mSiteSettingsTabText;
    private TextView mSiteSettingsBadge;

    // Content containers
    private LinearLayout mShieldsContent;
    private View mSiteSettingsContent;

    // Shields tab elements
    private ImageView mShieldIconUnified;
    private TextView mSiteTextUnified;
    private TextView mShieldsStatusText;
    private ImageView mShieldsToggleSwitch;
    private TextView mBlockCountNumber;
    private TextView mBlockCountText;
    private LinearLayout mBlockedItemsContainer;
    private FaviconHelper mFaviconHelper;
    private LinearLayout mAdvancedOptionsButton;
    private @Nullable LinearLayout mAdvancedOptionsContent;
    private @Nullable ImageView mAdvancedOptionsArrow;
    private boolean mIsAdvancedOptionsExpanded;

    // Advanced options switches
    private SwitchCompat mBlockScriptsSwitch;
    private SwitchCompat mForgetMeSwitch;
    private SwitchCompat mFingerprintingSwitch;
    private boolean mIsUpdatingSwitches;

    // Report broken site section
    private LinearLayout mReportBrokenSiteSection;
    private android.widget.Button mReportBrokenSiteButton;

    // Current state
    private @Nullable Profile mProfile;
    private @Nullable Tab mCurrentTab;
    // TODO: Migrate/Integrate this functionality?
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

        // Will be set when show() is called
        mLegacyShieldsHandler = null;
    }

    public void show(
            @Nullable View anchorView,
            @Nullable Tab tab,
            @Nullable BraveShieldsHandler shieldsHandler) {
        // Store the shields handler for stats tracking
        mLegacyShieldsHandler = shieldsHandler;
        if (mHardwareButtonMenuAnchor == null || mContext == null) {
            return;
        }

        // Safety check: ensure tab and required data are valid
        if (tab == null || tab.getUrl() == null || tab.getWebContents() == null) {
            return;
        }

        mCurrentTab = tab;
        mUrlSpec = tab.getUrl().getSpec();
        mHost = tab.getUrl().getHost();
        mProfile = Profile.fromWebContents(tab.getWebContents());

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
        mShieldsTabIndicator = assumeNonNull(popupView.findViewById(R.id.shields_tab_indicator));
        mSiteSettingsTabIndicator =
                assumeNonNull(popupView.findViewById(R.id.site_settings_tab_indicator));
        mShieldsTabIcon = assumeNonNull(popupView.findViewById(R.id.shields_tab_icon));
        mSiteSettingsTabIcon = assumeNonNull(popupView.findViewById(R.id.site_settings_tab_icon));
        mShieldsTabText = assumeNonNull(popupView.findViewById(R.id.shields_tab_text));
        mSiteSettingsTabText = assumeNonNull(popupView.findViewById(R.id.site_settings_tab_text));
        mSiteSettingsBadge = popupView.findViewById(R.id.site_settings_badge);

        // Content containers
        mShieldsContent = assumeNonNull(popupView.findViewById(R.id.shields_tab_content));
        mSiteSettingsContent =
                assumeNonNull(popupView.findViewById(R.id.site_settings_tab_content));

        // Shields content elements
        mShieldIconUnified = popupView.findViewById(R.id.shield_icon_unified);
        mSiteTextUnified = assumeNonNull(popupView.findViewById(R.id.site_text_unified));
        mShieldsStatusText = popupView.findViewById(R.id.shields_status_text);
        mShieldsToggleSwitch = popupView.findViewById(R.id.shields_toggle_switch);
        mBlockCountNumber = popupView.findViewById(R.id.block_count_number);
        mBlockCountText = popupView.findViewById(R.id.block_count_text);
        mBlockedItemsContainer = popupView.findViewById(R.id.blocked_items_container);

        // Initialize favicon helper for loading blocked item favicons
        mFaviconHelper = new FaviconHelper();
        mAdvancedOptionsButton =
                assumeNonNull(popupView.findViewById(R.id.advanced_options_button));
        mAdvancedOptionsContent = popupView.findViewById(R.id.advanced_options_content);
        mAdvancedOptionsArrow = popupView.findViewById(R.id.advanced_options_arrow);

        // Advanced options switches
        mBlockScriptsSwitch = mPopupView.findViewById(R.id.scripts_toggle);
        mForgetMeSwitch = mPopupView.findViewById(R.id.forget_me_toggle);
        mFingerprintingSwitch = mPopupView.findViewById(R.id.fingerprinting_toggle);

        // Report broken site section
        mReportBrokenSiteSection = popupView.findViewById(R.id.report_broken_site_section);
        mReportBrokenSiteButton = popupView.findViewById(R.id.report_broken_site_button);

        // Set up tab click listeners
        mShieldsTabButton.setOnClickListener(v -> switchToTab(0));
        mSiteSettingsTabButton.setOnClickListener(v -> switchToTab(1));

        // Set up shields toggle (using ImageView with selected state)
        mShieldsToggleSwitch.setOnClickListener(
                v -> {
                    boolean newState = !v.isSelected();
                    v.setSelected(newState);
                    onShieldsToggleChanged(newState);
                });

        // Set up advanced options switches
        if (mBlockScriptsSwitch != null) {
            mBlockScriptsSwitch.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> onBlockScriptsChanged(isChecked));
        }
        if (mForgetMeSwitch != null) {
            mForgetMeSwitch.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> onForgetMeChanged(isChecked));
        }
        if (mFingerprintingSwitch != null) {
            mFingerprintingSwitch.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> onFingerprintingChanged(isChecked));
        }

        // Set up report broken site button
        if (mReportBrokenSiteButton != null) {
            mReportBrokenSiteButton.setOnClickListener(v -> onReportBrokenSiteClicked());
        }

        // Set up advanced options (collapsible)
        mAdvancedOptionsButton.setOnClickListener(v -> toggleAdvancedOptions());

        // Reset advanced options state (collapsed by default when showing panel)
        mIsAdvancedOptionsExpanded = false;
        if (mAdvancedOptionsContent != null) {
            mAdvancedOptionsContent.setVisibility(View.GONE);
        }
        if (mAdvancedOptionsArrow != null) {
            mAdvancedOptionsArrow.setRotation(0f);
        }

        // Set up advanced options item click listeners
        setupAdvancedOptionsListeners();

        // Initialize with Shields tab selected
        switchToTab(0);
    }

    private void setupAdvancedOptionsListeners() {
        // HTTPS upgrade item
        View httpsUpgradeItem = mAdvancedOptionsContent.findViewById(R.id.https_upgrade_item);
        if (httpsUpgradeItem != null) {
            httpsUpgradeItem.setOnClickListener(v -> showHttpsUpgradePanel());
        }

        // Trackers & Ads item
        View trackersItem = mAdvancedOptionsContent.findViewById(R.id.trackers_item);
        if (trackersItem != null) {
            trackersItem.setOnClickListener(v -> showTrackersAdsPanel());
        }

        // Cookies item
        View cookiesItem = mAdvancedOptionsContent.findViewById(R.id.cookies_item);
        if (cookiesItem != null) {
            cookiesItem.setOnClickListener(v -> showCookiesPanel());
        }

        // Shred site data item
        View shredDataItem = mAdvancedOptionsContent.findViewById(R.id.shred_data_item);
        if (shredDataItem != null) {
            shredDataItem.setOnClickListener(v -> showShredPanel());
        }

        // Block element item
        View blockElementItem = mAdvancedOptionsContent.findViewById(R.id.block_element_item);
        if (blockElementItem != null) {
            // Show/hide based on feature flag and private window status
            boolean isPrivateWindow = mCurrentTab != null && mCurrentTab.isIncognito();
            boolean isFeatureEnabled =
                    ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHIELDS_ELEMENT_PICKER);

            if (!isPrivateWindow && isFeatureEnabled) {
                blockElementItem.setVisibility(View.VISIBLE);
                blockElementItem.setOnClickListener(
                        v -> {
                            if (mCurrentTab != null) {
                                BraveCosmeticFiltersUtils.launchContentPickerForWebContent(
                                        mCurrentTab);
                                hide(); // Close the popup after launching element picker
                            }
                        });
            } else {
                blockElementItem.setVisibility(View.GONE);
            }
        }

        // Global shields settings item
        View globalSettingsItem = mAdvancedOptionsContent.findViewById(R.id.global_settings_item);
        if (globalSettingsItem != null) {
            globalSettingsItem.setOnClickListener(v -> openGlobalShieldsSettings());
        }
    }

    private void openGlobalShieldsSettings() {
        if (mContext == null) {
            return;
        }

        // Hide the panel before navigating
        hide();

        // Navigate to BravePrivacySettings (Shields & Privacy settings)
        SettingsNavigation navigation = SettingsNavigationFactory.createSettingsNavigation();
        navigation.startSettings(
                mContext,
                org.chromium.chrome.browser.privacy.settings.BravePrivacySettings.class,
                null);
    }

    private void showHttpsUpgradePanel() {
        // Stub - implemented in Sub-panel Navigation commit
    }

    private void showTrackersAdsPanel() {
        // Stub - implemented in Sub-panel Navigation commit
    }

    private void showCookiesPanel() {
        // Stub - implemented in Sub-panel Navigation commit
    }

    private void showShredPanel() {
        // Stub - implemented in Sub-panel Navigation commit
    }

    private void switchToTab(int tabIndex) {
        if (mContext == null) {
            return;
        }

        if (tabIndex == 0) {
            // Shields tab
            mShieldsContent.setVisibility(View.VISIBLE);
            mSiteSettingsContent.setVisibility(View.GONE);
            mShieldsTabIndicator.setVisibility(View.VISIBLE);
            mSiteSettingsTabIndicator.setVisibility(View.GONE);
            mShieldsTabIcon.setImageTintList(
                    mContext.getColorStateList(R.color.tab_text_selected_color));
            mSiteSettingsTabIcon.setImageTintList(
                    mContext.getColorStateList(R.color.tab_text_unselected_color));
            mShieldsTabText.setTextAppearance(R.style.TextAppearance_BraveUnifiedPanelTab_Selected);
            mSiteSettingsTabText.setTextAppearance(
                    R.style.TextAppearance_BraveUnifiedPanelTab_Unselected);
        } else {
            // Site Settings tab
            mShieldsContent.setVisibility(View.GONE);
            mSiteSettingsContent.setVisibility(View.VISIBLE);
            mShieldsTabIndicator.setVisibility(View.GONE);
            mSiteSettingsTabIndicator.setVisibility(View.VISIBLE);
            mShieldsTabIcon.setImageTintList(
                    mContext.getColorStateList(R.color.tab_text_unselected_color));
            mSiteSettingsTabIcon.setImageTintList(
                    mContext.getColorStateList(R.color.tab_text_selected_color));
            mShieldsTabText.setTextAppearance(
                    R.style.TextAppearance_BraveUnifiedPanelTab_Unselected);
            mSiteSettingsTabText.setTextAppearance(
                    R.style.TextAppearance_BraveUnifiedPanelTab_Selected);
        }
    }

    private void updateValues() {
        if (mContext == null || mUrlSpec == null || mHost == null) {
            return;
        }

        // Update site name
        String siteName = mHost.replaceFirst("^(http[s]?://www\\.|http[s]?://|www\\.)", "");
        mSiteTextUnified.setText(siteName);

        // Update shields status
        boolean isShieldsUp =
                BraveShieldsContentSettings.getShields(
                        mProfile,
                        mUrlSpec,
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
        mShieldsToggleSwitch.setSelected(isShieldsUp);
        mShieldsStatusText.setText(
                isShieldsUp
                        ? mContext.getString(R.string.shields_up_status)
                        : mContext.getString(R.string.shields_down_status));

        // Toggle visibility of advanced options vs report broken site section
        updateShieldsSectionVisibility(isShieldsUp);

        // Update favicon/shield icon - use favicon if available, otherwise use shield icon
        Bitmap favicon = mCurrentTab != null ? TabFavicon.getBitmap(mCurrentTab) : null;
        if (favicon != null) {
            // Site has a favicon, use it
            mShieldIconUnified.setImageBitmap(favicon);
        } else {
            // No favicon, use shield icon as fallback
            int iconRes =
                    isShieldsUp ? R.drawable.ic_shield_done : R.drawable.ic_shield_done_outlined;
            mShieldIconUnified.setImageResource(iconRes);
        }

        // Update tab icon with shield icon (not favicon)
        int tabIconRes =
                isShieldsUp ? R.drawable.ic_shield_done : R.drawable.ic_shield_done_outlined;
        mShieldsTabIcon.setImageResource(tabIconRes);

        // Update block count from shields handler stats
        if (mCurrentTab != null && mLegacyShieldsHandler != null) {
            int tabId = mCurrentTab.getId();
            int totalBlocked = mLegacyShieldsHandler.getTotalBlockedCount(tabId);
            mBlockCountNumber.setText(String.valueOf(totalBlocked));
            mBlockCountText.setText(R.string.trackers_blocked_text);

            // Display favicons of blocked items
            displayBlockedItemFavicons(tabId);
        } else {
            mBlockCountNumber.setText("0");
            mBlockCountText.setText(R.string.trackers_blocked_text);
            // Clear blocked items container
            if (mBlockedItemsContainer != null) {
                mBlockedItemsContainer.removeAllViews();
            }
        }

        // Update advanced options switches
        updateAdvancedOptionsSwitches(isShieldsUp);

        // Update site info tab
        updateSiteInfo();

        // TODO: Update site settings badge count
        // For now, hide it
        mSiteSettingsBadge.setVisibility(View.GONE);
    }

    private void updateSiteInfo() {
        // Stub - implemented in Site Info commit
    }

    private void displayBlockedItemFavicons(int tabId) {
        if (mBlockedItemsContainer == null
                || mLegacyShieldsHandler == null
                || mProfile == null
                || mContext == null
                || mFaviconHelper == null) {
            return;
        }

        // Clear any existing icons
        mBlockedItemsContainer.removeAllViews();

        // Get blocked URLs from the shields handler
        ArrayList<String> blockedUrls = mLegacyShieldsHandler.getBlockedUrls(tabId);
        if (blockedUrls == null || blockedUrls.isEmpty()) {
            return;
        }

        // Extract unique domains from blocked URLs to avoid duplicates
        Set<String> uniqueDomains = new HashSet<>();
        List<String> urlsToShow = new ArrayList<>();

        for (String url : blockedUrls) {
            if (url == null || url.isEmpty()) continue;

            // Extract domain from URL - TODO: Make this un-hacky & use agreed spec.
            String domain = extractDomain(url);
            if (domain != null && !uniqueDomains.contains(domain)) {
                uniqueDomains.add(domain);
                urlsToShow.add(url); // Store original URL for favicon lookup
                if (urlsToShow.size() >= 3) break; // Only show up to 3 favicons
            }
        }

        // Fetch favicons for each unique domain
        int iconSize = dpToPx(20); // 20dp icons
        for (String url : urlsToShow) {
            loadBlockedItemFavicon(url, extractDomain(url), iconSize);
        }
    }

    /** Extract domain from a URL string. TODO: Make it not hacky */
    private String extractDomain(String url) {
        try {
            if (url == null || url.isEmpty()) return null;

            // Handle URLs without scheme
            if (!url.contains("://")) {
                url = "https://" + url;
            }

            GURL gurl = new GURL(url);
            if (gurl.isValid()) {
                return gurl.getHost();
            }
        } catch (Exception e) {
            // Ignore parsing errors
        }
        return null;
    }

    /**
     * Load favicon for a blocked item URL and add it to the container. Tries the original URL
     * first, then falls back to root domain, then shows default icon. TODO: also hacky.
     */
    private void loadBlockedItemFavicon(String url, String domain, int iconSize) {
        if (mFaviconHelper == null
                || mProfile == null
                || mBlockedItemsContainer == null
                || mContext == null) {
            return;
        }

        try {
            // Ensure URL has a scheme
            if (!url.contains("://")) {
                url = "https://" + url;
            }

            GURL gurl = new GURL(url);
            if (!gurl.isValid()) {
                // Show default icon if URL is invalid
                addDefaultIconToContainer(domain, iconSize);
                return;
            }

            final String finalDomain = domain;

            // Use getLocalFaviconImageForURL to only get locally cached favicons
            // This won't trigger network requests
            mFaviconHelper.getLocalFaviconImageForURL(
                    mProfile,
                    gurl,
                    iconSize,
                    (bitmap, iconUrl) -> {
                        if (mBlockedItemsContainer == null || mContext == null) return;

                        ((Activity) mContext)
                                .runOnUiThread(
                                        () -> {
                                            if (bitmap != null) {
                                                addFaviconToContainer(bitmap, iconSize);
                                            } else {
                                                // Try root domain favicon (e.g., google.com for
                                                // ads.google.com)
                                                tryRootDomainFavicon(finalDomain, iconSize);
                                            }
                                        });
                    });
        } catch (Exception e) {
            // Show default icon on error
            addDefaultIconToContainer(domain, iconSize);
        }
    }

    /**
     * Try to get favicon from root domain (e.g., google.com for ads.google.com). TODO: Un-hacky the
     * hacky.
     */
    private void tryRootDomainFavicon(String domain, int iconSize) {
        if (mFaviconHelper == null || mProfile == null || domain == null) {
            addDefaultIconToContainer(domain, iconSize);
            return;
        }

        try {
            // Extract root domain (e.g., google.com from ads.google.com)
            String rootDomain = getRootDomain(domain);
            if (rootDomain == null || rootDomain.equals(domain)) {
                // Already at root or couldn't extract, show default
                addDefaultIconToContainer(domain, iconSize);
                return;
            }

            GURL rootUrl = new GURL("https://" + rootDomain);
            if (!rootUrl.isValid()) {
                addDefaultIconToContainer(domain, iconSize);
                return;
            }

            final String finalDomain = domain;

            mFaviconHelper.getLocalFaviconImageForURL(
                    mProfile,
                    rootUrl,
                    iconSize,
                    (bitmap, iconUrl) -> {
                        if (mBlockedItemsContainer == null || mContext == null) return;

                        ((Activity) mContext)
                                .runOnUiThread(
                                        () -> {
                                            if (bitmap != null) {
                                                addFaviconToContainer(bitmap, iconSize);
                                            } else {
                                                // No favicon found, show default icon
                                                addDefaultIconToContainer(finalDomain, iconSize);
                                            }
                                        });
                    });
        } catch (Exception e) {
            addDefaultIconToContainer(domain, iconSize);
        }
    }

    // Extract root domain from a subdomain (e.g., google.com from ads.google.com).
    // TODO: More hacky domain cruft.
    private String getRootDomain(String domain) {
        if (domain == null) return null;

        String[] parts = domain.split("\\.");
        if (parts.length <= 2) {
            return domain; // Already at root (e.g., google.com)
        }

        // Return last two parts (e.g., google.com from ads.google.com)
        // Note: This is simplified and doesn't handle co.uk style TLDs
        return parts[parts.length - 2] + "." + parts[parts.length - 1];
    }

    // TODO: Do we even want this?
    private void addDefaultIconToContainer(String domain, int iconSize) {
        if (mBlockedItemsContainer == null || mContext == null) return;

        // Only show up to 3 icons
        if (mBlockedItemsContainer.getChildCount() >= 3) {
            return;
        }

        // Create a text-based icon with the first letter of the domain
        TextView textView = new TextView(mContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(iconSize, iconSize);
        params.setMarginEnd(dpToPx(4));
        textView.setLayoutParams(params);

        // Get first letter of domain
        String letter = "?";
        if (domain != null && !domain.isEmpty()) {
            // Skip www. if present
            String displayDomain = domain.startsWith("www.") ? domain.substring(4) : domain;
            if (!displayDomain.isEmpty()) {
                letter = displayDomain.substring(0, 1).toUpperCase(java.util.Locale.ROOT);
            }
        }

        textView.setText(letter);
        textView.setGravity(Gravity.CENTER);
        textView.setTextAppearance(R.style.TextAppearance_BraveBlockedItemIcon);
        textView.setBackgroundResource(R.drawable.blocked_item_default_bg);

        mBlockedItemsContainer.addView(textView);
    }

    private void addFaviconToContainer(Bitmap favicon, int iconSize) {
        if (mBlockedItemsContainer == null || mContext == null || favicon == null) {
            return;
        }

        // Only show up to 3 favicons
        if (mBlockedItemsContainer.getChildCount() >= 3) {
            return;
        }

        ImageView imageView = new ImageView(mContext);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(iconSize, iconSize);

        // Add small margin between icons
        int margin = dpToPx(4);
        params.setMarginEnd(margin);

        imageView.setLayoutParams(params);
        imageView.setImageBitmap(favicon);
        imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);

        // Add rounded corners (optional - can be done with a circular ImageView or
        // drawable)
        imageView.setClipToOutline(true);

        mBlockedItemsContainer.addView(imageView);
    }

    private void updateAdvancedOptionsSwitches(boolean shieldsEnabled) {
        if (mUrlSpec == null || mProfile == null) {
            return;
        }

        mIsUpdatingSwitches = true;

        // Block scripts switch
        if (mBlockScriptsSwitch != null) {
            if (shieldsEnabled) {
                boolean scriptsBlocked =
                        BraveShieldsContentSettings.getShields(
                                mProfile,
                                mUrlSpec,
                                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS);
                mBlockScriptsSwitch.setEnabled(true);
                mBlockScriptsSwitch.setChecked(scriptsBlocked);
            } else {
                mBlockScriptsSwitch.setEnabled(false);
                mBlockScriptsSwitch.setChecked(false);
            }
        }

        // Forget me switch
        if (mForgetMeSwitch != null) {
            if (shieldsEnabled) {
                boolean forgetMeEnabled =
                        BraveShieldsContentSettings.getShields(
                                mProfile,
                                mUrlSpec,
                                BraveShieldsContentSettings
                                        .RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE);
                mForgetMeSwitch.setEnabled(true);
                mForgetMeSwitch.setChecked(forgetMeEnabled);
            } else {
                mForgetMeSwitch.setEnabled(false);
                mForgetMeSwitch.setChecked(false);
            }
        }

        // Fingerprinting switch
        if (mFingerprintingSwitch != null) {
            if (shieldsEnabled) {
                String fingerprintingValue =
                        BraveShieldsContentSettings.getShieldsValue(
                                mProfile,
                                mUrlSpec,
                                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING);
                // If value is NOT ALLOW_RESOURCE, fingerprinting protection is enabled (checked)
                boolean fingerprintingEnabled =
                        !fingerprintingValue.equals(BraveShieldsContentSettings.ALLOW_RESOURCE);
                mFingerprintingSwitch.setEnabled(true);
                mFingerprintingSwitch.setChecked(fingerprintingEnabled);
            } else {
                mFingerprintingSwitch.setEnabled(false);
                mFingerprintingSwitch.setChecked(false);
            }
        }

        mIsUpdatingSwitches = false;
    }

    private void onShieldsToggleChanged(boolean isChecked) {
        if (mUrlSpec == null || mProfile == null) {
            return;
        }

        BraveShieldsContentSettings.setShields(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS,
                isChecked,
                true);

        updateValues();
    }

    private void updateShieldsSectionVisibility(boolean shieldsEnabled) {
        if (shieldsEnabled) {
            // Shields ON: Show advanced options, hide report broken site section
            if (mAdvancedOptionsButton != null) {
                mAdvancedOptionsButton.setVisibility(View.VISIBLE);
            }
            if (mReportBrokenSiteSection != null) {
                mReportBrokenSiteSection.setVisibility(View.GONE);
            }
        } else {
            // Shields OFF: Hide advanced options, show report broken site section
            if (mAdvancedOptionsButton != null) {
                mAdvancedOptionsButton.setVisibility(View.GONE);
            }
            if (mAdvancedOptionsContent != null) {
                mAdvancedOptionsContent.setVisibility(View.GONE);
                mIsAdvancedOptionsExpanded = false;
                if (mAdvancedOptionsArrow != null) {
                    mAdvancedOptionsArrow.setRotation(0f);
                }
            }
            if (mReportBrokenSiteSection != null) {
                mReportBrokenSiteSection.setVisibility(View.VISIBLE);
            }
        }
    }

    private void onBlockScriptsChanged(boolean isChecked) {
        if (mIsUpdatingSwitches || mUrlSpec == null || mProfile == null) {
            return;
        }

        BraveShieldsContentSettings.setShields(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS,
                isChecked,
                false);
    }

    private void onForgetMeChanged(boolean isChecked) {
        if (mIsUpdatingSwitches || mUrlSpec == null || mProfile == null) {
            return;
        }

        BraveShieldsContentSettings.setShields(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE,
                isChecked,
                false);
    }

    private void onFingerprintingChanged(boolean isChecked) {
        if (mIsUpdatingSwitches || mUrlSpec == null || mProfile == null) {
            return;
        }

        // Fingerprinting uses setShieldsValue with DEFAULT (enabled) or ALLOW_RESOURCE (disabled)
        BraveShieldsContentSettings.setShieldsValue(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                isChecked
                        ? BraveShieldsContentSettings.DEFAULT
                        : BraveShieldsContentSettings.ALLOW_RESOURCE,
                false);
    }

    private void onReportBrokenSiteClicked() {
        // Stub - implemented in Site Info commit
    }

    private void toggleAdvancedOptions() {
        mIsAdvancedOptionsExpanded = !mIsAdvancedOptionsExpanded;

        if (mIsAdvancedOptionsExpanded) {
            // Expand: show content and rotate arrow
            mAdvancedOptionsContent.setVisibility(View.VISIBLE);
            mAdvancedOptionsArrow.setRotation(180f);
        } else {
            // Collapse: hide content and reset arrow
            mAdvancedOptionsContent.setVisibility(View.GONE);
            mAdvancedOptionsArrow.setRotation(0f);
        }
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
        if (mFaviconHelper != null) {
            mFaviconHelper.destroy();
            mFaviconHelper = null;
        }
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

    // TODO: Get rid of this
    private int dpToPx(int dp) {
        return (int) (dp * mContext.getResources().getDisplayMetrics().density);
    }
}
