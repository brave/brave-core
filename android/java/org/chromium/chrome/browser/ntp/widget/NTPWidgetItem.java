/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.widget;

public class NTPWidgetItem {
    private String widgetType;
    private String widgetTitle;
    private String widgetText;

    public NTPWidgetItem(String widgetType, String widgetTitle, String widgetText) {
        this.widgetType = widgetType;
        this.widgetTitle = widgetTitle;
        this.widgetText = widgetText;
    }

    public NTPWidgetItem(NTPWidgetItem ntpWidgetItem) {
        this.widgetType = ntpWidgetItem.widgetType;
        this.widgetTitle = ntpWidgetItem.widgetTitle;
        this.widgetText = ntpWidgetItem.widgetText;
    }

    public String getWidgetType() {
        return widgetType;
    }

    public String getWidgetTitle() {
        return widgetTitle;
    }

    public String getWidgetText() {
        return widgetText;
    }
}
