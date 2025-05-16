/*
  Copyright (c) 2022 The Brave Authors. All rights reserved.
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark.promo;

import android.content.Context;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import androidx.annotation.NonNull;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;

public class SearchWidgetPromoPanel implements View.OnClickListener {
    private final PopupWindow mPopupWindow;
    private final Context mContext;

    public SearchWidgetPromoPanel(@NonNull Context context) {
        mContext = context;
        View view = View.inflate(context, R.layout.layout_search_widget_promo, null);
        view.findViewById(R.id.btAddWidget).setOnClickListener(this);
        view.findViewById(R.id.tvNotNow).setOnClickListener(this);
        int width = LinearLayout.LayoutParams.MATCH_PARENT;
        int height = LinearLayout.LayoutParams.WRAP_CONTENT;
        mPopupWindow = new PopupWindow(view, width, height, true);
    }

    @Override
    public void onClick(@NonNull View view) {
        if (view.getId() == R.id.btAddWidget)
            BraveSearchWidgetUtils.requestPinAppWidget();
        else if (view.getId() == R.id.tvNotNow)
            BraveSearchWidgetUtils.setShouldShowWidgetPromo(false);
        mPopupWindow.dismiss();
    }

    public void showIfNeeded(@NonNull View anchorView) {
        if (BraveSearchWidgetUtils.getShouldShowWidgetPromo(mContext))
            mPopupWindow.showAsDropDown(anchorView);
    }
}
