/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.shields;

import org.chromium.chrome.R;

public enum ShieldsTooltipEnum {
    ONE_TIME_ADS_TRACKER_BLOCKED_TOOLTIP(
            ShieldsTooltipEnumConstants.ONE_TIME_ADS_TRACKER_BLOCKED_TOOLTIP,
            R.string.tooltip_title_1, R.string.tooltip_text_1,
            R.color.shields_tooltip_arrow_color_1, R.drawable.shields_tooltip_background_1),
    VIDEO_ADS_BLOCKED_TOOLTIP(ShieldsTooltipEnumConstants.VIDEO_ADS_BLOCKED_TOOLTIP,
            R.string.tooltip_title_2, R.string.tooltip_text_2,
            R.color.shields_tooltip_arrow_color_2, R.drawable.shields_tooltip_background_2),
    ADS_TRACKER_BLOCKED_TOOLTIP(ShieldsTooltipEnumConstants.ADS_TRACKER_BLOCKED_TOOLTIP,
            R.string.tooltip_title_3, R.string.tooltip_text_3,
            R.color.shields_tooltip_arrow_color_2, R.drawable.shields_tooltip_background_2),
    HTTPS_UPGRADE_TOOLTIP(ShieldsTooltipEnumConstants.HTTPS_UPGRADE_TOOLTIP,
            R.string.tooltip_title_4, R.string.tooltip_text_4,
            R.color.shields_tooltip_arrow_color_2, R.drawable.shields_tooltip_background_2);

    private int id;
    private int title;
    private int text;
    private int arrowColor;
    private int tooltipBackground;

    ShieldsTooltipEnum(int id, int title, int text, int arrowColor, int tooltipBackground) {
        this.id = id;
        this.title = title;
        this.text = text;
        this.arrowColor = arrowColor;
        this.tooltipBackground = tooltipBackground;
    }

    public int getId() {
        return id;
    }

    public int getTitle() {
        return title;
    }

    public int getText() {
        return text;
    }

    public int getArrowColor() {
        return arrowColor;
    }

    public int getTooltipBackground() {
        return tooltipBackground;
    }

    interface ShieldsTooltipEnumConstants {
        static final int ONE_TIME_ADS_TRACKER_BLOCKED_TOOLTIP = 0;
        static final int VIDEO_ADS_BLOCKED_TOOLTIP = 1;
        static final int ADS_TRACKER_BLOCKED_TOOLTIP = 2;
        static final int HTTPS_UPGRADE_TOOLTIP = 3;
    }
}
