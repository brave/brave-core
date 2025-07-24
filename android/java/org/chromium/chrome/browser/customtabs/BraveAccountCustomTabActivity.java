/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs;

import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_DARK;
import static androidx.browser.customtabs.CustomTabsIntent.COLOR_SCHEME_LIGHT;

import static org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider.EXTRA_UI_TYPE;

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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.ui.util.ColorUtils;

public class BraveAccountCustomTabActivity extends CustomTabActivity {
    private static final int CLOSE_BUTTON_MARGIN_DP = 16;
    private static final int HEADER_HEIGHT_DP = 64;
    private static final int TITLE_TEXT_SIZE_SP = 18;

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        // Hide the toolbar container
        View toolbarContainer = findViewById(R.id.toolbar_container);
        if (toolbarContainer != null) toolbarContainer.setVisibility(View.GONE);

        TextView title = new TextView(this);
        title.setText(R.string.prefs_section_brave_account);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, TITLE_TEXT_SIZE_SP);
        title.setTextColor(ContextCompat.getColor(this, R.color.schemes_on_surface_variant));
        title.setGravity(Gravity.CENTER);
        title.setLayoutParams(new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT, Gravity.CENTER));

        ImageView closeButton = new ImageView(this);
        closeButton.setImageResource(R.drawable.ic_close);
        closeButton.setColorFilter(ContextCompat.getColor(this, R.color.schemes_on_surface_variant),
                android.graphics.PorterDuff.Mode.SRC_IN);
        closeButton.setOnClickListener(button -> finish());
        FrameLayout.LayoutParams closeParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER_VERTICAL | Gravity.END);
        closeParams.setMarginEnd(dpToPx(CLOSE_BUTTON_MARGIN_DP));
        closeButton.setLayoutParams(closeParams);

        FrameLayout header = new FrameLayout(this);
        header.setLayoutParams(new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, dpToPx(HEADER_HEIGHT_DP), Gravity.TOP));
        header.setBackgroundColor(getBaseStatusBarColor(null));
        header.addView(title);
        header.addView(closeButton);

        getContentView().addView(header);
    }

    @Override
    public int getBaseStatusBarColor(Tab tab) {
        return SemanticColorUtils.getColorSurfaceContainer(this);
    }

    // Converts dp to actual pixels for the current screen density.
    private int dpToPx(int dp) {
        return Math.round(TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics()));
    }

    public static void show(Activity activity) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("brave://account"));
        intent.setClassName(activity, BraveAccountCustomTabActivity.class.getName());
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, activity.getPackageName());
        intent.putExtra(CustomTabsIntent.EXTRA_COLOR_SCHEME,
                ColorUtils.inNightMode(activity) ? COLOR_SCHEME_DARK : COLOR_SCHEME_LIGHT);
        IntentUtils.addTrustedIntentExtras(intent);
        activity.startActivity(intent);
    }
}
