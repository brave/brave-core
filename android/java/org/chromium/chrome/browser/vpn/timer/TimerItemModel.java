/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.timer;

public class TimerItemModel {
    public enum TimerDuration {
        MINUTES_15(15),
        MINUTES_60(60),
        NONE(0),
        CANCEL(0);

        private final int mMinutes;

        TimerDuration(final int minutes) {
            mMinutes = minutes;
        }

        public int getMinutes() {
            return mMinutes;
        }
    }

    private final String mActionText;
    private final int mActionImage;
    private final TimerDuration mTimerDuration;

    public TimerItemModel(String actionText, int actionImage, TimerDuration timerDuration) {
        this.mActionText = actionText;
        this.mActionImage = actionImage;
        this.mTimerDuration = timerDuration;
    }

    public String getActionText() {
        return mActionText;
    }

    public int getActionImage() {
        return mActionImage;
    }

    public TimerDuration getTimerDuration() {
        return mTimerDuration;
    }
}
