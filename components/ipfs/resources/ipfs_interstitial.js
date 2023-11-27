// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { HIDDEN_CLASS, SecurityInterstitialCommandId, sendCommand } from 'chrome://interstitials/common/resources/interstitial_common.js';
import { loadTimeData } from 'chrome://resources/js/load_time_data.js';

function setupEvents() {
  // `loadTimeDataRaw` is injected to the `window` scope from C++.
  loadTimeData.data = window.loadTimeDataRaw;

  const body = document.querySelector('#body');
  body.classList.add('ipfs');
  const icon = document.querySelector('#icon');
  icon.classList.add('icon');

  const primaryButton = document.querySelector('#primary-button');
  primaryButton.addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });

  const mainContent = document.querySelector('#main-content');
  mainContent.classList.remove(HIDDEN_CLASS);

  const detailsButton = document.querySelector('#details-button');
  detailsButton.addEventListener('click', function(event) {
    const hiddenDetails =
      document.querySelector('#details').classList.toggle(HIDDEN_CLASS);
    detailsButton.innerText = hiddenDetails ?
        loadTimeData.getString('openDetails') :
        loadTimeData.getString('closeDetails');
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);
