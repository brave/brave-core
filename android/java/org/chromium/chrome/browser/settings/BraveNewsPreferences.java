
/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.EditText;

import androidx.annotation.Nullable;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.preference.SwitchPreference;

import org.chromium.base.ContextUtils;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.brave_news.mojom.PublisherType;
import org.chromium.brave_news.mojom.UserEnabled;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLaunchIntentDispatcher;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.mojom.Url;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class BraveNewsPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener, ConnectionErrorHandler {
    public static final String PREF_TURN_ON_NEWS = "kBraveTodayOptedIn";
    public static final String PREF_SHOW_NEWS = "kNewTabPageShowToday";
    public static final String PREF_SHOW_OPTIN = "show_optin";
    public static final String PREF_SOURCES_SECTION = "your_sources_section";
    public static final String PREF_ADD_SOURCES = "add_source_news";
    private static final String PREF_RSS_SOURCES = "rss_sources";

    private ChromeSwitchPreference mTurnOnNews;
    private ChromeSwitchPreference mShowNews;
    private EditTextPreference mAddSource;
    private PreferenceScreen mMainScreen;
    private PreferenceCategory mRssCategory;
    private PreferenceManager mPreferenceManager;
    private TreeMap<String, List<Publisher>> mCategsPublishers;

    private final HashMap<String, Preference> mRemovedPreferences = new HashMap<>();
    private BraveNewsController mBraveNewsController;
    private PreferenceFragmentCompat mSettingsFragment;

    public static int getPreferenceSummary() {
        return BraveLaunchIntentDispatcher.useCustomTabs() ? R.string.text_on : R.string.text_off;
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_news_preferences);

        InitBraveNewsController();
        mTurnOnNews = (ChromeSwitchPreference) findPreference(PREF_TURN_ON_NEWS);
        mShowNews = (ChromeSwitchPreference) findPreference(PREF_SHOW_NEWS);
        mRssCategory = (PreferenceCategory) findPreference(PREF_SOURCES_SECTION);
        mRssCategory.setOrderingAsAdded(true);
        mSettingsFragment = this;

        mTurnOnNews.setOnPreferenceChangeListener(this);
        mShowNews.setOnPreferenceChangeListener(this);

        mCategsPublishers = new TreeMap<>();
        mBraveNewsController.getPublishers((publishers) -> {
            List<Publisher> allPublishers = new ArrayList<>();
            List<Publisher> categoryPublishers = new ArrayList<>();
            List<Publisher> rssPublishers = new ArrayList<>();
            for (Map.Entry<String, Publisher> entry : publishers.entrySet()) {
                Publisher publisher = entry.getValue();
                if (publisher.type != PublisherType.DIRECT_SOURCE) {
                    categoryPublishers.add(publisher);
                    mCategsPublishers.put(publisher.categoryName, categoryPublishers);
                } else {
                    rssPublishers.add(publisher);
                }
            }
            mCategsPublishers.put("All Sources", allPublishers);
            addCategs(mCategsPublishers);
            addRss(rssPublishers);
        });
    }

    private void addRss(List<Publisher> publishers) {
        for (Publisher publisher : publishers) {
            assert (publisher.type == PublisherType.DIRECT_SOURCE);
            SwitchPreference source = new SwitchPreference(ContextUtils.getApplicationContext());
            boolean enabled = false;
            if (publisher.userEnabledStatus == UserEnabled.ENABLED) {
                enabled = true;
            } else if (publisher.userEnabledStatus == UserEnabled.NOT_MODIFIED) {
                enabled = publisher.isEnabled;
            }
            source.setTitle(publisher.publisherName);
            source.setKey(publisher.publisherName);
            source.setDefaultValue((boolean) true);
            source.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    @UserEnabled.EnumType
                    int type = UserEnabled.NOT_MODIFIED;
                    if ((boolean) newValue) {
                        type = UserEnabled.ENABLED;
                    } else {
                        type = UserEnabled.DISABLED;
                    }

                    SharedPreferencesManager.getInstance().writeBoolean(
                            BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, true);

                    if (mBraveNewsController != null) {
                        mBraveNewsController.setPublisherPref(publisher.publisherId, type);
                    }
                    source.setChecked((boolean) newValue);
                    if (!(boolean) newValue) {
                        mRssCategory.removePreference(source);
                    }
                    return false;
                }
            });
            mRssCategory.addPreference(source);
            source.setChecked(true);
        }
    }

    private void addCategs(TreeMap<String, List<Publisher>> publisherCategories) {
        for (Map.Entry<String, List<Publisher>> map : publisherCategories.entrySet()) {
            String category = map.getKey();
            ChromeBasePreference source =
                    new ChromeBasePreference(ContextUtils.getApplicationContext());
            source.setTitle(category);
            source.setKey(category);
            Bundle prefExtras = source.getExtras();
            prefExtras.putString("category", category);
            source.setFragment("org.chromium.chrome.browser.settings.BraveNewsCategorySources");
            source.setVisible(BravePrefServiceBridge.getInstance().getShowNews()
                    && BravePrefServiceBridge.getInstance().getNewsOptIn());
            mMainScreen.addPreference(source);
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
        InitBraveNewsController();
    }

    private void InitBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        mBraveNewsController =
                BraveNewsControllerFactory.getInstance().getBraveNewsController(this);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_news_title);

        mPreferenceManager = getPreferenceManager();
        mMainScreen = mPreferenceManager.getPreferenceScreen();

        mAddSource = (EditTextPreference) findPreference(PREF_RSS_SOURCES);
        if (mAddSource != null) {
            mAddSource.setPositiveButtonText(R.string.search_title);
            mAddSource.setOnPreferenceChangeListener(this);
            mAddSource.setText("");
        }

        boolean isNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();
        boolean isShowNewsOn = BravePrefServiceBridge.getInstance().getShowNews();
        if (isNewsOn) {
            mTurnOnNews.setVisible(false);
            mShowNews.setVisible(true);
            if (isShowNewsOn) {
                mShowNews.setChecked(true);
            }
            setSourcesVisibility(isShowNewsOn);
        } else {
            mTurnOnNews.setChecked(false);
            mShowNews.setVisible(false);
            setSourcesVisibility(isNewsOn);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_TURN_ON_NEWS.equals(key)) {
            if ((boolean) newValue) {
                mTurnOnNews.setVisible(false);
                mShowNews.setVisible(true);
                mShowNews.setChecked(true);
                BravePrefServiceBridge.getInstance().setNewsOptIn(true);
                BravePrefServiceBridge.getInstance().setShowNews(true);
                SharedPreferences.Editor sharedPreferencesEditor =
                        ContextUtils.getAppSharedPreferences().edit();
                sharedPreferencesEditor.putBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, false);
                sharedPreferencesEditor.apply();
            }
        } else if (PREF_SHOW_NEWS.equals(key)) {
            SharedPreferences.Editor sharedPreferencesEditor =
                    ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(BraveNewsPreferences.PREF_SHOW_OPTIN, false);
            sharedPreferencesEditor.apply();
            BravePrefServiceBridge.getInstance().setShowNews((boolean) newValue);

        } else if (PREF_RSS_SOURCES.equals(key)) {
            if (((String) newValue).equals("")) {
                return true;
            }
            PreferenceManager manager = getPreferenceManager();
            PreferenceScreen sourcesScreen =
                    manager.createPreferenceScreen(ContextUtils.getApplicationContext());
            sourcesScreen.setTitle((String) newValue);
            // fetch results from API
            Url rssUrl = new Url();

            rssUrl.url = (String) newValue;
            mBraveNewsController.subscribeToNewDirectFeed(
                    rssUrl, (isValidFeed, isDuplicate, result) -> {
                        if (isValidFeed && !isDuplicate && result != null) {
                            SharedPreferencesManager.getInstance().writeBoolean(
                                    BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, true);
                            getActivity().finish();
                            SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
                            settingsLauncher.launchSettingsActivity(
                                    getActivity(), BraveNewsPreferences.class);
                        }
                    });
            return true;
        }
        setSourcesVisibility((boolean) newValue);
        return true;
    }

    private void setSourcesVisibility(boolean isNewsShown) {
        for (int i = 0; i < getPreferenceScreen().getPreferenceCount(); i++) {
            Preference pref = getPreferenceScreen().getPreference(i);
            if (!pref.getKey().equals(PREF_SHOW_NEWS) && !pref.getKey().equals(PREF_TURN_ON_NEWS)) {
                pref.setVisible(isNewsShown);
            }
        }
    }

    private void createRssDialog(String newValue) {}

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
    }

    /**
     *  We need to override it to avoid NullPointerException in Chromium's child classes
     */
    @Override
    public <T extends Preference> T findPreference(CharSequence key) {
        T result = super.findPreference(key);
        if (result == null) {
            result = (T) mRemovedPreferences.get((String) key);
        }
        return result;
    }
}
