/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.searchwidget;

import android.app.SearchManager;
import android.content.Intent;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityConstants;

public class SearchWidgetProviderActivity extends SearchActivity {
    @Override
    public void onNewIntent(Intent intent) {
        Intent newIntent = new Intent();
        if (IntentUtils.safeGetStringExtra(intent, SearchManager.QUERY) != null) {
            newIntent.putExtra(SearchManager.QUERY,
                    IntentUtils.safeGetStringExtra(intent, SearchManager.QUERY));
        }

        if (intent.getAction().equals(SearchActivityConstants.ACTION_START_TEXT_SEARCH)) {
            newIntent.setAction(SearchActivityConstants.ACTION_START_TEXT_SEARCH);
        }
        super.onNewIntent(newIntent);
    }
}
