/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.graphics.Rect;
import android.graphics.Region;
import android.os.Build;
import android.util.ArrayMap;
import android.util.Pair;
import android.view.MotionEvent;
import android.view.TouchDelegate;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo.TouchDelegateInfo;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * A class holding multiple TouchDelegates to ensure touch events are dispatched correctly from
 * parent to all children.
 */
public class TouchDelegateComposite extends TouchDelegate {
    private List<Pair<View, Pair<Rect, TouchDelegate>>> mDelegates = new ArrayList<>(4);

    public TouchDelegateComposite(Rect bounds, View view) {
        super(bounds, view);
    }

    /**
     * Add a pre-existing TouchDelegate. Cannot get bounds and view from existing delegate.
     */
    public void addDelegate(TouchDelegate delegate) {
        mDelegates.add(new Pair<>(null, new Pair<>(new Rect(), delegate)));
    }

    /** Add a delegate by bounds and view. */
    @SuppressWarnings("NoStreams")
    public void addDelegate(Rect bounds, View view) {
        mDelegates =
                mDelegates.stream()
                        .filter(e -> e.first == null || e.first.getId() != view.getId())
                        .collect(Collectors.toList());

        mDelegates.add(new Pair<>(view, new Pair<>(bounds, new TouchDelegate(bounds, view))));
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean handled = false;
        float x = event.getX();
        float y = event.getY();
        for (Pair<View, Pair<Rect, TouchDelegate>> delegate : mDelegates) {
            event.setLocation(x, y);
            handled = delegate.second.second.onTouchEvent(event) || handled;
        }
        return handled;
    }

    @Override
    public TouchDelegateInfo getTouchDelegateInfo() {
        final ArrayMap<Region, View> targetMap = new ArrayMap<>();
        for (Pair<View, Pair<Rect, TouchDelegate>> delegate : mDelegates) {
            if (delegate.first == null) continue;
            Rect bounds = delegate.second.first;
            if (bounds == null) {
                bounds = new Rect();
            }
            targetMap.put(new Region(bounds), delegate.first);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return new TouchDelegateInfo(targetMap);
        }
        return null;
    }
}
