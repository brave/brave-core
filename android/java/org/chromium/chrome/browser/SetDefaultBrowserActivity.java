/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;

public class SetDefaultBrowserActivity extends AsyncInitializationActivity {
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_set_default_browser);

        Button btnSetDefaultBrowser = findViewById(R.id.btn_set_default_browser);
        btnSetDefaultBrowser.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().handleBraveSetDefaultBrowserDialog();
                }
                finish();
            }
        });

        Button btnNotNow = findViewById(R.id.btn_not_now);
        btnNotNow.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                finish();
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        boolean isNightMode = GlobalNightModeStateProviderHolder.getInstance().isInNightMode();

        ImageView setDefaultBrowserImg = findViewById(R.id.set_default_browser_img);
        setDefaultBrowserImg.setImageResource(isNightMode ? R.drawable.ic_setbraveasdefault_dark
                                                          : R.drawable.ic_setbraveasdefault);
        if (isNightMode) {
            LinearLayout.LayoutParams lp =
                    (LinearLayout.LayoutParams) setDefaultBrowserImg.getLayoutParams();
            lp.setMargins(0, dpToPx(this, 16), 0, dpToPx(this, 16));
            setDefaultBrowserImg.setLayoutParams(lp);
        }
    }

    @Override
    public void onBackPressed() {}

    @Override
    public boolean shouldStartGpuProcess() {
        return false;
    }
}
