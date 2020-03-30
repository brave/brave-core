/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.ntp_background_images;

import android.os.Bundle;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.graphics.Bitmap;
import android.widget.ImageView;
import android.widget.Button;
import android.support.v4.app.DialogFragment;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.common.BitMatrix;

import org.chromium.chrome.R;
import org.chromium.base.Log;

public class SuperReferralShareDialogFragment extends DialogFragment implements View.OnClickListener{
	private static final String TAG = "SUPER-REFERRAL";

	// For QR code generation
	private static final int WHITE = 0xFFFFFFFF;
  	private static final int BLACK = 0xFF000000;
  	private static final int WIDTH = 300;

	private ImageView mQRImage;
	private Button mShareButton;

	public SuperReferralShareDialogFragment() {
		// Empty constructor is required for DialogFragment
		// Make sure not to add arguments to the constructor
		// Use `newInstance` instead as shown below
	}
	
	public static SuperReferralShareDialogFragment newInstance(String title) {
		SuperReferralShareDialogFragment mSuperReferralShareDialogFragment = new SuperReferralShareDialogFragment();
		// Bundle args = new Bundle();
		// args.putString("title", title);
		// frag.setArguments(args);
		return mSuperReferralShareDialogFragment;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		return inflater.inflate(R.layout.fragment_super_referral_share, container);
	}

	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);
		mQRImage = view.findViewById(R.id.share_qr_code_image);
		mShareButton = view.findViewById(R.id.btn_share);

		mShareButton.setOnClickListener(this);

		generateQRCode();
	}

	@Override
    public void onClick(View view) {
    	if (view.getId() == R.id.btn_share) {
    		openShareIntent();
    		dismiss();
    	}
    }

    private void openShareIntent() {
    	Intent sharingIntent = new Intent(android.content.Intent.ACTION_SEND);
        sharingIntent.setType("text/plain");
        sharingIntent.putExtra(android.content.Intent.EXTRA_SUBJECT, "My App Name");
        sharingIntent.putExtra(android.content.Intent.EXTRA_TEXT, "Brave app");
        getActivity().startActivity(Intent.createChooser(sharingIntent, "Share app via"));
    }

    private void generateQRCode() {
        final String qrDataFinal = "https://brave.com/?ref=TECHNIK";
        new Thread(new Runnable() {
            @Override
            public void run() {
                // Generate QR code
                BitMatrix result;
                try {
                    result = new MultiFormatWriter().encode(qrDataFinal, BarcodeFormat.QR_CODE, WIDTH, WIDTH, null);
                } catch (WriterException e) {
                    Log.e(TAG, "QR code unsupported format: " + e);
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
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mQRImage.setImageBitmap(bitmap);
                        mQRImage.invalidate();
                    }
                });
            }
        }).start();
    }
}