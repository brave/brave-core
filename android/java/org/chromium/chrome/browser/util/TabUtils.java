/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.content.Context;
import android.view.ContextThemeWrapper;
import android.view.MenuItem;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.PopupMenu;

import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;

public class TabUtils {
    public static void showTabPopupMenu(Context context, View view) {
        ChromeActivity chromeActivity = getChromeActivity();
        Context wrapper = new ContextThemeWrapper(context,
                GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                        ? R.style.NewTabPopupMenuDark
                        : R.style.NewTabPopupMenuLight);
        // Creating the instance of PopupMenu
        PopupMenu popup = new PopupMenu(wrapper, view);
        // Inflating the Popup using xml file
        popup.getMenuInflater().inflate(R.menu.new_tab_menu, popup.getMenu());

        if (chromeActivity != null && chromeActivity.getCurrentTabModel().isIncognito()) {
            popup.getMenu().findItem(R.id.new_tab_menu_id).setVisible(false);
        }
        // registering popup with OnMenuItemClickListener
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.new_tab_menu_id) {
                    openNewTab(chromeActivity, false);
                } else if (id == R.id.new_incognito_tab_menu_id) {
                    openNewTab(chromeActivity, true);
                }
                return true;
            }
        });
        popup.show(); // showing popup menu
    }

    public static void openNewTab() {
        ChromeActivity chromeActivity = getChromeActivity();
        boolean isIncognito =
                chromeActivity != null ? chromeActivity.getCurrentTabModel().isIncognito() : false;
        openNewTab(chromeActivity, isIncognito);
    }

    private static void openNewTab(ChromeActivity chromeActivity, boolean isIncognito) {
        if (chromeActivity == null) return;
        chromeActivity.getTabModelSelector().getModel(isIncognito).commitAllTabClosures();
        chromeActivity.getTabCreator(isIncognito).launchNTP();
    }

    public static ChromeActivity getChromeActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof ChromeActivity)) continue;
            return (ChromeActivity) ref;
        }
        return null;
    }

    public static void enableRewardsButton() {
        ChromeActivity chromeActivity = getChromeActivity();
        if (chromeActivity == null || chromeActivity.getToolbarManager() == null) {
            return;
        }
        View toolbarView = chromeActivity.findViewById(R.id.toolbar);
        if (toolbarView == null) {
            return;
        }
        FrameLayout rewardsLayout = toolbarView.findViewById(R.id.brave_rewards_button_layout);
        if (rewardsLayout == null) {
            return;
        }
        rewardsLayout.setVisibility(View.VISIBLE);
    }
}
