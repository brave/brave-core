
/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.widget.EditText;

import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.preference.SwitchPreference;

import org.chromium.base.ContextUtils;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLaunchIntentDispatcher;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

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

    private ChromeSwitchPreference mTurnOnNews;
    private ChromeSwitchPreference mShowNews;
    private PreferenceScreen mMainScreen;
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
        mSettingsFragment = this;

        mTurnOnNews.setOnPreferenceChangeListener(this);
        mShowNews.setOnPreferenceChangeListener(this);

        mCategsPublishers = new TreeMap<>();
        mBraveNewsController.getPublishers((publishers) -> {
            List<Publisher> allPublishers = new ArrayList<>();
            List<Publisher> categoryPublishers = new ArrayList<>();
            for (Map.Entry<String, Publisher> entry : publishers.entrySet()) {
                Publisher publisher = entry.getValue();
                categoryPublishers.add(publisher);
                mCategsPublishers.put(publisher.categoryName, categoryPublishers);
            }
            mCategsPublishers.put("All Sources", allPublishers);
            addCategs(mCategsPublishers);
        });
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

        boolean isNewsOn = BravePrefServiceBridge.getInstance().getNewsOptIn();

        if (!isNewsOn) {
            mTurnOnNews.setChecked(false);
            mShowNews.setVisible(false);
        } else {
            mTurnOnNews.setChecked(true);
            mTurnOnNews.setVisible(false);
            mShowNews.setVisible(true);
            if (BravePrefServiceBridge.getInstance().getShowNews()) {
                mShowNews.setChecked(true);
            } else {
                mShowNews.setChecked(false);
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        if (PREF_TURN_ON_NEWS.equals(key)) {
            BravePrefServiceBridge.getInstance().setNewsOptIn((boolean) newValue);
            if ((boolean) newValue) {
                mShowNews.setVisible(true);
                mShowNews.setChecked(true);
                BravePrefServiceBridge.getInstance().setShowNews(true);
            } else {
                mShowNews.setVisible(false);
            }
        } else if (PREF_SHOW_NEWS.equals(key)) {
            BravePrefServiceBridge.getInstance().setShowNews((boolean) newValue);
            for (int i = 0; i < getPreferenceScreen().getPreferenceCount(); i++) {
                Preference pref = getPreferenceScreen().getPreference(i);
                if (!pref.getKey().equals(PREF_SHOW_NEWS)
                        && !pref.getKey().equals(PREF_TURN_ON_NEWS)) {
                    pref.setVisible((boolean) newValue);
                }
            }
        }
        return true;
    }

    private void removePreferenceIfPresent(String key) {
        Preference preference = getPreferenceScreen().findPreference(key);
        if (preference != null) {
            getPreferenceScreen().removePreference(preference);
            mRemovedPreferences.put(preference.getKey(), preference);
        }
    }

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
