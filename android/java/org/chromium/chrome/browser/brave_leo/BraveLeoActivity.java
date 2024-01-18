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
import android.view.MotionEvent;
import android.view.View;

import androidx.browser.customtabs.CustomTabsIntent;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.customtabs.TranslucentCustomTabActivity;
import org.chromium.ui.util.ColorUtils;

/**
 * Brave's Activity for AI Chat
 */
public class BraveLeoActivity extends TranslucentCustomTabActivity {
    // The Activity could be 50% or 100% of the screen. That number
    // indicates that we want it to be 50% on a first start.
    private static final int INITIAL_ACTIVITY_HEIGHT_PX = 300;

    private static final int DRAG_BAR_Y_DELTA_TOLERANCE = 300;
    private int mLastMotionEventAction;
    private boolean mDragInProgress;

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

        mLastMotionEventAction = -1;
        mDragInProgress = false;
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

    private void maybeRedirectToDragBar(MotionEvent ev) {
        View dragBar = findViewById(R.id.drag_bar);
        assert dragBar != null;

        int dragBarLocation[] = new int[2];
        dragBar.getLocationOnScreen(dragBarLocation);
        int dragBarYDelta = (int) ev.getRawY() - dragBarLocation[1];
        if (dragBarYDelta < DRAG_BAR_Y_DELTA_TOLERANCE || mDragInProgress) {
            dragBar.dispatchTouchEvent(ev);
            mDragInProgress = true;
        } else {
            mDragInProgress = false;
        }
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        boolean skipDrag = false;
        if (ev.getActionMasked() == MotionEvent.ACTION_DOWN
                || mLastMotionEventAction == MotionEvent.ACTION_DOWN
                        && ev.getActionMasked() == MotionEvent.ACTION_UP) {
            skipDrag = true;
        }
        if (!skipDrag && getActivityTab() != null) {
            maybeRedirectToDragBar(ev);
        }
        mLastMotionEventAction = ev.getActionMasked();
        if (mLastMotionEventAction != MotionEvent.ACTION_MOVE) {
            mDragInProgress = false;
        }
        return super.dispatchTouchEvent(ev);
    }
}
