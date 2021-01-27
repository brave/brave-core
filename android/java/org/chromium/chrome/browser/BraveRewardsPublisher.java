/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */
package org.chromium.chrome.browser;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

class BraveRewardsPublisher {
    //PublisherStatus @
    //vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({NOT_VERIFIED, CONNECTED, UPHOLD_VERIFIED, BITFLYER_VERIFIED})
    public @interface PublisherStatus {}
    public static final int NOT_VERIFIED = 0;
    public static final int CONNECTED = 1;
    public static final int UPHOLD_VERIFIED = 2;
    public static final int BITFLYER_VERIFIED = 3;
}
