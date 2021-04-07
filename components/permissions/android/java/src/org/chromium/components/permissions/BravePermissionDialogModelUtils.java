/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import android.content.Context;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;

/* Helpers for permission dialog. */
public class BravePermissionDialogModelUtils {
    /* Adds a permission lifetime spinner to a dialog view if lifetime options are available. */
    public static void addLifetimeSpinner(
            Context context, View customView, PermissionDialogDelegate delegate) {
        String[] lifetimeOptions = delegate.getLifetimeOptions();
        if (lifetimeOptions == null) {
            return;
        }

        // Create spinner view.
        Spinner spinner = new Spinner(context);

        // Set lifetime options as visible items to the spinner.
        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(
                context, android.R.layout.simple_spinner_item, lifetimeOptions);
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(dataAdapter);

        // Add select handler to store the selected lifetime option index in the delegate.
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(
                    AdapterView<?> parentView, View selectedItemView, int position, long id) {
                delegate.setSelectedLifetimeOption(position);
            }
            @Override
            public void onNothingSelected(AdapterView<?> parentView) {}
        });
        spinner.setSelection(0);

        LinearLayout layout = (LinearLayout) customView;
        layout.addView(spinner);
    }
}
