/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.timer;

public class TimerItemModel {
    private final String actionText;
    private final int actionImage;

    public TimerItemModel(String actionText, int actionImage) {
        this.actionText = actionText;
        this.actionImage = actionImage;
    }

    public String getActionText() {
        return actionText;
    }

    public int getActionImage() {
        return actionImage;
    }
}
