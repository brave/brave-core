/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BraveRewardsServiceReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        Intent mBraveRewardsServiceIntent = new Intent(context, BraveRewardsService.class);
        context.startService(mBraveRewardsServiceIntent);
    }
}
