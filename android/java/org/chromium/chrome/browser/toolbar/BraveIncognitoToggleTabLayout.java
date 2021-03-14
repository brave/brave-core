/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.toolbar;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;

import org.chromium.chrome.R;

/**
 * TabLayout shown in the Horizontal Tab Switcher.
 */
public class BraveIncognitoToggleTabLayout extends IncognitoToggleTabLayout {
    // To delete in bytecode, members from parent class will be used instead.
    private ImageView mIncognitoButtonIcon;

    public BraveIncognitoToggleTabLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        mIncognitoButtonIcon.setImageResource(R.drawable.brave_menu_new_private_tab);
    }
}
