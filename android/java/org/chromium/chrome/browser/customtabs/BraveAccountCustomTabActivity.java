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
import android.graphics.Color;
import android.net.Uri;
import android.provider.Browser;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.Window;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.CallSuper;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.components.browser_ui.styles.SemanticColorUtils;
import org.chromium.ui.util.ColorUtils;

public class BraveAccountCustomTabActivity extends CustomTabActivity {
    private static final int HEADER_HEIGHT = 64;
    private static final int CLOSE_BUTTON_RIGHT_MARGIN = 16;
    private static final int CLOSE_BUTTON_TOP_BOTTOM_MARGIN = 20;
    private static final int CLOSE_BUTTON_SIZE = 24;
    private static final int TITLE_MARGIN = 16;

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();
        WindowCompat.enableEdgeToEdge(getWindow());
    }

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        // Hide the default toolbar
        View toolbarContainer = findViewById(R.id.toolbar_container);
        if (toolbarContainer != null) {
            toolbarContainer.setVisibility(View.GONE);
        }

        // Header
        FrameLayout header = new FrameLayout(this);
        FrameLayout.LayoutParams headerParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        headerParams.gravity = Gravity.TOP;
        header.setLayoutParams(headerParams);
        header.setBackgroundColor(SemanticColorUtils.getColorSurfaceContainer(this));
        ViewCompat.setOnApplyWindowInsetsListener(header, (v, windowInsets) -> {
            Insets insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(v.getPaddingLeft(), insets.top, v.getPaddingRight(), v.getPaddingBottom());
            return WindowInsetsCompat.CONSUMED;
        });

        // Title
        TextView titleText = new TextView(this);
        titleText.setText(R.string.prefs_section_brave_account);
        titleText.setTextSize(18);
        titleText.setTextColor(ContextCompat.getColor(this, R.color.schemes_on_surface_variant));
        titleText.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams titleParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        titleParams.gravity = Gravity.CENTER;
        int titleMarginPx = dpToPx(TITLE_MARGIN);
        titleParams.setMargins(titleMarginPx, 0, titleMarginPx, 0);
        titleText.setLayoutParams(titleParams);

        // Close button
        ImageView closeImg = new ImageView(this);
        closeImg.setImageResource(R.drawable.ic_close);
        closeImg.setColorFilter(ContextCompat.getColor(this, R.color.schemes_on_surface_variant),
                android.graphics.PorterDuff.Mode.SRC_IN);
        closeImg.setOnClickListener(button -> finish());

        FrameLayout.LayoutParams closeParams =
                new FrameLayout.LayoutParams(dpToPx(CLOSE_BUTTON_SIZE), dpToPx(CLOSE_BUTTON_SIZE));
        closeParams.gravity = Gravity.CENTER_VERTICAL | Gravity.END;
        closeParams.setMargins(0, dpToPx(CLOSE_BUTTON_TOP_BOTTOM_MARGIN),
                dpToPx(CLOSE_BUTTON_RIGHT_MARGIN), dpToPx(CLOSE_BUTTON_TOP_BOTTOM_MARGIN));
        closeImg.setLayoutParams(closeParams);

        header.addView(titleText);
        header.addView(closeImg);
        ViewGroup content = findViewById(android.R.id.content);
        content.addView(header);
    }

    private int dpToPx(int dp) {
        return Math.round(TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics()));
    }

    public static void showPage(Context context, String url) {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setClassName(context, BraveAccountCustomTabActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.putExtra(CustomTabsIntent.EXTRA_TITLE_VISIBILITY_STATE, CustomTabsIntent.NO_TITLE);
        intent.putExtra(CustomTabsIntent.EXTRA_ENABLE_URLBAR_HIDING, false);
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
}
