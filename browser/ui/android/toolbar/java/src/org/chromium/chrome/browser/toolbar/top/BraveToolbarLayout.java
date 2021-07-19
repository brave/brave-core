/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.animation.Animator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

public interface BraveToolbarLayout {
    public void onClickImpl(View v);

    public boolean onLongClickImpl(View v);

    public void updateModernLocationBarColorImpl(int color);

    public int getBoundsAfterAccountingForRightButtonsImpl(ViewGroup toolbarButtonsContainer);

    public void populateUrlAnimatorSetImpl(boolean showExpandedState,
            int urlFocusToolbarButtonsDuration, int urlClearFocusTabStackDelayMs,
            List<Animator> animators);
}
