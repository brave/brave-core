/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import org.chromium.ui.base.ViewUtils;

/* Helpers for permission dialog. */
public class BravePermissionDialogModelUtils {
    /* Adds a permission lifetime options to a dialog view if lifetime options are available. */
    public static void addLifetimeOptions(
            Context context, View customView, PermissionDialogDelegate delegate) {
        String[] lifetimeOptions = delegate.getLifetimeOptions();
        if (lifetimeOptions == null) {
            return;
        }

        LinearLayout layout = (LinearLayout) customView;

        // Create a text label before the lifetime selector.
        TextView lifetimeOptionsText = new TextView(context);
        lifetimeOptionsText.setText(delegate.getLifetimeOptionsText());
        lifetimeOptionsText.setTextAppearance(R.style.TextAppearance_TextMedium_Primary);

        LinearLayout.LayoutParams lifetimeOptionsTextLayoutParams =
                new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lifetimeOptionsTextLayoutParams.setMargins(0, 0, 0, ViewUtils.dpToPx(context, 8));
        lifetimeOptionsText.setLayoutParams(lifetimeOptionsTextLayoutParams);
        layout.addView(lifetimeOptionsText);

        // Create radio buttons with lifetime options.
        RadioGroup radioGroup = new RadioGroup(context);
        int radioButtonIndex = 0;
        for (String lifetimeOption : lifetimeOptions) {
            RadioButton radioButon = new RadioButton(context);
            radioButon.setText(lifetimeOption);
            radioButon.setId(radioButtonIndex);
            radioButtonIndex += 1;
            radioGroup.addView(radioButon);
        }
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                delegate.setSelectedLifetimeOption(checkedId);
            }
        });
        radioGroup.check(0);
        layout.addView(radioGroup);
    }
}
