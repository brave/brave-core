/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;

public class BraveHelper {
  public static final String SHARED_PREF_DISPLAYED_INFOBAR_PROMO =
            "displayed_data_reduction_infobar_promo";

  public BraveHelper() {}

  public static void DisableFREDRP() {
      CommandLine.getInstance().appendSwitch(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE);
      FirstRunStatus.setFirstRunFlowComplete(true);
      
      // Disables data reduction promo dialog
      ContextUtils.getAppSharedPreferences()
              .edit()
              .putBoolean(SHARED_PREF_DISPLAYED_INFOBAR_PROMO, true);
  }
}
