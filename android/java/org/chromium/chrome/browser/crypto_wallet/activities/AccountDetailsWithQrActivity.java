/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.Toolbar;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

public class AccountDetailsWithQrActivity extends BraveWalletBaseActivity {
    private static final int WIDTH = 300;

    private ImageView qrCodeImage;
    private AccountInfo mAccountInfo;

    public static Intent createIntent(@NonNull Context context, @NonNull AccountInfo accountInfo) {
        Intent intent = new Intent(context, AccountDetailsWithQrActivity.class);
        WalletUtils.addAccountInfoToIntent(intent, accountInfo);
        return intent;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_account_details_with_qr);

        if (getIntent() != null) {
            mAccountInfo = WalletUtils.getAccountInfoFromIntent(getIntent());
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.account_details));

        qrCodeImage = findViewById(R.id.qr_code_image);
        fillQrCode(mAccountInfo.address);

        TextView accountValueText = findViewById(R.id.account_value_text);
        accountValueText.setText(Utils.stripAccountAddress(mAccountInfo.address));

        ImageView accountCopyImage = findViewById(R.id.account_copy_image);
        accountCopyImage.setOnClickListener(v
                -> Utils.saveTextToClipboard(AccountDetailsWithQrActivity.this,
                        mAccountInfo.address, R.string.address_has_been_copied, false));

        EditText accountNameText = findViewById(R.id.account_name_text);
        accountNameText.setText(mAccountInfo.name);
        accountNameText.setEnabled(false);

        TextView privateKeyText = findViewById(R.id.account_private_key_text);
        privateKeyText.setOnClickListener(v -> {
            Intent intent = AccountPrivateKeyActivity.createIntent(
                    AccountDetailsWithQrActivity.this, mAccountInfo);
            startActivity(intent);
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
