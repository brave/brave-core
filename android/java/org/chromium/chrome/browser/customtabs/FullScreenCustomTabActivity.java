/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs;

import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_DARK;
import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_LIGHT;

import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.EXTRA_UI_TYPE;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Browser;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.core.content.ContextCompat;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.customtabs.content.CustomTabActivityTabController;
import org.chromium.chrome.browser.customtabs.features.minimizedcustomtab.CustomTabMinimizationManagerHolder;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbarCoordinator;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.ui.RootUiCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.ui.util.ColorUtils;

/** New Rewards 3.0 custom tab activity */
public class FullScreenCustomTabActivity extends CustomTabActivity {

    // Unused members, never read:
    // - mIsEnterAnimationCompleted
    @SuppressWarnings("UnusedVariable")
    private boolean mIsEnterAnimationCompleted;

    private static final int CLOSE_BUTTON_MARGIN = 16;
    private static final int CLOSE_BUTTON_PADDING = 8;

    private BaseCustomTabRootUiCoordinator mBaseCustomTabRootUiCoordinator;
    private CustomTabToolbarCoordinator mToolbarCoordinator;
    private BrowserServicesIntentDataProvider mIntentDataProvider;
    private CustomTabActivityTabController mTabController;
    private CustomTabMinimizationManagerHolder mMinimizationManagerHolder;
    private CustomTabFeatureOverridesManager mCustomTabFeatureOverridesManager;

    public static boolean sIsFullScreenCustomTabActivityClosed;

    @Override
    public boolean supportsAppMenu() {
        return false;
    }

    @Override
    public void performPostInflationStartup() {

        // Updating the value of mIsEnterAnimationCompleted to true to avoid
        // https://github.com/brave/brave-browser/issues/45005
        mIsEnterAnimationCompleted = true;

        super.performPostInflationStartup();

        View toolbarContainer = findViewById(R.id.toolbar_container);
        if (toolbarContainer != null) {
            toolbarContainer.setVisibility(View.GONE);
        }

        FrameLayout.LayoutParams layoutParams =
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        FrameLayout.LayoutParams.WRAP_CONTENT);
        layoutParams.gravity = Gravity.TOP | Gravity.START;
        layoutParams.setMargins(CLOSE_BUTTON_MARGIN, CLOSE_BUTTON_MARGIN, 0, 0);

        ViewGroup parentView = findViewById(android.R.id.content);
        ImageView closeImg = new ImageView(FullScreenCustomTabActivity.this);
        closeImg.setPadding(
                CLOSE_BUTTON_PADDING,
                CLOSE_BUTTON_PADDING,
                CLOSE_BUTTON_PADDING,
                CLOSE_BUTTON_PADDING);
        closeImg.setImageResource(R.drawable.ic_close);
        closeImg.setColorFilter(
                ContextCompat.getColor(
                        FullScreenCustomTabActivity.this, R.color.schemes_on_surface_variant),
                android.graphics.PorterDuff.Mode.SRC_IN);
        closeImg.setOnClickListener(
                button -> {
                    finish();
                });
        parentView.addView(closeImg, layoutParams);

        int count =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePermissionUtils.REWARDS_NOTIFICATION_PERMISSION_COUNT, 0);

        if (!BravePermissionUtils.hasNotificationPermission(FullScreenCustomTabActivity.this)
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && count < 3) {
            BravePermissionUtils.showNotificationPermissionDialog(FullScreenCustomTabActivity.this);
            ChromeSharedPreferences.getInstance()
                    .writeInt(
                            BravePermissionUtils.REWARDS_NOTIFICATION_PERMISSION_COUNT, count + 1);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        sIsFullScreenCustomTabActivityClosed = false;
    }

    @Override
    public void finish() {
        sIsFullScreenCustomTabActivityClosed = true;
        super.finish();
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == BravePermissionUtils.NOTIFICATION_PERMISSION_CODE
                && grantResults.length != 0
                && grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            Snackbar snackbar =
                    Snackbar.make(
                                    getResources()
                                            .getString(
                                                    R.string
                                                            .enable_notifications_from_brave_to_earn_brave_rewards),
                                    new SnackbarController() {
                                        @Override
                                        public void onDismissNoAction(Object actionData) {}

                                        @Override
                                        public void onAction(Object actionData) {
                                            BravePermissionUtils.notificationSettingPage(
                                                    FullScreenCustomTabActivity.this);
                                        }
                                    },
                                    Snackbar.TYPE_ACTION,
                                    Snackbar.UMA_UNKNOWN)
                            .setAction(
                                    getResources()
                                            .getString(R.string.brave_open_system_sync_settings),
                                    null)
                            .setSingleLine(false)
                            .setDuration(0); // it would use default timing for snackbar

            SnackbarManager snackbarManager = SnackbarManagerProvider.from(getWindowAndroid());
            snackbarManager.showSnackbar(snackbar);
        }
    }

    public static void showPage(Context context, String url) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setClassName(context, FullScreenCustomTabActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.putExtra(CustomTabsIntent.EXTRA_TITLE_VISIBILITY_STATE, CustomTabsIntent.NO_TITLE);
        intent.putExtra(CustomTabsIntent.EXTRA_ENABLE_URLBAR_HIDING, false);
        intent.putExtra(
                CustomTabsIntent.EXTRA_COLOR_SCHEME,
                ColorUtils.inNightMode(context) ? COLOR_SCHEME_DARK : COLOR_SCHEME_LIGHT);
        intent.setData(Uri.parse(url));
        intent.setPackage(context.getPackageName());
        intent.putExtra(
                EXTRA_UI_TYPE, BrowserServicesIntentDataProvider.CustomTabsUiType.INFO_PAGE);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, context.getPackageName());
        if (!(context instanceof Activity)) intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        IntentUtils.addTrustedIntentExtras(intent);

        context.startActivity(intent);
    }

    @Override
    protected RootUiCoordinator createRootUiCoordinator() {
        mBaseCustomTabRootUiCoordinator =
                new FullScreenCustomTabRootUiCoordinator(
                        this,
                        getShareDelegateSupplier(),
                        getActivityTabProvider(),
                        mTabModelProfileSupplier,
                        mBookmarkModelSupplier,
                        mTabBookmarkerSupplier,
                        getTabModelSelectorSupplier(),
                        getBrowserControlsManager(),
                        getWindowAndroid(),
                        getLifecycleDispatcher(),
                        getLayoutManagerSupplier(),
                        /* menuOrKeyboardActionController= */ this,
                        this::getActivityThemeColor,
                        getModalDialogManagerSupplier(),
                        /* appMenuBlocker= */ this,
                        this::supportsAppMenu,
                        this::supportsFindInPage,
                        getTabCreatorManagerSupplier(),
                        getFullscreenManager(),
                        getCompositorViewHolderSupplier(),
                        getTabContentManagerSupplier(),
                        this::getSnackbarManager,
                        mEdgeToEdgeControllerSupplier,
                        getActivityType(),
                        this::isInOverviewMode,
                        /* appMenuDelegate= */ this,
                        /* statusBarColorProvider= */ this,
                        getIntentRequestTracker(),
                        () -> mToolbarCoordinator,
                        () -> mIntentDataProvider,
                        mBackPressManager,
                        () -> mTabController,
                        () -> mMinimizationManagerHolder.getMinimizationManager(),
                        () -> mCustomTabFeatureOverridesManager,
                        () -> getCustomTabActivityNavigationController().openCurrentUrlInBrowser(),
                        getEdgeToEdgeManager());
        return mBaseCustomTabRootUiCoordinator;
    }
}
