/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.multiwindow;

import android.app.Activity;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.tabmodel.TabModelOrchestrator;
import org.chromium.chrome.browser.app.tabmodel.TabWindowManagerSingleton;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.modaldialog.ModalDialogManager;

class BraveMultiInstanceManagerApi31 extends MultiInstanceManagerApi31 {

    private static final String TAG = "MultiInstanceApi31";
    private int mInstanceId;
    private boolean mIsMoveTabsFromSettings;

    BraveMultiInstanceManagerApi31(
            Activity activity,
            ObservableSupplier<TabModelOrchestrator> tabModelOrchestratorSupplier,
            MultiWindowModeStateDispatcher multiWindowModeStateDispatcher,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            ObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            MenuOrKeyboardActionController menuOrKeyboardActionController,
            Supplier<DesktopWindowStateManager> desktopWindowStateManagerSupplier) {
        super(
                activity,
                tabModelOrchestratorSupplier,
                multiWindowModeStateDispatcher,
                activityLifecycleDispatcher,
                modalDialogManagerSupplier,
                menuOrKeyboardActionController,
                desktopWindowStateManagerSupplier);
    }

    @Override
    public boolean handleMenuOrKeyboardAction(int id, boolean fromMenu) {
        if (id == org.chromium.chrome.R.id.move_to_other_window_menu_id) {
            mIsMoveTabsFromSettings = !fromMenu;
        }
        return super.handleMenuOrKeyboardAction(id, fromMenu);
    }

    @Override
    public void moveTabAction(InstanceInfo info, Tab tab, int tabAtIndex) {
        super.moveTabAction(info, tab, tabAtIndex);
        if (mIsMoveTabsFromSettings) {
            mIsMoveTabsFromSettings = false;
            TabModelSelector selector =
                    TabWindowManagerSingleton.getInstance().getTabModelSelectorById(mInstanceId);
            if (selector.getTotalTabCount() == 0) {
                closeInstance(mInstanceId, INVALID_TASK_ID);
            }
            if (MultiWindowUtils.getInstanceCount() == 1) {
                BraveMultiWindowUtils.updateEnableMultiWindows(false);
            } else {
                Snackbar snackbar =
                        Snackbar.make(
                                        mActivity
                                                .getResources()
                                                .getString(R.string.merge_windows_snackbar),
                                        new SnackbarController() {
                                            @Override
                                            public void onDismissNoAction(Object actionData) {}

                                            @Override
                                            public void onAction(Object actionData) {
                                                BraveMultiWindowUtils.mergeWindows(null);
                                            }
                                        },
                                        Snackbar.TYPE_ACTION,
                                        Snackbar.UMA_UNKNOWN)
                                .setAction(
                                        mActivity.getResources().getString(R.string.merge_windows),
                                        null)
                                .setSingleLine(false)
                                .setDuration(10000);
                SnackbarManager snackbarManager =
                        SnackbarManagerProvider.from(tab.getWindowAndroid());
                snackbarManager.showSnackbar(snackbar);
            }
        }
    }
}
