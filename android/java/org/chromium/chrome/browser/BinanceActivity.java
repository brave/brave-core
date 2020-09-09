/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.os.Bundle;

import android.net.Uri;
import android.text.TextUtils;

import org.chromium.base.Log;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.embedder_support.util.UrlConstants;

import java.util.List;

public class BinanceActivity extends AppCompatActivity {

    private static final String REDIRECT_URI_ROOT = "com.brave.binance";
    private static final String CODE = "code";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Uri data = getIntent().getData();
        if (data != null && !TextUtils.isEmpty(data.getScheme())) {
            if (REDIRECT_URI_ROOT.equals(data.getScheme())) {
                Log.e("NTP", "scheme : " + data.getScheme());
                String host = data.getHost();
                Log.e("NTP", "host : " + data.getHost());
                List<String> params = data.getPathSegments();
                // Set<String> args = data.getQueryParameterNames();
                for (String arg : params) {
                    Log.e("NTP", "Path segments : " + arg);
                }
                String code = data.getQueryParameter(CODE);
                if (!TextUtils.isEmpty(code)) {
                    Log.e("NTP", "Code : " + code);
                    TabUtils.openUrlInSameTab(UrlConstants.NTP_URL);
                    finish();
                }
            }
        }
    }
}