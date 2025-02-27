// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { SecurityInterstitialCommandId, sendCommand } from 'chrome://interstitials/common/resources/interstitial_common.js';
import { loadTimeData } from 'chrome://resources/js/load_time_data.js';

function setupEvents() {
  const primaryButton = document.querySelector('#primary-button');
  primaryButton.addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });

  const backButton = document.querySelector('#back-button');
  backButton.addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_DONT_PROCEED);
  });

  // Check if we should show the "Don't warn again" checkbox.
  const showCheckbox = loadTimeData.getBoolean('showDontWarnAgainCheckbox');
  if (showCheckbox) {
    const dontWarnAgain = document.querySelector('#dont-warn-again-checkbox')
    dontWarnAgain.addEventListener('click', function() {
      sendCommand(dontWarnAgain.checked ?
                  SecurityInterstitialCommandId.CMD_DO_REPORT :
                  SecurityInterstitialCommandId.CMD_DONT_REPORT);
    });
  }
}

document.addEventListener('DOMContentLoaded', setupEvents);
