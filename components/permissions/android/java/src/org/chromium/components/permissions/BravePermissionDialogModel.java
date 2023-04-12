/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modelutil.PropertyModel;

/* Adds additional items to Permission Dialog. */
class BravePermissionDialogModel {
    public static PropertyModel getModel(ModalDialogProperties.Controller controller,
            PermissionDialogDelegate delegate, Runnable touchFilteredCallback) {
        PropertyModel model =
                PermissionDialogModel.getModel(controller, delegate, touchFilteredCallback);
        View customView = (View) model.get(ModalDialogProperties.CUSTOM_VIEW);
        addLifetimeOptions(customView, delegate);
        return model;
    }

    /* Adds a permission lifetime options to a dialog view if lifetime options are available. */
    private static void addLifetimeOptions(View customView, PermissionDialogDelegate delegate) {
        Context context = delegate.getWindow().getContext().get();
        BravePermissionDialogDelegate braveDelegate =
                (BravePermissionDialogDelegate) (Object) delegate;

        String[] lifetimeOptions = braveDelegate.getLifetimeOptions();
        if (lifetimeOptions == null) {
            return;
        }

        LinearLayout layout = (LinearLayout) customView;

        // Create a text label before the lifetime selector.
        TextView lifetimeOptionsText = new TextView(context);
        lifetimeOptionsText.setText(braveDelegate.getLifetimeOptionsText());
        ApiCompatibilityUtils.setTextAppearance(
                lifetimeOptionsText, R.style.TextAppearance_TextMedium_Primary);

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
                braveDelegate.setSelectedLifetimeOption(checkedId);
            }
        });
        radioGroup.check(0);
        layout.addView(radioGroup);
    }
}
