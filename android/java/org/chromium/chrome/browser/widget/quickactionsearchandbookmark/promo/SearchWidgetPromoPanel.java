/*
  Copyright (c) 2022 The Brave Authors. All rights reserved.
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark.promo;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;

@NullMarked
public class SearchWidgetPromoPanel implements View.OnClickListener {
    private final PopupWindow mPopupWindow;
    private final Context mContext;

    public SearchWidgetPromoPanel(Context context) {
        mContext = context;
        View view = View.inflate(context, R.layout.layout_search_widget_promo, null);
        view.findViewById(R.id.btAddWidget).setOnClickListener(this);
        view.findViewById(R.id.tvNotNow).setOnClickListener(this);
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        mPopupWindow = new PopupWindow(view, width, height, true);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btAddWidget) {
            BraveSearchWidgetUtils.requestPinAppWidget();
        } else if (view.getId() == R.id.tvNotNow) {
            BraveSearchWidgetUtils.setShouldShowWidgetPromo(false);
        }
        mPopupWindow.dismiss();
    }

    public void showIfNeeded(final View parentView, int extraBottomOffset) {
        if (BraveSearchWidgetUtils.getShouldShowWidgetPromo(mContext)) {
            if (extraBottomOffset == 0) {
                WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(parentView);
                if (insets != null) {
                    extraBottomOffset = insets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom;
                }
            }
            if (mPopupWindow.isShowing()) {
                mPopupWindow.update(0, extraBottomOffset, -1, -1);
            } else {
                mPopupWindow.showAtLocation(
                        parentView,
                        Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL,
                        0,
                        extraBottomOffset);
            }
        }
    }
}
