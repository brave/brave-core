/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assertNonNull;

import android.content.Context;
import android.content.res.Resources;
import android.os.Bundle;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ResettersForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.search.ChromeBaseSearchIndexProvider;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.chrome.browser.tab.TabArchiveSettings;
import org.chromium.chrome.browser.tab_group_sync.BraveSyncedTabGroupHelper;
import org.chromium.chrome.browser.tab_group_sync.TabGroupSyncFeatures;
import org.chromium.chrome.browser.tabmodel.BraveTabGroupHelper;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.tasks.tab_management.TabsSettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.sync.DataType;
import org.chromium.components.sync.SyncService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManagerHolder;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modaldialog.SimpleModalDialogController;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Map;
import java.util.Set;

/** Brave-owned Tabs and tab groups settings screen. */
@NullMarked
public class BraveTabsAndTabGroupsSettings extends BravePreferenceFragment {
    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_ENABLE_TAB_GROUPS_SWITCH = "enable_tab_groups";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_SHOW_SYNCED_TAB_GROUPS_SWITCH = "show_synced_tab_groups_switch";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_TAB_GROUPS_BAR_SWITCH = "tab_groups_bar_switch";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_OPEN_LINKS_IN_CURRENT_TAB_GROUP_SWITCH = "brave_enable_tab_groups";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_CLOSING_ALL_TABS_CLOSES_BRAVE = "closing_all_tabs_closes_brave";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_SHOW_UNDO_WHEN_TABS_CLOSED = "show_undo_when_tabs_closed";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_TAB_ARCHIVE_SETTINGS = "archive_settings_entrypoint";

    private static @Nullable Boolean sTabGroupSyncActiveForTesting;

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
        // This screen owns the complete XML because Brave groups upstream tabs settings with
        // Brave-only tab preferences and replaces the upstream search-index provider as a unit.
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_tabs_and_tab_groups_preferences);
        mPageTitle.set(getString(R.string.tabs_settings_title));

        configureEnableTabGroupsSwitch();
        configureShowSyncedTabGroupsSwitch();
        configureTabGroupsBarSwitch();
        configureOpenLinksInCurrentTabGroupSwitch();
        configureClosingAllTabsClosesBraveSwitch();
        configureShowUndoWhenTabsClosedSwitch();
        updateTabGroupDependentPreferences();
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public void onStart() {
        super.onStart();
        configureTabArchiveSettings();
    }

    @Override
    public @Nullable String getMainMenuKey() {
        return "tabs";
    }

    private void configureEnableTabGroupsSwitch() {
        ChromeSwitchPreference enableTabGroupsSwitch =
                assertNonNull(findPreference(PREF_ENABLE_TAB_GROUPS_SWITCH));
        enableTabGroupsSwitch.setChecked(BraveTabUiFeatureUtilities.isTabGroupsEnabled());
        enableTabGroupsSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    boolean enabled = (boolean) newValue;
                    if (!enabled && !isTabGroupSyncActive()) {
                        // Turning tab groups off removes the existing groups, and with tab group
                        // sync inactive the local groups are the only copy. Ask first and leave the
                        // switch on until the user confirms.
                        showDisableTabGroupsDialog((ChromeSwitchPreference) preference);
                        return false;
                    }
                    setTabGroupsEnabled(enabled);
                    return true;
                });
    }

    private void setTabGroupsEnabled(boolean enabled) {
        setTabGroupsEnabled(enabled, /* ungroupExistingGroups= */ false);
    }

    /**
     * Applies the "Enable tab groups" switch. With {@code ungroupExistingGroups} the open tab
     * groups are ungrouped first, which is what the confirmation dialog offers when tab group sync
     * is inactive. Ungrouping has to come before the setting flips: disabling tab groups closes
     * every group that sync knows about, and every group created on this device is put there even
     * when tab groups are not being synced, so by then there would be nothing left to ungroup.
     */
    private void setTabGroupsEnabled(boolean enabled, boolean ungroupExistingGroups) {
        if (ungroupExistingGroups) {
            BraveTabGroupHelper.ungroupAllTabGroups();
        }
        BraveTabUiFeatureUtilities.setTabGroupsEnabled(enabled);
        updateTabGroupDependentPreferences();
        // Disabling tab groups hides the synced ones, enabling them brings them back.
        BraveSyncedTabGroupHelper.notifySettingsChanged();
    }

    private void showDisableTabGroupsDialog(ChromeSwitchPreference enableTabGroupsSwitch) {
        ModalDialogManager modalDialogManager =
                ((ModalDialogManagerHolder) assertNonNull(getActivity())).getModalDialogManager();
        ModalDialogProperties.Controller dialogController =
                new SimpleModalDialogController(
                        modalDialogManager,
                        dismissalCause -> {
                            if (dismissalCause != DialogDismissalCause.POSITIVE_BUTTON_CLICKED) {
                                // Cancelled: tab groups stay enabled and the groups are kept.
                                return;
                            }
                            enableTabGroupsSwitch.setChecked(false);
                            setTabGroupsEnabled(false, /* ungroupExistingGroups= */ true);
                        });
        PropertyModel dialog =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER, dialogController)
                        .with(
                                ModalDialogProperties.TITLE,
                                getString(R.string.tab_groups_disable_dialog_title))
                        .with(
                                ModalDialogProperties.MESSAGE_PARAGRAPH_1,
                                getString(R.string.tab_groups_disable_dialog_message))
                        .with(
                                ModalDialogProperties.POSITIVE_BUTTON_TEXT,
                                getString(R.string.tab_groups_disable_dialog_confirm_button))
                        .with(
                                ModalDialogProperties.BUTTON_STYLES,
                                ModalDialogProperties.ButtonStyles.PRIMARY_FILLED_NEGATIVE_OUTLINE)
                        .with(
                                ModalDialogProperties.NEGATIVE_BUTTON_TEXT,
                                getString(R.string.cancel))
                        .build();
        modalDialogManager.showDialog(dialog, ModalDialogManager.ModalDialogType.APP);
    }

    /** Returns whether tab groups are currently being synced to the user's Brave account. */
    private boolean isTabGroupSyncActive() {
        if (sTabGroupSyncActiveForTesting != null) {
            return sTabGroupSyncActiveForTesting;
        }
        @Nullable SyncService syncService = SyncServiceFactory.getForProfile(getProfile());
        return syncService != null
                && syncService.getActiveDataTypes().contains(DataType.SAVED_TAB_GROUP);
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    static void setTabGroupSyncActiveForTesting(boolean active) {
        sTabGroupSyncActiveForTesting = active;
        ResettersForTesting.register(() -> sTabGroupSyncActiveForTesting = null);
    }

    /**
     * Configures the "Show synced tab groups" switch. It is independent of the "Enable tab groups"
     * master switch: with tab groups disabled the synced tab groups are still shown while this
     * switch is on, and stay hidden while it is off. The switch is backed by the upstream {@link
     * Pref#AUTO_OPEN_SYNCED_TAB_GROUPS} pref, which is what decides whether a synced tab group gets
     * opened in this browser.
     */
    private void configureShowSyncedTabGroupsSwitch() {
        ChromeSwitchPreference showSyncedTabGroupsSwitch =
                assertNonNull(findPreference(PREF_SHOW_SYNCED_TAB_GROUPS_SWITCH));
        if (!isTabGroupSyncConfigurable(getProfile())) {
            showSyncedTabGroupsSwitch.setVisible(false);
            return;
        }

        showSyncedTabGroupsSwitch.setVisible(true);
        PrefService prefService = UserPrefs.get(getProfile());
        showSyncedTabGroupsSwitch.setChecked(
                prefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS));
        showSyncedTabGroupsSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    boolean enabled = (boolean) newValue;
                    prefService.setBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS, enabled);
                    RecordHistogram.recordBooleanHistogram(
                            "Tabs.AutoOpenSyncedTabGroupsSwitch.ToggledToState", enabled);
                    return true;
                });
    }

    private void configureTabGroupsBarSwitch() {
        ChromeSwitchPreference tabGroupsBarSwitch =
                assertNonNull(findPreference(PREF_TAB_GROUPS_BAR_SWITCH));
        tabGroupsBarSwitch.setChecked(BraveTabUiFeatureUtilities.isTabGroupsBarPreferenceEnabled());
        tabGroupsBarSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    BraveTabUiFeatureUtilities.setTabGroupsBarEnabled((boolean) newValue);
                    return true;
                });
    }

    private void configureOpenLinksInCurrentTabGroupSwitch() {
        ChromeSwitchPreference openLinksInCurrentTabGroupSwitch =
                assertNonNull(findPreference(PREF_OPEN_LINKS_IN_CURRENT_TAB_GROUP_SWITCH));
        openLinksInCurrentTabGroupSwitch.setChecked(
                BraveTabUiFeatureUtilities.isOpenLinksInCurrentTabGroupEnabled());
        openLinksInCurrentTabGroupSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    BraveTabUiFeatureUtilities.setOpenLinksInCurrentTabGroupEnabled(
                            (boolean) newValue);
                    return true;
                });
    }

    private void configureClosingAllTabsClosesBraveSwitch() {
        ChromeSwitchPreference closingAllTabsClosesBraveSwitch =
                assertNonNull(findPreference(PREF_CLOSING_ALL_TABS_CLOSES_BRAVE));
        closingAllTabsClosesBraveSwitch.setChecked(
                CloseBraveManager.getClosingAllTabsClosesBraveEnabled());
        closingAllTabsClosesBraveSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    CloseBraveManager.setClosingAllTabsClosesBraveEnabled((boolean) newValue);
                    return true;
                });
    }

    private void configureShowUndoWhenTabsClosedSwitch() {
        ChromeSwitchPreference showUndoWhenTabsClosedSwitch =
                assertNonNull(findPreference(PREF_SHOW_UNDO_WHEN_TABS_CLOSED));
        showUndoWhenTabsClosedSwitch.setChecked(
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, true));
        showUndoWhenTabsClosedSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    ChromeSharedPreferences.getInstance()
                            .writeBoolean(
                                    BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED,
                                    (boolean) newValue);
                    return true;
                });
    }

    private void updateTabGroupDependentPreferences() {
        boolean tabGroupsEnabled = BraveTabUiFeatureUtilities.isTabGroupsEnabled();
        setPreferenceEnabled(PREF_TAB_GROUPS_BAR_SWITCH, tabGroupsEnabled);
        setPreferenceEnabled(PREF_OPEN_LINKS_IN_CURRENT_TAB_GROUP_SWITCH, tabGroupsEnabled);
    }

    private void setPreferenceEnabled(String key, boolean enabled) {
        Preference preference = findPreference(key);
        if (preference != null) {
            preference.setEnabled(enabled);
        }
    }

    private void configureTabArchiveSettings() {
        Preference tabArchiveSettingsPref =
                assertNonNull(findPreference(PREF_TAB_ARCHIVE_SETTINGS));

        String summary = getTabArchiveSettingsSummary(getResources());
        tabArchiveSettingsPref.setSummary(summary);
    }

    private static String getTabArchiveSettingsSummary(Resources resources) {
        TabArchiveSettings archiveSettings =
                new TabArchiveSettings(ChromeSharedPreferences.getInstance());
        try {
            return getTabArchiveSettingsSummary(resources, archiveSettings);
        } finally {
            archiveSettings.destroy();
        }
    }

    private static String getTabArchiveSettingsSummary(
            Resources resources, TabArchiveSettings archiveSettings) {
        if (archiveSettings.getArchiveEnabled()) {
            int days = archiveSettings.getArchiveTimeDeltaDays();
            int summaryId = R.plurals.archive_settings_summary;
            return resources.getQuantityString(summaryId, days, days);
        }
        int neverSummaryId = R.string.archive_settings_time_delta_never;
        return resources.getString(neverSummaryId);
    }

    private static boolean isTabGroupSyncConfigurable(Profile profile) {
        return TabGroupSyncFeatures.isTabGroupSyncEnabled(profile);
    }

    public static final ChromeBaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new ChromeBaseSearchIndexProvider(
                    BraveTabsAndTabGroupsSettings.class.getName(),
                    R.xml.brave_tabs_and_tab_groups_preferences) {

                @Override
                public String getPrefFragmentName() {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        return TabsSettings.SEARCH_INDEX_DATA_PROVIDER.getPrefFragmentName();
                    }
                    return super.getPrefFragmentName();
                }

                @Override
                public int getXmlRes() {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        return TabsSettings.SEARCH_INDEX_DATA_PROVIDER.getXmlRes();
                    }
                    return super.getXmlRes();
                }

                @Override
                public String getUniqueId(String childPrefName) {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        return TabsSettings.SEARCH_INDEX_DATA_PROVIDER.getUniqueId(childPrefName);
                    }
                    return super.getUniqueId(childPrefName);
                }

                @Override
                public void registerFragmentHeaders(
                        Context context,
                        SettingsIndexData indexData,
                        Map<String, SearchIndexProvider> providerMap,
                        Set<String> processedFragments) {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        TabsSettings.SEARCH_INDEX_DATA_PROVIDER.registerFragmentHeaders(
                                context, indexData, providerMap, processedFragments);
                        return;
                    }
                    super.registerFragmentHeaders(
                            context, indexData, providerMap, processedFragments);
                }

                @Override
                public void initPreferenceXml(
                        Context context,
                        Profile profile,
                        SettingsIndexData indexData,
                        Map<String, SearchIndexProvider> providerMap) {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        TabsSettings.SEARCH_INDEX_DATA_PROVIDER.initPreferenceXml(
                                context, profile, indexData, providerMap);
                        return;
                    }

                    super.initPreferenceXml(context, profile, indexData, providerMap);
                    String tabsParentId =
                            PreferenceParser.createUniqueId(
                                    MainSettings.class.getName(), MainSettings.PREF_TABS);
                    SettingsIndexData.Entry tabsParentEntry = indexData.getEntry(tabsParentId);
                    if (tabsParentEntry != null) {
                        // The upstream main settings XML still names TabsSettings. Point the
                        // search parent entry at Brave's replacement so breadcrumbs resolve.
                        indexData.updateEntry(
                                tabsParentId,
                                new SettingsIndexData.Entry.Builder(tabsParentEntry)
                                        .setFragment(BraveTabsAndTabGroupsSettings.class.getName())
                                        .build());
                    }
                }

                @Override
                public void updateDynamicPreferences(
                        Context context, SettingsIndexData indexData, Profile profile) {
                    if (!BraveTabUiFeatureUtilities
                            .isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
                        TabsSettings.SEARCH_INDEX_DATA_PROVIDER.updateDynamicPreferences(
                                context, indexData, profile);
                        return;
                    }

                    String id = getUniqueId(PREF_TAB_ARCHIVE_SETTINGS);
                    SettingsIndexData.Entry entry = indexData.getEntry(id);
                    if (entry != null) {
                        indexData.updateEntry(
                                id,
                                new SettingsIndexData.Entry.Builder(entry)
                                        .setSummary(
                                                getTabArchiveSettingsSummary(
                                                        context.getResources()))
                                        .build());
                    }

                    if (!isTabGroupSyncConfigurable(profile)) {
                        indexData.removeEntry(getUniqueId(PREF_SHOW_SYNCED_TAB_GROUPS_SWITCH));
                    }
                }
            };
}
