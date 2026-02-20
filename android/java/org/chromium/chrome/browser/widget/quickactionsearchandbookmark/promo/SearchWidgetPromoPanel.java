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
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;

@NullMarked
public class SearchWidgetPromoPanel implements View.OnClickListener {
    @Nullable
    private PopupWindow mPopupWindow;

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btAddWidget) {
            BraveSearchWidgetUtils.requestPinAppWidget();
        } else if (view.getId() == R.id.tvNotNow) {
            BraveSearchWidgetUtils.setShouldShowWidgetPromo(false);
        }
        if (mPopupWindow != null) {
            mPopupWindow.dismiss();
        }
    }

    public void showIfNeeded(final View parentView, final int extraBottomOffset, final Context context) {
        dismiss();
        if (BraveSearchWidgetUtils.getShouldShowWidgetPromo(context)) {
            View view = View.inflate(context, R.layout.layout_search_widget_promo, null);
            view.findViewById(R.id.btAddWidget).setOnClickListener(this);
            view.findViewById(R.id.tvNotNow).setOnClickListener(this);
            int width = LinearLayout.LayoutParams.MATCH_PARENT;
            int height = LinearLayout.LayoutParams.WRAP_CONTENT;
            mPopupWindow = new PopupWindow(view, width, height, true);
            int navigationBarBottomInset = 0;
            WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(parentView);
            if (insets != null) {
                navigationBarBottomInset =
                        insets.getInsets(WindowInsetsCompat.Type.tappableElement()).bottom;
            }
            int totalBottomOffset = extraBottomOffset + navigationBarBottomInset;
            mPopupWindow.showAtLocation(
                    parentView,
                    Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL,
                    0,
                    totalBottomOffset);
        }
    }

    /**
     * Returns whether the promo popup is currently visible on screen.
     */
    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    /**
     * Dismisses the promo popup if it is currently shown and clears its reference.
     * This method is safe to call multiple times. If the popup is already dismissed, it will be a
     * no-op.
     */
    public void dismiss() {
        if (mPopupWindow != null) {
            mPopupWindow.dismiss();
            mPopupWindow = null;
        }
    }

}
