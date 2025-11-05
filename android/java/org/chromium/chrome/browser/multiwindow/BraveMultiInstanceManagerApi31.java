/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.multiwindow;

import static org.chromium.build.NullUtil.assertNonNull;

import android.app.Activity;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.tabmodel.TabModelOrchestrator;
import org.chromium.chrome.browser.app.tabwindow.TabWindowManagerSingleton;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.List;
import java.util.function.Supplier;

@NullMarked
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
    public void moveTabsToWindow(InstanceInfo info, List<Tab> tabs, int tabAtIndex) {
        super.moveTabsToWindow(info, tabs, tabAtIndex);

        if (mIsMoveTabsFromSettings && !tabs.isEmpty()) {
            mIsMoveTabsFromSettings = false;
            TabModelSelector selector =
                    TabWindowManagerSingleton.getInstance().getTabModelSelectorById(mInstanceId);
            if (selector != null && selector.getTotalTabCount() == 0) {
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
                                            public void onDismissNoAction(
                                                    @Nullable Object actionData) {}

                                            // TODO(https://github.com/brave/brave-browser/issues/50673)
                                            // BraveMultiWindowUtils.mergeWindows takes non-null
                                            // activity and can't do anything when null is passed
                                            @SuppressWarnings("NullAway")
                                            @Override
                                            public void onAction(@Nullable Object actionData) {
                                                BraveMultiWindowUtils.mergeWindows(null);
                                            }
                                        },
                                        Snackbar.TYPE_ACTION,
                                        Snackbar.UMA_UNKNOWN)
                                .setAction(
                                        mActivity.getResources().getString(R.string.merge_windows),
                                        null)
                                .setDefaultLines(false)
                                .setDuration(10000);
                Tab firstTab = tabs.get(0);
                assertNonNull(firstTab);
                WindowAndroid windowAndroid = firstTab.getWindowAndroid();
                assertNonNull(windowAndroid);
                SnackbarManager snackbarManager = SnackbarManagerProvider.from(windowAndroid);
                assertNonNull(snackbarManager);
                snackbarManager.showSnackbar(snackbar);
            }
        }
    }
}
