/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;

import java.util.ArrayList;
import java.util.List;

public class AccountDetailsWithQrActivity extends AsyncInitializationActivity {
    private static final int WIDTH = 300;

    private ImageView qrCodeImage;

    private String address;
    private String name;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_details_with_qr);

        if (getIntent() != null) {
            address = getIntent().getStringExtra("address");
            name = getIntent().getStringExtra("name");
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.account_details));

        qrCodeImage = findViewById(R.id.qr_code_image);
        fillQrCode(address);

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText(address);

        ImageView accountCopyImage = findViewById(R.id.account_copy_image);
        accountCopyImage.setOnClickListener(v
                -> Utils.saveTextToClipboard(
                        AccountDetailsWithQrActivity.this, accountValueText.getText().toString()));

        EditText accountNameText = findViewById(R.id.account_name_text);
        accountNameText.setText(name);
        accountNameText.setEnabled(false);

        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    private void fillQrCode(String qrData) {
        final int WHITE = 0xFFFFFFFF;
        final int BLACK = 0xFF000000;
        new Thread(new Runnable() {
            @Override
            public void run() {
                // Generate QR code
                BitMatrix result;
                try {
                    result = new MultiFormatWriter().encode(
                            qrData, BarcodeFormat.QR_CODE, WIDTH, WIDTH, null);
                } catch (WriterException e) {
                    Log.e("AccountDetailsWithQrActivity", "QR code unsupported format: " + e);
                    return;
                }
                int w = result.getWidth();
                int h = result.getHeight();
                int[] pixels = new int[w * h];
                for (int y = 0; y < h; y++) {
                    int offset = y * w;
                    for (int x = 0; x < w; x++) {
                        pixels[offset + x] = result.get(x, y) ? BLACK : WHITE;
                    }
                }
                Bitmap bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
                bitmap.setPixels(pixels, 0, WIDTH, 0, 0, w, h);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        qrCodeImage.setImageBitmap(bitmap);
                        qrCodeImage.invalidate();
                    }
                });
            }
        }).start();
    }
}
