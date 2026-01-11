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
import org.chromium.chrome.browser.offlinepages.OfflinePageUtils;
import org.chromium.chrome.browser.page_info.ChromePageInfoControllerDelegate;
import org.chromium.chrome.browser.page_info.ChromePageInfoHighlight;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.site_settings.ChromeSiteSettingsDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabFavicon;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.components.browser_ui.site_settings.ContentSettingException;
import org.chromium.components.browser_ui.site_settings.ContentSettingsResources;
import org.chromium.components.browser_ui.site_settings.PermissionInfo;
import org.chromium.components.browser_ui.site_settings.SingleWebsiteSettings;
import org.chromium.components.browser_ui.site_settings.Website;
import org.chromium.components.browser_ui.site_settings.WebsiteAddress;
import org.chromium.components.browser_ui.site_settings.WebsitePermissionsFetcher;
import org.chromium.components.content_settings.ContentSetting;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.components.page_info.PageInfoController;
import org.chromium.components.page_info.PageInfoPermissionsController;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.Collection;
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
    private View mMainPanelContainer;
    private View mHttpsPanelContainer;
    private View mTrackersPanelContainer;
    private View mCookiesPanelContainer;
    private View mShredPanelContainer;

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

    // Site Info elements
    private TextView mSiteNameText;
    private TextView mPermissionsSummary;

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

        // Get container views
        mMainPanelContainer = popupView.findViewById(R.id.main_panel_container);
        mHttpsPanelContainer = popupView.findViewById(R.id.https_panel_container);
        mTrackersPanelContainer = popupView.findViewById(R.id.trackers_panel_container);
        mCookiesPanelContainer = popupView.findViewById(R.id.cookies_panel_container);
        mShredPanelContainer = popupView.findViewById(R.id.shred_panel_container);

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

        // Site Info elements - note: this is inside site_settings_tab_content which starts hidden
        // It's okay if this is null initially, we'll find it when needed
        if (mSiteSettingsContent != null) {
            mSiteNameText = mSiteSettingsContent.findViewById(R.id.site_name_text);
            mPermissionsSummary = mSiteSettingsContent.findViewById(R.id.permissions_summary);

            // Wire up clickable Site Info items
            View dangerousSiteItem = mSiteSettingsContent.findViewById(R.id.dangerous_site_item);
            View permissionsItem = mSiteSettingsContent.findViewById(R.id.permissions_item);

            if (dangerousSiteItem != null) {
                dangerousSiteItem.setOnClickListener(v -> onDangerousSiteClicked());
            }

            if (permissionsItem != null) {
                permissionsItem.setOnClickListener(v -> onPermissionsClicked());
            }
        }

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
        if (mSiteNameText != null && mHost != null) {
            // Set the site name (same logic as Shields tab)
            String siteName = mHost.replaceFirst("^(http[s]?://www\\.|http[s]?://|www\\.)", "");
            mSiteNameText.setText(siteName);
        }

        // Update permissions summary
        updatePermissionsSummary();
    }

    /**
     * Update the permissions summary text to show what permissions the site has. Uses Chromium's
     * PageInfoPermissionsController.getPermissionSummaryString().
     */
    private void updatePermissionsSummary() {
        if (mPermissionsSummary == null
                || mProfile == null
                || mUrlSpec == null
                || mContext == null) {
            return;
        }

        WebsiteAddress address = WebsiteAddress.create(mUrlSpec);
        if (address == null) {
            mPermissionsSummary.setText(R.string.no_site_permissions_set);
            return;
        }

        ChromeSiteSettingsDelegate delegate = new ChromeSiteSettingsDelegate(mContext, mProfile);
        new WebsitePermissionsFetcher(delegate)
                .fetchAllPreferences(sites -> processPermissionsSummary(sites, address));
    }

    private void processPermissionsSummary(Collection<Website> sites, WebsiteAddress address) {
        if (mPermissionsSummary == null || mContext == null) return;

        Website site =
                SingleWebsiteSettings.mergePermissionAndStorageInfoForTopLevelOrigin(
                        address, sites);

        // Build PermissionObject list directly from Website's stored permissions
        List<PageInfoPermissionsController.PermissionObject> permissions = new ArrayList<>();
        if (site != null) {
            for (PermissionInfo info : site.getPermissionInfos()) {
                @ContentSetting Integer setting = info.getContentSetting(mProfile);
                if (setting != null && setting != ContentSetting.DEFAULT) {
                    permissions.add(
                            createPermissionObject(
                                    info.getContentSettingsType(),
                                    setting == ContentSetting.ALLOW));
                }
            }
            for (ContentSettingException ex : site.getContentSettingExceptions()) {
                if (ex.getContentSetting() != ContentSetting.DEFAULT) {
                    permissions.add(
                            createPermissionObject(
                                    ex.getContentSettingType(),
                                    ex.getContentSetting() == ContentSetting.ALLOW));
                }
            }
        }

        final String summaryText =
                PageInfoPermissionsController.getPermissionSummaryString(
                        permissions, mContext.getResources());

        String displayText =
                (summaryText != null && !summaryText.isEmpty())
                        ? summaryText
                        : mContext.getString(R.string.no_site_permissions_set);

        ((Activity) mContext)
                .runOnUiThread(
                        () -> {
                            if (mPermissionsSummary != null) {
                                mPermissionsSummary.setText(displayText);
                            }
                        });
    }

    /** Create a PermissionObject for the summary string. */
    private PageInfoPermissionsController.PermissionObject createPermissionObject(
            @ContentSettingsType.EnumType int type, boolean allowed) {
        String name = mContext.getString(ContentSettingsResources.getTitle(type));
        String nameMid =
                name.substring(0, 1).toLowerCase(java.util.Locale.getDefault()) + name.substring(1);
        return new PageInfoPermissionsController.PermissionObject(
                type, name, nameMid, allowed, /* warningTextResource= */ 0, /* requested= */ false);
    }

    /**
     * Display up to 3 favicons of blocked trackers/ads. Only shows favicons that are already cached
     * locally.
     */
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

    private void onReportBrokenSiteClicked() {
        // Hide the unified panel and use the legacy shields handler to show the report form
        hide();

        if (mLegacyShieldsHandler != null && mAnchorView != null && mCurrentTab != null) {
            // Use the legacy handler to show the report broken site flow
            mLegacyShieldsHandler.showShieldsReportBrokenSite(mAnchorView, mCurrentTab);
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

    private void showHttpsUpgradePanel() {
        if (mMainPanelContainer == null || mHttpsPanelContainer == null) {
            return;
        }

        // Set up the HTTPS panel if not already done
        if (mHttpsPanelContainer instanceof android.widget.ScrollView) {
            android.widget.ScrollView scrollView = (android.widget.ScrollView) mHttpsPanelContainer;
            if (scrollView.getChildCount() > 0) {
                View httpsPanel = scrollView.getChildAt(0);
                setupHttpsUpgradePanel(httpsPanel);
            }
        }

        // Hide main panel, show HTTPS panel
        mMainPanelContainer.setVisibility(View.GONE);
        mHttpsPanelContainer.setVisibility(View.VISIBLE);
    }

    private void showMainPanel() {
        if (mMainPanelContainer == null
                || mHttpsPanelContainer == null
                || mTrackersPanelContainer == null
                || mCookiesPanelContainer == null) {
            return;
        }

        // Hide all sub-panels, show main panel
        mHttpsPanelContainer.setVisibility(View.GONE);
        mTrackersPanelContainer.setVisibility(View.GONE);
        mCookiesPanelContainer.setVisibility(View.GONE);
        if (mShredPanelContainer != null) {
            mShredPanelContainer.setVisibility(View.GONE);
        }
        mMainPanelContainer.setVisibility(View.VISIBLE);

        // Restore the expanded state of advanced options if it was expanded
        if (mIsAdvancedOptionsExpanded
                && mAdvancedOptionsContent != null
                && mAdvancedOptionsArrow != null) {
            mAdvancedOptionsContent.setVisibility(View.VISIBLE);
            mAdvancedOptionsArrow.setRotation(180f);
        }
    }

    private void setupHttpsUpgradePanel(View panel) {
        // Set up back button
        ImageView backButton = panel.findViewById(R.id.https_back_button);
        if (backButton != null) {
            backButton.setOnClickListener(v -> showMainPanel());
        }

        // Set up radio buttons
        androidx.appcompat.widget.AppCompatRadioButton strictRadio =
                panel.findViewById(R.id.https_strict_radio);
        androidx.appcompat.widget.AppCompatRadioButton defaultRadio =
                panel.findViewById(R.id.https_default_radio);
        androidx.appcompat.widget.AppCompatRadioButton disabledRadio =
                panel.findViewById(R.id.https_disabled_radio);

        LinearLayout strictOption = panel.findViewById(R.id.https_strict_option);
        LinearLayout defaultOption = panel.findViewById(R.id.https_default_option);
        LinearLayout disabledOption = panel.findViewById(R.id.https_disabled_option);

        if (strictRadio == null || defaultRadio == null || disabledRadio == null) {
            return;
        }

        // Get current HTTPS upgrade setting
        // For now, we'll default to "default" (the middle option)
        // TODO: Get actual setting from BraveShieldsContentSettings
        defaultRadio.setChecked(true);

        // Set up click listeners for the entire option rows
        if (strictOption != null) {
            strictOption.setOnClickListener(
                    v -> {
                        strictRadio.setChecked(true);
                        defaultRadio.setChecked(false);
                        disabledRadio.setChecked(false);
                        // TODO: Save setting to BraveShieldsContentSettings
                    });
        }

        if (defaultOption != null) {
            defaultOption.setOnClickListener(
                    v -> {
                        strictRadio.setChecked(false);
                        defaultRadio.setChecked(true);
                        disabledRadio.setChecked(false);
                        // TODO: Save setting to BraveShieldsContentSettings
                    });
        }

        if (disabledOption != null) {
            disabledOption.setOnClickListener(
                    v -> {
                        strictRadio.setChecked(false);
                        defaultRadio.setChecked(false);
                        disabledRadio.setChecked(true);
                        // TODO: Save setting to BraveShieldsContentSettings
                    });
        }
    }

    private void showTrackersAdsPanel() {
        if (mMainPanelContainer == null || mTrackersPanelContainer == null) {
            return;
        }

        // Set up the Trackers panel if not already done
        if (mTrackersPanelContainer instanceof android.widget.ScrollView) {
            android.widget.ScrollView scrollView =
                    (android.widget.ScrollView) mTrackersPanelContainer;
            if (scrollView.getChildCount() > 0) {
                View trackersPanel = scrollView.getChildAt(0);
                setupTrackersAdsPanel(trackersPanel);
            }
        }

        // Hide main panel, show Trackers panel
        mMainPanelContainer.setVisibility(View.GONE);
        mTrackersPanelContainer.setVisibility(View.VISIBLE);
    }

    private void setupTrackersAdsPanel(View panel) {
        // Set up back button
        ImageView backButton = panel.findViewById(R.id.trackers_back_button);
        if (backButton != null) {
            backButton.setOnClickListener(v -> showMainPanel());
        }

        // Set up radio buttons
        androidx.appcompat.widget.AppCompatRadioButton aggressiveRadio =
                panel.findViewById(R.id.trackers_aggressive_radio);
        androidx.appcompat.widget.AppCompatRadioButton standardRadio =
                panel.findViewById(R.id.trackers_standard_radio);
        androidx.appcompat.widget.AppCompatRadioButton allowRadio =
                panel.findViewById(R.id.trackers_allow_radio);

        // Get current setting
        String currentSetting =
                BraveShieldsContentSettings.getShieldsValue(
                        mProfile,
                        mUrlSpec,
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS);

        // Set initial radio button state based on current setting
        if (aggressiveRadio != null && standardRadio != null && allowRadio != null) {
            if (currentSetting.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
                aggressiveRadio.setChecked(true);
            } else if (currentSetting.equals(BraveShieldsContentSettings.DEFAULT)
                    || currentSetting.equals(
                            BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE)) {
                standardRadio.setChecked(true);
            } else if (currentSetting.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
                allowRadio.setChecked(true);
            } else {
                standardRadio.setChecked(true); // Default to standard
            }
        }

        // Set up click listeners for the option containers
        View aggressiveOption = panel.findViewById(R.id.trackers_aggressive_option);
        View standardOption = panel.findViewById(R.id.trackers_standard_option);
        View allowOption = panel.findViewById(R.id.trackers_allow_option);

        if (aggressiveOption != null && aggressiveRadio != null) {
            aggressiveOption.setOnClickListener(
                    v -> {
                        aggressiveRadio.setChecked(true);
                        setTrackersAdsSetting(BraveShieldsContentSettings.BLOCK_RESOURCE);
                    });
        }

        if (standardOption != null && standardRadio != null) {
            standardOption.setOnClickListener(
                    v -> {
                        standardRadio.setChecked(true);
                        setTrackersAdsSetting(BraveShieldsContentSettings.DEFAULT);
                    });
        }

        if (allowOption != null && allowRadio != null) {
            allowOption.setOnClickListener(
                    v -> {
                        allowRadio.setChecked(true);
                        setTrackersAdsSetting(BraveShieldsContentSettings.ALLOW_RESOURCE);
                    });
        }
    }

    private void setTrackersAdsSetting(String value) {
        if (mUrlSpec == null || mProfile == null) {
            return;
        }

        // Use setShieldsValue for trackers setting
        BraveShieldsContentSettings.setShieldsValue(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                value,
                false);
    }

    private void showCookiesPanel() {
        if (mMainPanelContainer == null || mCookiesPanelContainer == null) {
            return;
        }

        // Set up the Cookies panel if not already done
	// TODO: Cleanup type cruft.
        if (mCookiesPanelContainer instanceof android.widget.ScrollView) {
            android.widget.ScrollView scrollView =
                    (android.widget.ScrollView) mCookiesPanelContainer;
            if (scrollView.getChildCount() > 0) {
                View cookiesPanel = scrollView.getChildAt(0);
                setupCookiesPanel(cookiesPanel);
            }
        }

        // Hide main panel, show Cookies panel
        mMainPanelContainer.setVisibility(View.GONE);
        mCookiesPanelContainer.setVisibility(View.VISIBLE);
    }

    private void setupCookiesPanel(View panel) {
        // Set up back button
        ImageView backButton = panel.findViewById(R.id.cookies_back_button);
        if (backButton != null) {
            backButton.setOnClickListener(v -> showMainPanel());
        }

        // Set up radio buttons
        androidx.appcompat.widget.AppCompatRadioButton blockAllRadio =
                panel.findViewById(R.id.cookies_block_all_radio);
        androidx.appcompat.widget.AppCompatRadioButton blockThirdPartyRadio =
                panel.findViewById(R.id.cookies_block_third_party_radio);
        androidx.appcompat.widget.AppCompatRadioButton allowRadio =
                panel.findViewById(R.id.cookies_allow_radio);

        // Get current setting
        String currentSetting =
                BraveShieldsContentSettings.getShieldsValue(
                        mProfile,
                        mUrlSpec,
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES);

        // Set initial radio button state based on current setting
        if (blockAllRadio != null && blockThirdPartyRadio != null && allowRadio != null) {
            if (currentSetting.equals(BraveShieldsContentSettings.BLOCK_RESOURCE)) {
                blockAllRadio.setChecked(true);
            } else if (currentSetting.equals(
                    BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE)) {
                blockThirdPartyRadio.setChecked(true);
            } else if (currentSetting.equals(BraveShieldsContentSettings.ALLOW_RESOURCE)) {
                allowRadio.setChecked(true);
            } else {
                blockThirdPartyRadio.setChecked(true); // Default to block 3rd-party
            }
        }

        // Set up click listeners for the option containers
        View blockAllOption = panel.findViewById(R.id.cookies_block_all_option);
        View blockThirdPartyOption = panel.findViewById(R.id.cookies_block_third_party_option);
        View allowOption = panel.findViewById(R.id.cookies_allow_option);

        if (blockAllOption != null && blockAllRadio != null) {
            blockAllOption.setOnClickListener(
                    v -> {
                        blockAllRadio.setChecked(true);
                        setCookiesSetting(BraveShieldsContentSettings.BLOCK_RESOURCE);
                    });
        }

        if (blockThirdPartyOption != null && blockThirdPartyRadio != null) {
            blockThirdPartyOption.setOnClickListener(
                    v -> {
                        blockThirdPartyRadio.setChecked(true);
                        setCookiesSetting(BraveShieldsContentSettings.BLOCK_THIRDPARTY_RESOURCE);
                    });
        }

        if (allowOption != null && allowRadio != null) {
            allowOption.setOnClickListener(
                    v -> {
                        allowRadio.setChecked(true);
                        setCookiesSetting(BraveShieldsContentSettings.ALLOW_RESOURCE);
                    });
        }
    }

    private void setCookiesSetting(String value) {
        if (mUrlSpec == null || mProfile == null) {
            return;
        }

        // Use setShieldsValue for cookies setting
        BraveShieldsContentSettings.setShieldsValue(
                mProfile,
                mUrlSpec,
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES,
                value,
                false);
    }

    // ==================== Shred Site Data Panel ====================

    private void showShredPanel() {
        if (mMainPanelContainer == null || mShredPanelContainer == null || mContext == null) {
            return;
        }

        // Set up the Shred panel if not already done
        setupShredPanel();

        // Hide main panel, show Shred panel
        mMainPanelContainer.setVisibility(View.GONE);
        mShredPanelContainer.setVisibility(View.VISIBLE);
    }

    private void setupShredPanel() {
        if (mShredPanelContainer == null) {
            return;
        }

        // Set up back button
        View backButton = mShredPanelContainer.findViewById(R.id.shred_back_button);
        if (backButton != null) {
            backButton.setOnClickListener(v -> showMainPanel());
        }

        // Set site label
        TextView siteLabel = mShredPanelContainer.findViewById(R.id.shred_site_label);
        if (siteLabel != null && mHost != null) {
            String siteName = mHost.replaceFirst("^(http[s]?://www\\.|http[s]?://|www\\.)", "");
            siteLabel.setText(siteName.toUpperCase(java.util.Locale.ROOT));
        }

        // Set up auto shred dropdown click
        View autoShredItem = mShredPanelContainer.findViewById(R.id.auto_shred_item);
        if (autoShredItem != null) {
            autoShredItem.setOnClickListener(
                    v -> {
                        // TODO: Show dropdown/dialog for auto shred options
                        // For now, this is just UI - wiring will be added later
                    });
        }

        // Set up shred now button
        View shredNowItem = mShredPanelContainer.findViewById(R.id.shred_now_item);
        if (shredNowItem != null) {
            shredNowItem.setOnClickListener(
                    v -> {
                        // TODO: Trigger immediate shred action
                        // For now, this is just UI - wiring will be added later
                    });
        }

        // Set initial auto shred value display
        TextView autoShredValue = mShredPanelContainer.findViewById(R.id.auto_shred_value);
        if (autoShredValue != null) {
            // TODO: Get actual setting value
            // For now, show default value
            autoShredValue.setText(R.string.shred_option_site_tabs_closed);
        }
    }

    private void onDangerousSiteClicked() {
        // Show the page info dialog - user can navigate to connection/security details
        showPageInfo(ChromePageInfoHighlight.noHighlight());
    }

    private void onPermissionsClicked() {
        // Stub - showPermissionsPanel() implemented in Permissions Panel commit
    }

    private void showPageInfo(ChromePageInfoHighlight highlight) {
        if (mContext == null || mCurrentTab == null || mCurrentTab.getWebContents() == null) {
            return;
        }

        // Hide the unified panel first
        hide();

        // Get the modal dialog manager from WindowAndroid
        WindowAndroid windowAndroid = mCurrentTab.getWebContents().getTopLevelNativeWindow();
        if (windowAndroid == null) {
            return;
        }

        ModalDialogManager modalDialogManager = windowAndroid.getModalDialogManager();
        if (modalDialogManager == null) {
            return;
        }

        // Show the standard Chrome page info dialog
        if (mContext instanceof Activity) {
            Activity activity = (Activity) mContext;
            PageInfoController.show(
                    activity,
                    mCurrentTab.getWebContents(),
                    null, // publisher (for Reader Mode)
                    PageInfoController.OpenedFromSource.TOOLBAR,
                    new ChromePageInfoControllerDelegate(
                            activity,
                            mCurrentTab.getWebContents(),
                            () -> modalDialogManager, // modalDialogManagerSupplier
                            new OfflinePageUtils.TabOfflinePageLoadUrlDelegate(mCurrentTab),
                            null, // storeInfoActionHandlerSupplier
                            null, // ephemeralTabCoordinatorSupplier
                            highlight,
                            null, // tabCreator
                            null), // packageName
                    highlight,
                    Gravity.TOP,
                    /* openPermissionsSubpage= */ false);
        }
    }

    // TODO: Get rid of this
    private int dpToPx(int dp) {
        return (int) (dp * mContext.getResources().getDisplayMetrics().density);
    }
}
