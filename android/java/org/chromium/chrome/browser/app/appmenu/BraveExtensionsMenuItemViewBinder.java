/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.appmenu;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.MeasureSpec;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.Px;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.appmenu.AppMenuClickHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModel.WritableObjectPropertyKey;

/** Binds the phone app menu Extensions item: icon, title and an optional subtitle. */
@NullMarked
public class BraveExtensionsMenuItemViewBinder {
    public static final WritableObjectPropertyKey<CharSequence> SUBTITLE =
            new WritableObjectPropertyKey<>("SUBTITLE");

    public static final PropertyKey[] ALL_KEYS =
            PropertyModel.concatKeys(AppMenuItemProperties.ALL_KEYS, new PropertyKey[] {SUBTITLE});

    public static void bind(PropertyModel model, View view, PropertyKey key) {
        if (key == AppMenuItemProperties.MENU_ITEM_ID) {
            view.setId(model.get(AppMenuItemProperties.MENU_ITEM_ID));
        } else if (key == AppMenuItemProperties.TITLE) {
            ((TextView) view.findViewById(R.id.menu_item_text))
                    .setText(model.get(AppMenuItemProperties.TITLE));
        } else if (key == AppMenuItemProperties.ICON) {
            ImageView iconView = view.findViewById(R.id.menu_item_icon);
            iconView.setImageDrawable(model.get(AppMenuItemProperties.ICON));
            iconView.setVisibility(
                    model.get(AppMenuItemProperties.ICON) != null ? View.VISIBLE : View.GONE);
        } else if (key == SUBTITLE) {
            CharSequence subtitle = model.get(SUBTITLE);
            TextView subtitleView = view.findViewById(R.id.menu_item_subtitle);
            subtitleView.setText(subtitle);
            subtitleView.setVisibility(TextUtils.isEmpty(subtitle) ? View.GONE : View.VISIBLE);
        } else if (key == AppMenuItemProperties.CLICK_HANDLER
                || key == AppMenuItemProperties.ENABLED) {
            AppMenuClickHandler clickHandler = model.get(AppMenuItemProperties.CLICK_HANDLER);
            boolean enabled = model.get(AppMenuItemProperties.ENABLED);
            view.setOnClickListener(
                    enabled && clickHandler != null ? v -> clickHandler.onItemClick(model) : null);
            view.setClickable(enabled);
        }
    }

    /**
     * Estimates the item height, only used to size the menu before it is laid out. Measured at the
     * narrowest menu width, so the subtitle wraps as much as it can.
     */
    public static @Px int getPixelHeight(Context context, PropertyModel model) {
        View view = LayoutInflater.from(context).inflate(R.layout.brave_extensions_menu_item, null);
        for (PropertyKey key : model.getAllSetProperties()) {
            bind(model, view, key);
        }
        view.measure(
                MeasureSpec.makeMeasureSpec(
                        context.getResources().getDimensionPixelSize(R.dimen.menu_width_min),
                        MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        return view.getMeasuredHeight();
    }
}
