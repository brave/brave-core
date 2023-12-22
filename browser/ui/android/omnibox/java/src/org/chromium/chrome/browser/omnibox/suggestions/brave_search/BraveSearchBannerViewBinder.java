/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions.brave_search;

import android.content.res.Configuration;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.chrome.browser.omnibox.R;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveSearchBannerViewBinder {
    public static void bind(PropertyModel model, View contentView, PropertyKey propertyKey) {
        int orientation = contentView.getContext().getResources().getConfiguration().orientation;
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            final TextView textViewDesc = contentView.findViewById(R.id.textview_desc);
            LinearLayout.LayoutParams textViewParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            textViewParams.topMargin = contentView.getResources().getDimensionPixelSize(
                    R.dimen.omnibox_brave_search_banner_text_desc_top_margin);
            textViewParams.rightMargin = contentView.getResources().getDimensionPixelSize(
                    R.dimen.omnibox_brave_search_banner_text_desc_right_margin);
            textViewDesc.setLayoutParams(textViewParams);
        }

        final Button btnPositive = contentView.findViewById(R.id.search_suggestions_action_btn);
        final ImageView btnNegative = contentView.findViewById(R.id.search_suggestions_close_btn);

        // boolean isMaybeLaterPressed =
        //         OmniboxPrefManager.getInstance().isBraveSearchPromoBannerMaybeLater();
        // if (isMaybeLaterPressed) {
        //     btnNegative.setText(
        //             contentView.getResources().getString(R.string.dismiss_search_promo));
        // } else {
        //     btnNegative.setText(contentView.getResources().getString(R.string.maybe_later));
        // }
        if (propertyKey == BraveSearchBannerProperties.DELEGATE) {
            BraveSearchBannerProperties.Delegate delegate =
                    model.get(BraveSearchBannerProperties.DELEGATE);
            if (delegate != null) {
                btnPositive.setOnClickListener(view -> { delegate.onPositiveClicked(); });

                btnNegative.setOnClickListener(
                        view -> {
                            // if (!isMaybeLaterPressed) {
                            //
                            // OmniboxPrefManager.getInstance().setBraveSearchPromoBannerMaybeLater();
                            // } else {
                            //
                            // OmniboxPrefManager.getInstance().setBraveSearchPromoBannerDismissed();
                            // }
                            delegate.onNegativeClicked();
                        });
            }
        }
    }
}
