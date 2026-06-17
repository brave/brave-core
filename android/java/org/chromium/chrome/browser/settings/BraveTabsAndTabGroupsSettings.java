/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assertNonNull;

import android.content.Context;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.auxiliary_search.AuxiliarySearchConfigManager;
import org.chromium.chrome.browser.auxiliary_search.AuxiliarySearchControllerFactory;
import org.chromium.chrome.browser.auxiliary_search.AuxiliarySearchUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.search.ChromeBaseSearchIndexProvider;
import org.chromium.chrome.browser.tab.TabArchiveSettings;
import org.chromium.chrome.browser.tab_group_sync.TabGroupSyncFeatures;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.tasks.tab_management.TabsSettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.TextMessagePreference;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;

import java.util.Map;
import java.util.Set;

/** Brave-owned Tabs and tab groups settings screen. */
@NullMarked
public class BraveTabsAndTabGroupsSettings extends BravePreferenceFragment {
    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_ENABLE_TAB_GROUPS_SWITCH = "enable_tab_groups";

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static final String PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH =
            "auto_open_synced_tab_groups_switch";

    private static final String PREF_SHARE_TITLES_AND_URLS_WITH_OS_SWITCH =
            "share_titles_and_urls_with_os_switch";

    private static final String PREF_SHARE_TITLES_AND_URLS_WITH_OS_LEARN_MORE =
            "share_titles_and_urls_with_os_learn_more";

    private static final String LEARN_MORE_URL =
            "https://support.google.com/chrome/?p=share_titles_urls";

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
        configureAutoOpenSyncedTabGroupsSwitch();
        configureTabGroupsBarSwitch();
        configureOpenLinksInCurrentTabGroupSwitch();
        configureClosingAllTabsClosesBraveSwitch();
        configureShowUndoWhenTabsClosedSwitch();
        configureShareTitlesAndUrlsWithOsSwitch();
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

    public static boolean isBraveAndroidTabGroupsSettingsEnabled() {
        return ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_TAB_GROUPS_SETTINGS);
    }

    private void configureEnableTabGroupsSwitch() {
        ChromeSwitchPreference enableTabGroupsSwitch =
                assertNonNull(findPreference(PREF_ENABLE_TAB_GROUPS_SWITCH));
        enableTabGroupsSwitch.setChecked(BraveTabUiFeatureUtilities.isTabGroupsEnabled());
        enableTabGroupsSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    BraveTabUiFeatureUtilities.setTabGroupsEnabled((boolean) newValue);
                    updateTabGroupDependentPreferences();
                    return true;
                });
    }

    private void configureAutoOpenSyncedTabGroupsSwitch() {
        ChromeSwitchPreference autoOpenSyncedTabGroupsSwitch =
                assertNonNull(findPreference(PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH));
        // LINT.IfChange(isTabGroupSyncAutoOpenConfigurable)
        if (!isTabGroupSyncAutoOpenConfigurable(getProfile())) {
            autoOpenSyncedTabGroupsSwitch.setVisible(false);
            return;
        }
        // LINT.ThenChange(:isTabGroupSyncAutoOpenConfigurableIndex)

        autoOpenSyncedTabGroupsSwitch.setVisible(true);
        PrefService prefService = UserPrefs.get(getProfile());
        autoOpenSyncedTabGroupsSwitch.setChecked(
                prefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS));
        autoOpenSyncedTabGroupsSwitch.setOnPreferenceChangeListener(
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

    private void configureShareTitlesAndUrlsWithOsSwitch() {
        ChromeSwitchPreference shareTitlesAndUrlsWithOsSwitch =
                assertNonNull(findPreference(PREF_SHARE_TITLES_AND_URLS_WITH_OS_SWITCH));
        TextMessagePreference learnMoreTextMessagePreference =
                assertNonNull(findPreference(PREF_SHARE_TITLES_AND_URLS_WITH_OS_LEARN_MORE));

        // LINT.IfChange(isShareTitlesAndUrlsEnabled)
        if (!isShareTitlesAndUrlsEnabled()) {
            shareTitlesAndUrlsWithOsSwitch.setVisible(false);
            learnMoreTextMessagePreference.setVisible(false);
            return;
        }
        // LINT.ThenChange(:isShareTitlesAndUrlsEnabledIndex)

        shareTitlesAndUrlsWithOsSwitch.setVisible(true);
        learnMoreTextMessagePreference.setVisible(true);
        shareTitlesAndUrlsWithOsSwitch.setChecked(AuxiliarySearchUtils.isShareTabsWithOsEnabled());
        shareTitlesAndUrlsWithOsSwitch.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    AuxiliarySearchConfigManager.getInstance()
                            .notifyShareTabsStateChanged((boolean) newValue);
                    return true;
                });

        CharSequence summary = learnMoreTextMessagePreference.getSummary();
        learnMoreTextMessagePreference.setSummary(
                SpanApplier.applySpans(
                        assertNonNull(summary).toString(),
                        new SpanApplier.SpanInfo(
                                "<link>",
                                "</link>",
                                new ChromeClickableSpan(getContext(), this::onLearnMoreClicked))));
    }

    private void onLearnMoreClicked(View view) {
        getCustomTabLauncher().openUrlInCct(getContext(), LEARN_MORE_URL);
    }

    private void updateTabGroupDependentPreferences() {
        boolean tabGroupsEnabled = BraveTabUiFeatureUtilities.isTabGroupsEnabled();
        setPreferenceEnabled(PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH, tabGroupsEnabled);
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
            if (archiveSettings.getArchiveEnabled()) {
                int days = archiveSettings.getArchiveTimeDeltaDays();
                int summaryId = R.plurals.archive_settings_summary;
                return resources.getQuantityString(summaryId, days, days);
            }
            int neverSummaryId = R.string.archive_settings_time_delta_never;
            return resources.getString(neverSummaryId);
        } finally {
            archiveSettings.destroy();
        }
    }

    private static boolean isTabGroupSyncAutoOpenConfigurable(Profile profile) {
        return TabGroupSyncFeatures.isTabGroupSyncEnabled(profile);
    }

    private static boolean isShareTitlesAndUrlsEnabled() {
        return AuxiliarySearchControllerFactory.getInstance().isEnabledAndDeviceCompatible();
    }

    public static final ChromeBaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new ChromeBaseSearchIndexProvider(
                    BraveTabsAndTabGroupsSettings.class.getName(),
                    R.xml.brave_tabs_and_tab_groups_preferences) {

                @Override
                public String getPrefFragmentName() {
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
                        return TabsSettings.SEARCH_INDEX_DATA_PROVIDER.getPrefFragmentName();
                    }
                    return super.getPrefFragmentName();
                }

                @Override
                public int getXmlRes() {
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
                        return TabsSettings.SEARCH_INDEX_DATA_PROVIDER.getXmlRes();
                    }
                    return super.getXmlRes();
                }

                @Override
                public String getUniqueId(String childPrefName) {
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
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
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
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
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
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
                    if (!isBraveAndroidTabGroupsSettingsEnabled()) {
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

                    // LINT.IfChange(isTabGroupSyncAutoOpenConfigurableIndex)
                    if (!isTabGroupSyncAutoOpenConfigurable(profile)) {
                        indexData.removeEntry(getUniqueId(PREF_AUTO_OPEN_SYNCED_TAB_GROUPS_SWITCH));
                    }
                    // LINT.ThenChange(:isTabGroupSyncAutoOpenConfigurable)

                    // LINT.IfChange(isShareTitlesAndUrlsEnabledIndex)
                    if (!isShareTitlesAndUrlsEnabled()) {
                        indexData.removeEntry(
                                getUniqueId(PREF_SHARE_TITLES_AND_URLS_WITH_OS_SWITCH));
                    }

                    // It's not useful for "Learn more" text pref to be searchable.
                    indexData.removeEntry(
                            getUniqueId(PREF_SHARE_TITLES_AND_URLS_WITH_OS_LEARN_MORE));
                    // LINT.ThenChange(:isShareTitlesAndUrlsEnabled)
                }
            };
}
