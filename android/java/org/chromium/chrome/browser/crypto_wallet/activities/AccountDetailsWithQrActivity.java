/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;
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
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;

public class AccountDetailsWithQrActivity extends BraveWalletBaseActivity {
    private static final int WIDTH = 300;

    private ImageView qrCodeImage;

    private String mAddress;
    private String mName;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_details_with_qr);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.account_details));

        qrCodeImage = findViewById(R.id.qr_code_image);
        fillQrCode(mAddress);

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText(Utils.stripAccountAddress(mAddress));

        ImageView accountCopyImage = findViewById(R.id.account_copy_image);
        accountCopyImage.setOnClickListener(v
                -> Utils.saveTextToClipboard(AccountDetailsWithQrActivity.this, mAddress,
                        R.string.address_has_been_copied, false));

        EditText accountNameText = findViewById(R.id.account_name_text);
        accountNameText.setText(mName);
        accountNameText.setEnabled(false);

        TextView privateKeyText = findViewById(R.id.account_private_key_text);
        privateKeyText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent accountPrivateKeyActivityIntent = new Intent(
                        AccountDetailsWithQrActivity.this, AccountPrivateKeyActivity.class);
                accountPrivateKeyActivityIntent.putExtra(Utils.ADDRESS, mAddress);
                startActivity(accountPrivateKeyActivityIntent);
            }
        });

        onInitialLayoutInflationComplete();
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
