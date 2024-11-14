/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.utils;





public class QuickSearchEnginesViewsUtil {

    // private static View mQuickSearchEnginesView;

    // public void showQuickActionSearchEnginesView(Activity activity, int keypadHeight) {
    //     mQuickSearchEnginesView =
    //             activity.getLayoutInflater().inflate(R.layout.quick_serach_engines_view, null);
    //     RecyclerView recyclerView =
    //             (RecyclerView)
    //                     mQuickSearchEnginesView.findViewById(
    //                             R.id.quick_search_engines_recyclerview);
    //     LinearLayoutManager linearLayoutManager =
    //             new LinearLayoutManager(activity, LinearLayoutManager.HORIZONTAL, false);
    //     recyclerView.setLayoutManager(linearLayoutManager);

    //     ImageView quickSearchEnginesSettings =
    //             (ImageView)
    //                     mQuickSearchEnginesView.findViewById(R.id.quick_search_engines_settings);
    //     quickSearchEnginesSettings.setOnClickListener(
    //             new View.OnClickListener() {
    //                 @Override
    //                 public void onClick(View v) {
    //                     // openQuickSearchEnginesSettings();
    //                 }
    //             });

    //     // List<QuickSearchEnginesModel> searchEngines =
    //     //         QuickSearchEnginesUtil.getQuickSearchEnginesForView(getCurrentProfile());
    //     // QuickSearchEnginesModel leoQuickSearchEnginesModel =
    //     //         new QuickSearchEnginesModel("", "", "", true);
    //     // searchEngines.add(0, leoQuickSearchEnginesModel);

    //     // QuickSearchEnginesViewAdapter adapter =
    //     //         new QuickSearchEnginesViewAdapter(BraveActivity.this, searchEngines, this);
    //     // recyclerView.setAdapter(adapter);
    //     if (mQuickSearchEnginesView.getParent() == null) {
    //         WindowManager.LayoutParams params =
    //                 new WindowManager.LayoutParams(
    //                         WindowManager.LayoutParams.MATCH_PARENT,
    //                         WindowManager.LayoutParams.WRAP_CONTENT,
    //                         WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
    //                         WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
    //                         WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
    //         params.gravity = Gravity.BOTTOM;
    //         params.y = keypadHeight; // Position the view above the keyboard

    //         WindowManager windowManager = (WindowManager)
    // activity.getSystemService(WINDOW_SERVICE);
    //         windowManager.addView(mQuickSearchEnginesView, params);
    //     }
    // }

    // public void removeQuickActionSearchEnginesView(Activity activity) {
    //     // if (mKeyboardVisibilityHelper != null) {
    //     //     mKeyboardVisibilityHelper.removeListener();
    //     // }

    //     if (mQuickSearchEnginesView != null && mQuickSearchEnginesView.getParent() != null) {
    //         Log.e("onUrlFocusChange", "removeQuickActionSearchEnginesView");
    //         WindowManager windowManager = (WindowManager)
    // activity.getSystemService(WINDOW_SERVICE);
    //         windowManager.removeView(mQuickSearchEnginesView);
    //     }
    // }
}
