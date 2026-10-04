/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.widget;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public class BraveRadioButtonWithDescriptionRight extends RadioButtonWithDescription {
    public BraveRadioButtonWithDescriptionRight(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected int getLayoutResource() {
        return R.layout.brave_radio_button_with_description_right;
    }
}
