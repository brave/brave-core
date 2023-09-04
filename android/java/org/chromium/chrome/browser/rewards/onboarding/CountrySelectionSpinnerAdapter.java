/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.onboarding;

import android.content.Context;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.browser.BraveRewardsNativeWorker;

import java.util.ArrayList;
import java.util.Locale;

public class CountrySelectionSpinnerAdapter extends ArrayAdapter<String> {
    public CountrySelectionSpinnerAdapter(Context context, ArrayList<String> countryList) {
        super(context, 0, countryList);
    }

    @NonNull
    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        if (convertView == null) {
            convertView = LayoutInflater.from(getContext())
                                  .inflate(android.R.layout.simple_spinner_item, parent, false);
        }
        return createView(position, convertView, parent, false);
    }

    @Override
    public View getDropDownView(
            int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        if (convertView == null) {
            convertView =
                    LayoutInflater.from(getContext())
                            .inflate(android.R.layout.simple_spinner_dropdown_item, parent, false);
        }
        return createView(position, convertView, parent, true);
    }

    private View createView(int position, View convertView, ViewGroup parent, boolean isDropdown) {
        TextView textViewName = convertView.findViewById(android.R.id.text1);
        String currentItem = getItem(position);
        if (isDropdown) {
            String defaultCountry = BraveRewardsNativeWorker.getInstance().getCountryCode() != null
                    ? new Locale("", BraveRewardsNativeWorker.getInstance().getCountryCode())
                              .getDisplayCountry()
                    : null;
            if (defaultCountry != null && currentItem.equals(defaultCountry)) {
                textViewName.setTypeface(textViewName.getTypeface(), Typeface.BOLD);
            }
        }

        if (currentItem != null) {
            textViewName.setText(currentItem);
        }
        return convertView;
    }
}
