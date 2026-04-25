/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.brave_leo;

import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.suggestions.basic.SuggestionViewProperties;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveLeoSuggestionViewBinder {
    public static void bind(PropertyModel model, View contentView, PropertyKey propertyKey) {
        if (propertyKey == SuggestionViewProperties.TEXT_LINE_1_TEXT) {
            TextView tv = contentView.findViewById(R.id.line_1);
            tv.setText(model.get(SuggestionViewProperties.TEXT_LINE_1_TEXT));
        } else if (propertyKey == SuggestionViewProperties.TEXT_LINE_2_TEXT) {
            TextView tv = contentView.findViewById(R.id.line_2);
            tv.setVisibility(View.VISIBLE);
            tv.setText(model.get(SuggestionViewProperties.TEXT_LINE_2_TEXT));
        }
    }
}
