/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs;

import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_DARK;
import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_LIGHT;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.browser.customtabs.CustomTabsIntent;
import androidx.core.content.ContextCompat;

import org.chromium.base.IntentUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSwipeRefreshHandler;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.SwipeRefreshHandler;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.util.ColorUtils;

@NullMarked
public class BraveAccountCustomTabActivity extends CustomTabActivity {
    private static final int HORIZONTAL_MARGIN_DP = 12;

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        Tab tab = getActivityTab();
        assert tab != null;
        // Due to bytecode manipulation, SwipeRefreshHandler instances
        // are actually BraveSwipeRefreshHandler at runtime.
        BraveSwipeRefreshHandler handler = (BraveSwipeRefreshHandler) SwipeRefreshHandler.get(tab);
        assert handler != null;
        handler.mIgnorePullToRefresh = true;

        // Hide the toolbar container
        View toolbarContainer = findViewById(R.id.toolbar_container);
        if (toolbarContainer != null) toolbarContainer.setVisibility(View.GONE);

        TextView title = new TextView(this);
        title.setText(R.string.prefs_brave_account);
        title.setTextAppearance(R.style.TextAppearance_Headline_Primary);
        title.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams titleParams =
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        Gravity.CENTER_VERTICAL | Gravity.START);
        titleParams.setMarginStart(dpToPx(this, HORIZONTAL_MARGIN_DP));
        title.setLayoutParams(titleParams);

        ImageView closeButton = new ImageView(this);
        closeButton.setImageResource(R.drawable.btn_close);
        closeButton.setColorFilter(
                ContextCompat.getColor(this, R.color.schemes_on_surface),
                android.graphics.PorterDuff.Mode.SRC_IN);
        closeButton.setOnClickListener(button -> finish());
        FrameLayout.LayoutParams closeButtonParams =
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        FrameLayout.LayoutParams.WRAP_CONTENT,
                        Gravity.CENTER_VERTICAL | Gravity.END);
        closeButtonParams.setMarginEnd(dpToPx(this, HORIZONTAL_MARGIN_DP));
        closeButton.setLayoutParams(closeButtonParams);

        FrameLayout header = new FrameLayout(this);
        header.setLayoutParams(
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT, getActionBarSize(), Gravity.TOP));
        header.addView(title);
        header.addView(closeButton);

        getContentView().addView(header);
    }

    private int getActionBarSize() {
        TypedValue value = new TypedValue();
        if (getTheme().resolveAttribute(android.R.attr.actionBarSize, value, true)) {
            return TypedValue.complexToDimensionPixelSize(
                    value.data, getResources().getDisplayMetrics());
        }

        return dpToPx(this, 56); // the standard action bar height on phones in portrait
    }

    public static void show(Activity activity) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("brave://account"));
        intent.setClassName(activity, BraveAccountCustomTabActivity.class.getName());
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, activity.getPackageName());
        intent.putExtra(
                CustomTabsIntent.EXTRA_COLOR_SCHEME,
                ColorUtils.inNightMode(activity) ? COLOR_SCHEME_DARK : COLOR_SCHEME_LIGHT);
        intent.putExtra(IntentHandler.EXTRA_PAGE_TRANSITION_TYPE, PageTransition.AUTO_TOPLEVEL);
        IntentUtils.addTrustedIntentExtras(intent);
        activity.startActivity(intent);
    }
}
