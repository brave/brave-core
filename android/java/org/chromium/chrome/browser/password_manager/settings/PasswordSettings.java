// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.chrome.browser.password_manager.PasswordMetricsUtil.PASSWORD_SETTINGS_EXPORT_METRICS_ID;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;

import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.FragmentManager;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceGroup;

import org.chromium.base.DeviceInfo;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.password_manager.BravePasswordManagerHelper;
import org.chromium.chrome.browser.password_manager.ManagePasswordsReferrer;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.chrome.browser.settings.ChromeManagedPreferenceDelegate;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SearchUtils;
import org.chromium.components.browser_ui.settings.SettingsFragment.AnimationType;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.TextMessagePreference;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

import java.util.Locale;

/**
 * The "Passwords" screen in Settings, which allows the user to enable or disable password saving,
 * to view saved passwords (just the username and URL), and to delete saved passwords.
 *
 * <p>TODO: crbug.com/372657804 - Make sure that the PasswordSettings is not created in UPM M4.1
 */
@NullMarked
public class PasswordSettings extends ChromeBaseSettingsFragment
        implements PasswordListObserver,
                Preference.OnPreferenceChangeListener,
                Preference.OnPreferenceClickListener {

    // Keys for name/password dictionaries.
    public static final String PASSWORD_LIST_URL = "url";
    public static final String PASSWORD_LIST_NAME = "name";
    public static final String PASSWORD_LIST_PASSWORD = "password";

    // Used to pass the password id into a new activity.
    public static final String PASSWORD_LIST_ID = "id";

    // The key for saving |mSearchQuery| to instance bundle.
    private static final String SAVED_STATE_SEARCH_QUERY = "saved-state-search-query";

    public static final String PREF_SAVE_PASSWORDS_SWITCH = "save_passwords_switch";
    public static final String PREF_AUTOSIGNIN_SWITCH = "autosignin_switch";
    public static final String PREF_IMPORT_PASSWORDS = "import_passwords";
    public static final String PREF_EXPORT_PASSWORDS = "export_passwords";

    private static final String PREF_KEY_CATEGORY_SAVED_PASSWORDS = "saved_passwords";
    private static final String PREF_KEY_CATEGORY_EXCEPTIONS = "exceptions";
    private static final String PREF_KEY_SAVED_PASSWORDS_NO_TEXT = "saved_passwords_no_text";

    private static final int ORDER_SAVED_PASSWORDS = 6;
    private static final int ORDER_SAVED_PASSWORDS_NO_TEXT = 7;
    private static final int ORDER_EXCEPTIONS = 8;

    // Unique request code for the password exporting activity.
    private static final int PASSWORD_EXPORT_INTENT_REQUEST_CODE = 3485764;
    // Unique request code for the password importing activity.
    private static final int PASSWORD_IMPORT_INTENT_REQUEST_CODE = 3485765;

    private boolean mNoPasswords;
    private boolean mNoPasswordExceptions;

    private /*@Nullable*/ MenuItem mHelpItem;
    private /*@Nullable*/ MenuItem mSearchItem;

    private @Nullable String mSearchQuery;
    private @Nullable Preference mLinkPref;
    private /*@Nullable*/ Menu mMenu;
    private @Nullable Preference mExportPasswordsPreference;

    private @ManagePasswordsReferrer int mManagePasswordsReferrer;
    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    /** For controlling the UX flow of exporting passwords. */
    private final ExportFlow mExportFlow = new ExportFlow();

    /** For controlling the UX flow of importing passwords. */
    private final ImportFlow mImportFlow = new ImportFlow();

    public ExportFlow getExportFlowForTesting() {
        return mExportFlow;
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        mExportFlow.onCreate(
                savedInstanceState,
                new ExportFlowInterface.Delegate() {
                    @Override
                    public Activity getActivity() {
                        return PasswordSettings.this.getActivity();
                    }

                    @Override
                    public FragmentManager getFragmentManager() {
                        return assertNonNull(PasswordSettings.this.getFragmentManager());
                    }

                    @Override
                    public int getViewId() {
                        return assertNonNull(getView()).getId();
                    }

                    @Override
                    public void runCreateFileOnDiskIntent(Intent intent) {
                        startActivityForResult(intent, PASSWORD_EXPORT_INTENT_REQUEST_CODE);
                    }

                    @Override
                    public Profile getProfile() {
                        return PasswordSettings.this.getProfile();
                    }
                },
                PASSWORD_SETTINGS_EXPORT_METRICS_ID);

        mImportFlow.onCreate(
                savedInstanceState,
                new ImportFlow.Delegate() {
                    @Override
                    public Activity getActivity() {
                        return PasswordSettings.this.getActivity();
                    }

                    @Override
                    public FragmentManager getFragmentManager() {
                        return assertNonNull(PasswordSettings.this.getFragmentManager());
                    }

                    @Override
                    public Profile getProfile() {
                        return PasswordSettings.this.getProfile();
                    }

                    @Override
                    public void runCreateFilePickerIntent(Intent intent) {
                        startActivityForResult(intent, PASSWORD_IMPORT_INTENT_REQUEST_CODE);
                    }
                });
        mPageTitle.set(getString(R.string.password_manager_settings_title));

        // Load preferences from XML instead of creating programmatically
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_password_settings_preferences);

        PasswordManagerHandlerProvider.getForProfile(getProfile()).addObserver(this);
        setHasOptionsMenu(true); // Password Export might be optional but Search is always present.

        mManagePasswordsReferrer = getReferrerFromInstanceStateOrLaunchBundle(savedInstanceState);

        // Set up preference change listeners
        setupPreferenceListeners();

        if (savedInstanceState == null) return;

        if (savedInstanceState.containsKey(SAVED_STATE_SEARCH_QUERY)) {
            mSearchQuery = savedInstanceState.getString(SAVED_STATE_SEARCH_QUERY);
        }
    }

    private void setupPreferenceListeners() {
        // Set up save passwords switch
        ChromeSwitchPreference savePasswordsSwitch =
                (ChromeSwitchPreference) findPreference(PREF_SAVE_PASSWORDS_SWITCH);
        if (savePasswordsSwitch != null) {
            savePasswordsSwitch.setOnPreferenceChangeListener(this);
            savePasswordsSwitch.setManagedPreferenceDelegate(
                    new ChromeManagedPreferenceDelegate(getProfile()) {
                        @Override
                        public boolean isPreferenceControlledByPolicy(Preference preference) {
                            return getPrefService()
                                    .isManagedPreference(Pref.CREDENTIALS_ENABLE_SERVICE);
                        }
                    });
            // Set initial state
            savePasswordsSwitch.setChecked(
                    getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_SERVICE));
        }

        // Set up auto sign-in switch
        ChromeSwitchPreference autoSignInSwitch =
                (ChromeSwitchPreference) findPreference(PREF_AUTOSIGNIN_SWITCH);
        if (autoSignInSwitch != null) {
            if (shouldShowAutoSigninOption()) {
                autoSignInSwitch.setOnPreferenceChangeListener(this);
                autoSignInSwitch.setManagedPreferenceDelegate(
                        new ChromeManagedPreferenceDelegate(getProfile()) {
                            @Override
                            public boolean isPreferenceControlledByPolicy(Preference preference) {
                                return getPrefService()
                                        .isManagedPreference(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN);
                            }
                        });
                // Set initial state
                autoSignInSwitch.setChecked(
                        getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN));
            } else {
                // Hide the auto sign-in switch if not needed
                getPreferenceScreen().removePreference(autoSignInSwitch);
            }
        }

        // Set up no passwords message divider settings
        TextMessagePreference noPasswordsMessage =
                (TextMessagePreference) findPreference(PREF_KEY_SAVED_PASSWORDS_NO_TEXT);
        if (noPasswordsMessage != null) {
            noPasswordsMessage.setDividerAllowedAbove(false);
            noPasswordsMessage.setDividerAllowedBelow(false);
        }

        // Set up import passwords preference
        Preference importPasswordsPreference = findPreference(PREF_IMPORT_PASSWORDS);
        if (importPasswordsPreference != null) {
            importPasswordsPreference.setOnPreferenceClickListener(this);
        }

        // Set up export passwords preference
        mExportPasswordsPreference = findPreference(PREF_EXPORT_PASSWORDS);
        if (mExportPasswordsPreference != null) {
            mExportPasswordsPreference.setOnPreferenceClickListener(this);
            // Set initial enabled state - only show if export is supported
            mExportPasswordsPreference.setVisible(ExportFlow.providesPasswordExport());
            mExportPasswordsPreference.setEnabled(
                    false); // Will be enabled when passwords are available
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_SAVE_PASSWORDS_SWITCH.equals(key)) {
            getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_SERVICE, (boolean) newValue);
            return true;
        } else if (PREF_AUTOSIGNIN_SWITCH.equals(key)) {
            getPrefService().setBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN, (boolean) newValue);
            return true;
        }
        return false;
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private @ManagePasswordsReferrer int getReferrerFromInstanceStateOrLaunchBundle(
            @Nullable Bundle savedInstanceState) {
        if (savedInstanceState != null
                && savedInstanceState.containsKey(
                        BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER)) {
            return savedInstanceState.getInt(BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER);
        }
        Bundle extras = getArguments();
        assert extras.containsKey(BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER)
                : "PasswordSettings must be launched with a manage-passwords-referrer fragment"
                        + "argument, but none was provided.";
        return extras.getInt(BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        // Disable animations of preference changes.
        getListView().setItemAnimator(null);
    }

    @Initializer
    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        menu.clear();
        mMenu = menu;
        inflater.inflate(R.menu.save_password_preferences_action_bar_menu, menu);
        // Hide the export passwords menu item since we now have it as a preference
        menu.findItem(R.id.export_passwords).setVisible(false);
        mSearchItem = menu.findItem(R.id.menu_id_search);
        mSearchItem.setVisible(true);
        mHelpItem = menu.findItem(R.id.menu_id_targeted_help);
        SearchUtils.initializeSearchView(
                mSearchItem, mSearchQuery, getActivity(), this::filterPasswords);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        // Export passwords menu item is now hidden, no need to enable/disable it
        super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (SearchUtils.handleSearchNavigation(item, mSearchItem, mSearchQuery, getActivity())) {
            filterPasswords(null);
            return true;
        }
        if (id == R.id.menu_id_targeted_help) {
            getHelpAndFeedbackLauncher()
                    .show(getActivity(), getString(R.string.help_context_passwords), null);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void filterPasswords(@Nullable String query) {
        mSearchQuery = query;
        mHelpItem.setShowAsAction(
                mSearchQuery == null
                        ? MenuItem.SHOW_AS_ACTION_IF_ROOM
                        : MenuItem.SHOW_AS_ACTION_NEVER);
        rebuildPasswordLists();
    }

    /** Empty screen message when no passwords or exceptions are stored. */
    private void displayEmptyScreenMessage() {
        TextMessagePreference emptyView = new TextMessagePreference(getStyledContext(), null);
        emptyView.setSummary(R.string.saved_passwords_none_text);
        emptyView.setKey(PREF_KEY_SAVED_PASSWORDS_NO_TEXT);
        emptyView.setOrder(ORDER_SAVED_PASSWORDS_NO_TEXT);
        emptyView.setDividerAllowedAbove(false);
        emptyView.setDividerAllowedBelow(false);
        getPreferenceScreen().addPreference(emptyView);
    }

    /** Include a message when there's no match. */
    private void displayPasswordNoResultScreenMessage() {
        Preference noResultView = new Preference(getStyledContext());
        noResultView.setLayoutResource(R.layout.password_no_result);
        noResultView.setSelectable(false);
        getPreferenceScreen().addPreference(noResultView);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        ReauthenticationManager.resetLastReauth();
    }

    void rebuildPasswordLists() {
        mNoPasswords = false;
        mNoPasswordExceptions = false;

        // Clear existing password and exception preferences
        clearPasswordLists();

        PasswordManagerHandlerProvider passwordManagerHandlerProvider =
                assertNonNull(PasswordManagerHandlerProvider.getForProfile(getProfile()));
        PasswordManagerHandler passwordManagerHandler =
                passwordManagerHandlerProvider.getPasswordManagerHandler();
        if (passwordManagerHandler == null) {
            return;
        }

        if (mSearchQuery != null) {
            // Only the filtered passwords and exceptions should be shown.
            getPreferenceScreen().removeAll();
            passwordManagerHandler.updatePasswordLists();
            return;
        }

        if (findPreference(PREF_SAVE_PASSWORDS_SWITCH) == null) {
            getPreferenceScreen().removeAll();
            // We wiped all preferences to show the search result, add them again
            SettingsUtils.addPreferencesFromResource(
                    this, R.xml.brave_password_settings_preferences);
            // And with the listeners
            setupPreferenceListeners();
        }

        // Update preference states
        updatePreferenceStates();
        passwordManagerHandler.updatePasswordLists();
    }

    private boolean shouldShowAutoSigninOption() {
        return !DeviceInfo.isAutomotive();
    }

    private void updateExportPasswordsPreferenceState() {
        if (mExportPasswordsPreference != null) {
            mExportPasswordsPreference.setEnabled(!mNoPasswords && !mExportFlow.isActive());
        }
    }

    private void clearPasswordLists() {
        // Clear saved passwords category
        PreferenceCategory savedPasswordsCategory =
                (PreferenceCategory) findPreference(PREF_KEY_CATEGORY_SAVED_PASSWORDS);
        if (savedPasswordsCategory != null) {
            savedPasswordsCategory.removeAll();
        }

        // Clear exceptions category
        PreferenceCategory exceptionsCategory =
                (PreferenceCategory) findPreference(PREF_KEY_CATEGORY_EXCEPTIONS);
        if (exceptionsCategory != null) {
            exceptionsCategory.removeAll();
        }

        // Remove no entries message
        resetNoEntriesTextMessage();
    }

    private void updatePreferenceStates() {
        // Update save passwords switch state
        ChromeSwitchPreference savePasswordsSwitch =
                (ChromeSwitchPreference) findPreference(PREF_SAVE_PASSWORDS_SWITCH);
        if (savePasswordsSwitch != null) {
            savePasswordsSwitch.setChecked(
                    getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_SERVICE));
        }

        // Update auto sign-in switch state
        ChromeSwitchPreference autoSignInSwitch =
                (ChromeSwitchPreference) findPreference(PREF_AUTOSIGNIN_SWITCH);
        if (autoSignInSwitch != null) {
            autoSignInSwitch.setChecked(
                    getPrefService().getBoolean(Pref.CREDENTIALS_ENABLE_AUTOSIGNIN));
        }
    }

    /**
     * Clears the contents of the saved passwords or exceptions category.
     *
     * @param preferenceCategoryKey The key string identifying the PreferenceCategory to be cleared.
     */
    private void resetList(String preferenceCategoryKey) {
        PreferenceCategory profileCategory =
                (PreferenceCategory) getPreferenceScreen().findPreference(preferenceCategoryKey);
        if (profileCategory != null) {
            profileCategory.removeAll();
        }
    }

    /** Removes the message informing the user that there are no saved entries to display. */
    private void resetNoEntriesTextMessage() {
        Preference message = getPreferenceScreen().findPreference(PREF_KEY_SAVED_PASSWORDS_NO_TEXT);
        if (message != null) {
            getPreferenceScreen().removePreference(message);
        }
    }

    @Override
    public void passwordListAvailable(int count) {
        resetList(PREF_KEY_CATEGORY_SAVED_PASSWORDS);
        resetNoEntriesTextMessage();

        mNoPasswords = count == 0;
        if (mNoPasswords) {
            if (mNoPasswordExceptions) displayEmptyScreenMessage();
            // Update export preference state when no passwords
            updateExportPasswordsPreferenceState();
            return;
        }

        PreferenceGroup passwordParent;
        if (mSearchQuery == null) {
            // Use the existing category from XML instead of creating a new one
            PreferenceCategory profileCategory =
                    (PreferenceCategory) findPreference(PREF_KEY_CATEGORY_SAVED_PASSWORDS);
            if (profileCategory == null) {
                // Fallback: create new category if XML one doesn't exist (shouldn't happen)
                profileCategory = new PreferenceCategory(getStyledContext());
                profileCategory.setKey(PREF_KEY_CATEGORY_SAVED_PASSWORDS);
                profileCategory.setTitle(R.string.password_list_title);
                profileCategory.setOrder(ORDER_SAVED_PASSWORDS);
                getPreferenceScreen().addPreference(profileCategory);
            }
            passwordParent = profileCategory;
        } else {
            passwordParent = getPreferenceScreen();
        }
        PasswordManagerHandlerProvider passwordManagerHandlerProvider =
                assertNonNull(PasswordManagerHandlerProvider.getForProfile(getProfile()));
        PasswordManagerHandler passwordManagerHandler =
                assertNonNull(passwordManagerHandlerProvider.getPasswordManagerHandler());
        for (int i = 0; i < count; i++) {
            SavedPasswordEntry saved = passwordManagerHandler.getSavedPasswordEntry(i);
            String url = saved.getUrl();
            String name = saved.getUserName();
            String password = saved.getPassword();
            if (shouldBeFiltered(url, name)) {
                continue; // The current password won't show with the active filter, try the next.
            }
            Preference preference = new Preference(getStyledContext());
            preference.setTitle(url);
            preference.setOnPreferenceClickListener(this);
            preference.setSummary(name);
            Bundle args = preference.getExtras();
            args.putString(PASSWORD_LIST_NAME, name);
            args.putString(PASSWORD_LIST_URL, url);
            args.putString(PASSWORD_LIST_PASSWORD, password);
            args.putInt(PASSWORD_LIST_ID, i);
            passwordParent.addPreference(preference);
        }
        mNoPasswords = passwordParent.getPreferenceCount() == 0;
        // Update export passwords preference enabled state
        updateExportPasswordsPreferenceState();
        if (mNoPasswords) {
            if (count == 0) displayEmptyScreenMessage(); // Show if the list was already empty.
            if (mSearchQuery == null) {
                // Keep the XML-defined category visible even when empty
                // Don't remove it: getPreferenceScreen().removePreference(passwordParent);
            } else {
                displayPasswordNoResultScreenMessage();
            }
        }
    }

    /**
     * Returns true if there is a search query that requires the exclusion of an entry based on the
     * passed url or name.
     *
     * @param url the visible URL of the entry to check. May be empty but must not be null.
     * @param name the visible user name of the entry to check. May be empty but must not be null.
     * @return Returns whether the entry with the passed url and name should be filtered.
     */
    private boolean shouldBeFiltered(final String url, final String name) {
        if (mSearchQuery == null) {
            return false;
        }
        return !url.toLowerCase(Locale.ENGLISH).contains(mSearchQuery.toLowerCase(Locale.ENGLISH))
                && !name.toLowerCase(Locale.getDefault())
                        .contains(mSearchQuery.toLowerCase(Locale.getDefault()));
    }

    @Override
    public void passwordExceptionListAvailable(int count) {
        if (mSearchQuery != null) return; // Don't show exceptions if a search is ongoing.
        resetList(PREF_KEY_CATEGORY_EXCEPTIONS);
        resetNoEntriesTextMessage();

        mNoPasswordExceptions = count == 0;
        if (mNoPasswordExceptions) {
            if (mNoPasswords) displayEmptyScreenMessage();
            return;
        }

        // Use the existing category from XML instead of creating a new one
        PreferenceCategory profileCategory =
                (PreferenceCategory) findPreference(PREF_KEY_CATEGORY_EXCEPTIONS);
        if (profileCategory == null) {
            // Fallback: create new category if XML one doesn't exist (shouldn't happen)
            profileCategory = new PreferenceCategory(getStyledContext());
            profileCategory.setKey(PREF_KEY_CATEGORY_EXCEPTIONS);
            profileCategory.setTitle(R.string.section_saved_passwords_exceptions);
            profileCategory.setOrder(ORDER_EXCEPTIONS);
            getPreferenceScreen().addPreference(profileCategory);
        }
        PasswordManagerHandlerProvider passwordManagerHandlerProvider =
                assertNonNull(PasswordManagerHandlerProvider.getForProfile(getProfile()));
        PasswordManagerHandler passwordManagerHandler =
                assertNonNull(passwordManagerHandlerProvider.getPasswordManagerHandler());
        for (int i = 0; i < count; i++) {
            String exception = passwordManagerHandler.getSavedPasswordException(i);
            Preference preference = new Preference(getStyledContext());
            preference.setTitle(exception);
            preference.setOnPreferenceClickListener(this);
            Bundle args = preference.getExtras();
            args.putString(PASSWORD_LIST_URL, exception);
            args.putInt(PASSWORD_LIST_ID, i);
            profileCategory.addPreference(preference);
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        rebuildPasswordLists();
    }

    @Override
    public void onResume() {
        super.onResume();
        mExportFlow.onResume();
        // Update export preference state in case export flow state changed
        updateExportPasswordsPreferenceState();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent intent) {
        super.onActivityResult(requestCode, resultCode, intent);

        if (resultCode != Activity.RESULT_OK || intent == null || intent.getData() == null) {
            return;
        }

        if (requestCode == PASSWORD_EXPORT_INTENT_REQUEST_CODE) {
            mExportFlow.savePasswordsToDownloads(intent.getData());
        } else if (requestCode == PASSWORD_IMPORT_INTENT_REQUEST_CODE) {
            mImportFlow.processImportFile(intent);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mExportFlow.onSaveInstanceState(outState);
        mImportFlow.onSaveInstanceState(outState);
        if (mSearchQuery != null) {
            outState.putString(SAVED_STATE_SEARCH_QUERY, mSearchQuery);
        }
        outState.putInt(
                BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER, mManagePasswordsReferrer);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        // The component should only be destroyed when the activity has been closed by the user
        // (e.g. by pressing on the back button) and not when the activity is temporarily destroyed
        // by the system.
        if (getActivity().isFinishing()) {
            PasswordManagerHandlerProvider.getForProfile(getProfile()).removeObserver(this);
        }
    }

    /**
     * Preference was clicked. Either navigate to manage account site or launch the PasswordEditor
     * depending on which preference it was.
     */
    @Override
    public boolean onPreferenceClick(Preference preference) {
        String key = preference.getKey();

        if (PREF_IMPORT_PASSWORDS.equals(key)) {
            // Start the password import flow
            mImportFlow.startImporting();
            return true;
        } else if (PREF_EXPORT_PASSWORDS.equals(key)) {
            // Moved from menu item - export passwords functionality
            RecordHistogram.recordEnumeratedHistogram(
                    mExportFlow.getExportEventHistogramName(),
                    ExportFlow.PasswordExportEvent.EXPORT_OPTION_SELECTED,
                    ExportFlow.PasswordExportEvent.COUNT);
            mExportFlow.startExporting();
            return true;
        } else if (preference == mLinkPref) {
            Intent intent =
                    new Intent(
                            Intent.ACTION_VIEW, Uri.parse(PasswordUiView.getAccountDashboardURL()));
            intent.setPackage(getActivity().getPackageName());
            getActivity().startActivity(intent);
        } else {
            boolean isBlockedCredential =
                    !preference.getExtras().containsKey(PasswordSettings.PASSWORD_LIST_NAME);
            PasswordManagerHandlerProvider passwordManagerHandlerProvider =
                    assertNonNull(PasswordManagerHandlerProvider.getForProfile(getProfile()));
            PasswordManagerHandler passwordManagerHandler =
                    assertNonNull(passwordManagerHandlerProvider.getPasswordManagerHandler());

            passwordManagerHandler.showPasswordEntryEditingView(
                    getActivity(),
                    preference.getExtras().getInt(PasswordSettings.PASSWORD_LIST_ID),
                    isBlockedCredential);
        }
        return true;
    }

    private Context getStyledContext() {
        return getPreferenceManager().getContext();
    }

    private PrefService getPrefService() {
        return UserPrefs.get(getProfile());
    }

    Menu getMenuForTesting() {
        return mMenu;
    }

    Toolbar getToolbarForTesting() {
        return getActivity().findViewById(R.id.action_bar);
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }
}
