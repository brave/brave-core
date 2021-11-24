/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.widget.EditText;

import androidx.preference.CheckBoxPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;

import org.chromium.base.ContextUtils;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLaunchIntentDispatcher;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.Map;

public class BraveAddNewsSources extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener, ConnectionErrorHandler {
    private static final String PREF_ADD_SOURCES = "news_source_1";
    private EditTextPreference addSource;
    private EditText mEditText;
    private PreferenceScreen mainScreen;
    private BraveNewsController mBraveNewsController;
    private ArrayList<Publisher> mPublishers;

    public static int getPreferenceSummary() {
        return BraveLaunchIntentDispatcher.useCustomTabs() ? R.string.text_on : R.string.text_off;
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
        getActivity().setTitle(R.string.news_add_source);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_news_sources_default);
        findPreference(PREF_ADD_SOURCES).setOnPreferenceChangeListener(this);
        InitBraveNewsController();

        mainScreen = getPreferenceManager().getPreferenceScreen();

        addSource = (EditTextPreference) findPreference(PREF_ADD_SOURCES);
        addSource.setPositiveButtonText(R.string.search_title);
        mPublishers = new ArrayList<>();

        mBraveNewsController.getPublishers((publishers) -> {
            for (Map.Entry<String, Publisher> entry : publishers.entrySet()) {
                String key = entry.getKey();
                Publisher publisher = entry.getValue();
                mPublishers.add(publisher);
                CheckBoxPreference source =
                        new CheckBoxPreference(ContextUtils.getApplicationContext());
                source.setTitle(publisher.publisherName);
                mainScreen.addPreference(source);
            }
        });
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();

        if (PREF_ADD_SOURCES.equals(key)) {
            PreferenceManager manager = getPreferenceManager();
            PreferenceScreen sourcesScreen =
                    manager.createPreferenceScreen(ContextUtils.getApplicationContext());
            sourcesScreen.setTitle((String) newValue);
            // fetch results from API
            // populate checkboxes
            CheckBoxPreference source1 =
                    new CheckBoxPreference(ContextUtils.getApplicationContext());
            source1.setTitle((String) newValue + " 1");
            source1.setChecked(true);
            sourcesScreen.addPreference(source1);

            CheckBoxPreference source2 =
                    new CheckBoxPreference(ContextUtils.getApplicationContext());
            source2.setTitle((String) newValue + " 2");
            source2.setChecked(true);
            sourcesScreen.addPreference(source2);

            // end fetch. finish the layout

            Preference button = new Preference(ContextUtils.getApplicationContext());
            button.setTitle("Add");
            button.setKey("add_news_source");
            button.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    setPreferencesFromResource(R.xml.brave_news_preferences, null);
                    return true;
                }
            });
            sourcesScreen.addPreference(button);

            setPreferenceScreen(sourcesScreen);
            return true;
        }
        return true;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
    }

    private ArrayList<Preference> getPreferenceList(Preference p, ArrayList<Preference> list) {
        if (p instanceof PreferenceCategory || p instanceof PreferenceScreen) {
            PreferenceGroup pGroup = (PreferenceGroup) p;
            int pCount = pGroup.getPreferenceCount();
            for (int i = 0; i < pCount; i++) {
                getPreferenceList(pGroup.getPreference(i), list); // recursive call
            }
        } else {
            list.add(p);
        }
        return list;
    }
}
