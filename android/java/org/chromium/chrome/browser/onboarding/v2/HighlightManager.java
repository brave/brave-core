/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.view.View;

import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.base.Log;

import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment;
import org.chromium.chrome.browser.onboarding.v2.HighlightItem;

public class HighlightManager {

    final private static String TAG_FRAGMENT = "HIGHLIGHT_FRAG";

    private FragmentActivity activity;

    public HighlightManager(FragmentActivity activity) {
        this.activity = activity;
    }

    public void showHighlight(int id, int position )  {

        Log.e("NTP", "showHighlight : "+position);

        final HighlightItem item = new HighlightItem();
        View view = activity.findViewById(id);
        view.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                       int oldLeft, int oldTop, int oldRight, int oldBottom) {
                if (left == 0 && top == 0 && right == 0 && bottom == 0) {
                    return;
                }
                v.removeOnLayoutChangeListener(this);
                HighlightItem tempItem = setScreenPosition(v, item);
                show(tempItem);
            }
        });
        if (position >= 0) {
            HighlightItem tempItem = setScreenPosition(view, item);
            show(tempItem);
        }
    }

    private HighlightItem setScreenPosition(View v, HighlightItem item) {
        int[] location = new int[2];
        v.getLocationOnScreen(location);
        int screenLeft = location[0];
        int screenTop = location[1];
        int screenRight = location[0] + v.getMeasuredWidth();
        int screenBottom = location[1] + v.getMeasuredHeight();
        item.setScreenPosition(screenLeft, screenTop, screenRight, screenBottom);
        return item;
    }

    private void show(HighlightItem item) {
        Log.e("NTP", "show");
        FragmentManager fm = activity.getSupportFragmentManager();
        HighlightDialogFragment fragment = (HighlightDialogFragment) fm
                                           .findFragmentByTag(TAG_FRAGMENT);

        FragmentTransaction transaction = fm.beginTransaction();

        if (fragment != null) {
            Log.e("NTP", "fragment != null");
            fragment.setHighlightItem(item);
        } else {
            Log.e("NTP", "fragment == null");
            fragment = new HighlightDialogFragment();
            fragment.setManager(this);
            fragment.setHighlightItem(item);
            transaction.add(fragment, TAG_FRAGMENT);
            transaction.commit();
        }
    }
}