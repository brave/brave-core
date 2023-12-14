/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_DARK;
import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_LIGHT;

import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.EXTRA_UI_TYPE;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.view.View;

import androidx.browser.customtabs.CustomTabsIntent;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.customtabs.BaseCustomTabRootUiCoordinator;
import org.chromium.chrome.browser.customtabs.TranslucentCustomTabActivity;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.CustomTabHeightStrategy;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.PartialCustomTabBaseStrategy;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.PartialCustomTabBottomSheetStrategy;
import org.chromium.chrome.browser.customtabs.features.partialcustomtab.PartialCustomTabDisplayManager;
import org.chromium.ui.util.ColorUtils;

/**
 * Brave's Activity for AI Chat
 */
public class BraveLeoActivity extends TranslucentCustomTabActivity {
    // The Activity could be 50% or 100% of the screen. That number
    // indicates that we want it to be 50% on a first start.
    static final int INITIAL_ACTIVITY_HEIGHT_PX = 300;

    private final GestureDetector mGestureDetector;

    // In fact this is PartialCustomTabBottomSheetStrategy.HeightStatus
    private static final int HEIGHT_STATUS_TOP = 0;
    private static final int HEIGHT_STATUS_INITIAL_HEIGHT = 1;
    private static final int HEIGHT_STATUS_TRANSITION = 2;
    private static final int HEIGHT_STATUS_CLOSE = 3;

    public BraveLeoActivity() {
        super();
        mGestureDetector =
                new GestureDetector(ContextUtils.getApplicationContext(), new GestureListener());
    }

    @Override
    public boolean supportsAppMenu() {
        return false;
    }

    @Override
    public int getControlContainerHeightResource() {
        return R.dimen.custom_tabs_control_container_leo_height;
    }

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        View toolbarContainer = findViewById(R.id.toolbar_container);
        if (toolbarContainer != null) {
            toolbarContainer.setVisibility(View.GONE);
        }
    }

    public static void showPage(Context context, String url) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setClassName(context, BraveLeoActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.putExtra(CustomTabsIntent.EXTRA_TITLE_VISIBILITY_STATE, CustomTabsIntent.NO_TITLE);
        intent.putExtra(CustomTabsIntent.EXTRA_ENABLE_URLBAR_HIDING, false);
        intent.putExtra(
                CustomTabsIntent.EXTRA_INITIAL_ACTIVITY_HEIGHT_PX, INITIAL_ACTIVITY_HEIGHT_PX);
        intent.putExtra(CustomTabsIntent.EXTRA_COLOR_SCHEME,
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

    private final class GestureListener extends SimpleOnGestureListener {
        private static final int SWIPE_THRESHOLD = 100;
        private static final int SWIPE_VELOCITY_THRESHOLD = 100;

        @Override
        public boolean onDown(MotionEvent e) {
            return true;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            boolean result = false;
            try {
                float diffY = e2.getY() - e1.getY();
                float diffX = e2.getX() - e1.getX();
                if (Math.abs(diffX) < Math.abs(diffY)
                        && Math.abs(diffY) > SWIPE_THRESHOLD
                        && Math.abs(velocityY) > SWIPE_VELOCITY_THRESHOLD) {
                    if (diffY > 0) {
                        onSwipeBottom();
                    } else {
                        onSwipeTop();
                    }
                    result = true;
                }
            } catch (Exception exception) {
                exception.printStackTrace();
            }
            return result;
        }
    }

    public void onSwipeTop() {
        PartialCustomTabBottomSheetStrategy strategy = getPartialCustomTabBottomSheetStrategy();
        if (strategy != null) {
            int status = getPanelHeightStatus(strategy);
            if (status == HEIGHT_STATUS_INITIAL_HEIGHT || status == HEIGHT_STATUS_TRANSITION) {
                callAnimateTabTo(strategy, HEIGHT_STATUS_TOP);
            }
        }
    }

    public void onSwipeBottom() {
        PartialCustomTabBottomSheetStrategy strategy = getPartialCustomTabBottomSheetStrategy();
        if (strategy != null) {
            int status = getPanelHeightStatus(strategy);
            if (status == HEIGHT_STATUS_TOP || status == HEIGHT_STATUS_TRANSITION) {
                callAnimateTabTo(strategy, HEIGHT_STATUS_INITIAL_HEIGHT);
            }
        }
    }

    private PartialCustomTabBottomSheetStrategy getPartialCustomTabBottomSheetStrategy() {
        CustomTabHeightStrategy customTabHeightStrategy =
                (CustomTabHeightStrategy)
                        BraveReflectionUtil.getField(
                                BaseCustomTabRootUiCoordinator.class,
                                "mCustomTabHeightStrategy",
                                mBaseCustomTabRootUiCoordinator);

        if (customTabHeightStrategy instanceof PartialCustomTabDisplayManager) {
            PartialCustomTabDisplayManager partialCustomTabDisplayManager =
                    (PartialCustomTabDisplayManager) customTabHeightStrategy;
            PartialCustomTabBaseStrategy partialCustomTabBaseStrategy =
                    (PartialCustomTabBaseStrategy)
                            BraveReflectionUtil.getField(
                                    PartialCustomTabDisplayManager.class,
                                    "mStrategy",
                                    partialCustomTabDisplayManager);
            if (partialCustomTabBaseStrategy instanceof PartialCustomTabBottomSheetStrategy) {
                PartialCustomTabBottomSheetStrategy partialCustomTabBottomSheetStrategy =
                        (PartialCustomTabBottomSheetStrategy) partialCustomTabBaseStrategy;
                return partialCustomTabBottomSheetStrategy;
            }
        }
        return null;
    }

    private void callAnimateTabTo(PartialCustomTabBottomSheetStrategy strategy, int heightStatus) {

        BraveReflectionUtil.InvokeMethod(
                PartialCustomTabBottomSheetStrategy.class,
                strategy,
                "animateTabTo",
                int.class,
                heightStatus,
                boolean.class,
                true);
    }

    private int getPanelHeightStatus(PartialCustomTabBottomSheetStrategy strategy) {
        Object status =
                BraveReflectionUtil.getField(
                        PartialCustomTabBottomSheetStrategy.class, "mStatus", strategy);
        return (int) status;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        mGestureDetector.onTouchEvent(ev);
        return super.dispatchTouchEvent(ev);
    }
}
