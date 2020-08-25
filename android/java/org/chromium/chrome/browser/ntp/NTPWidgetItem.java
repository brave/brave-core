/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

class NTPWidgetItem {
    private String widgetTitle;
    private String widgetText;

    NTPWidgetItem(String widgetTitle, String widgetText) {
        this.widgetTitle = widgetTitle;
        this.widgetText = widgetText;
    }

    NTPWidgetItem(NTPWidgetItem ntpWidgetItem) {
        this.widgetTitle = ntpWidgetItem.widgetTitle;
        this.widgetText = ntpWidgetItem.widgetText;
    }

    String getWidgetTitle() {
        return widgetTitle;
    }

    String getWidgetText() {
        return widgetText;
    }
}
