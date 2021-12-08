/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.crypto.binance;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.core.content.res.ResourcesCompat;

import org.chromium.chrome.R;

import java.util.List;

public class BinanceSpinnerAdapter extends ArrayAdapter<CoinNetworkModel> {
    private List<CoinNetworkModel> items;
    private boolean shouldShowIcon;

    public BinanceSpinnerAdapter(
            Context context, List<CoinNetworkModel> items, boolean shouldShowIcon) {
        super(context, R.layout.binance_spinner_item, items);
        this.items = items;
        this.shouldShowIcon = shouldShowIcon;
    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater =
                    (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.binance_spinner_dropdown_item, parent, false);
        }

        TextView currencyText = view.findViewById(R.id.currency_text);
        currencyText.setText(items.get(position).getCoin());

        ImageView currencyImage = view.findViewById(R.id.currency_image);
        if (shouldShowIcon && items.get(position).getCoinRes() != 0) {
            currencyImage.setVisibility(View.VISIBLE);
            currencyImage.setImageResource(items.get(position).getCoinRes());
        } else if (!shouldShowIcon) {
            currencyImage.setVisibility(View.GONE);
        } else {
            currencyImage.setVisibility(View.INVISIBLE);
        }
        return view;
    }

    @Override
    public CoinNetworkModel getItem(int position) {
        return items.get(position);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        TextView textView = (TextView) super.getView(position, convertView, parent);

        if (textView == null) {
            textView = new TextView(getContext());
        }
        textView.setText(items.get(position).getCoin());

        Drawable coinDrawable;
        if (shouldShowIcon && items.get(position).getCoinRes() != 0) {
            Drawable tempCoinDrawable = ResourcesCompat.getDrawable(getContext().getResources(),
                    items.get(position).getCoinRes(), /* theme= */ null);
            Bitmap bitmap = Bitmap.createBitmap(tempCoinDrawable.getIntrinsicWidth(),
                    tempCoinDrawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            tempCoinDrawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
            tempCoinDrawable.draw(canvas);
            coinDrawable = new BitmapDrawable(getContext().getResources(),
                    Bitmap.createScaledBitmap(
                            bitmap, dpToPx(getContext(), 24), dpToPx(getContext(), 24), true));
        } else {
            coinDrawable = null;
        }

        textView.setCompoundDrawablesWithIntrinsicBounds(coinDrawable, null,
                ResourcesCompat.getDrawable(getContext().getResources(),
                        R.drawable.ic_arrow_drop_down_white, /* theme= */ null),
                null);
        return textView;
    }
}
