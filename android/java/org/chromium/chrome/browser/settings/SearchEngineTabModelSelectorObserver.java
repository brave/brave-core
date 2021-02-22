/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabCreationState;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;

/**
 *  Set proper active default search engine(DSE) provider when current TabModel is changed.
 *  Whenever current TabModel is changed, we should set appropriate DSE because
 *  we use different DSE for normal and private tab.
 */
public class SearchEngineTabModelSelectorObserver implements TabModelSelectorObserver {
    private TabModelSelector mTabModelSelector;

    public SearchEngineTabModelSelectorObserver(TabModelSelector tabModelSelector) {
        mTabModelSelector = tabModelSelector;
    }

    @Override
    public void onChange() {}

    @Override
    public void onNewTabCreated(Tab tab, @TabCreationState int creationState) {}

    @Override
    public void onTabModelSelected(TabModel newModel, TabModel oldModel) {
        BraveSearchEngineUtils.updateActiveDSE(mTabModelSelector.isIncognitoSelected());
    }

    @Override
    public void onTabStateInitialized() {}

    @Override
    public void onTabHidden(Tab tab) {}
}
