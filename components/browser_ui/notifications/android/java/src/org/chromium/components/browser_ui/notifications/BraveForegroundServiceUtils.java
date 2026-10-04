/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.notifications;

import android.app.Notification;
import android.app.Service;
import android.content.pm.ServiceInfo;

public class BraveForegroundServiceUtils extends ForegroundServiceUtils {
    private static final String sBraveTalkServiceClassName = "PlaybackListenerMicService";

    @Override
    public void startForeground(
            Service service, int id, Notification notification, int foregroundServiceType) {
        // Check for a service that is dedicated to Brave Talk
        String serviceSimpleName = service.getClass().getSimpleName();
        if (serviceSimpleName.equals(sBraveTalkServiceClassName)
                || serviceSimpleName.endsWith("$" + sBraveTalkServiceClassName)) {
            foregroundServiceType |= ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE;
        }
        super.startForeground(service, id, notification, foregroundServiceType);
    }
}
