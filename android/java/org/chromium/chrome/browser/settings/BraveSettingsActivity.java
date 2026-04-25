/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;

public class BraveSettingsActivity extends SettingsActivity {
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.clear();
        getMenuInflater().inflate(R.menu.exit_settings_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.removeItem(R.id.menu_id_general_help);
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.close_menu_id) {
            Intent intent = new Intent(this, ChromeTabbedActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            intent.setAction(Intent.ACTION_VIEW);
            startActivity(intent);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
