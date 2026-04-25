// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { HIDDEN_CLASS, SecurityInterstitialCommandId, sendCommand } from 'chrome://interstitials/common/resources/interstitial_common.js';

// Clickjacking protection: delay before accepting proceed clicks
const PROCEED_CLICK_DELAY_MS = 500;
let proceedClicksEnabled = false;

function setupEvents() {
  const body = document.querySelector('#body');
  body.classList.add('decentralized_dns');
  const icon = document.querySelector('#icon');
  icon.classList.add('icon');

  const primaryButton = document.querySelector('#primary-button');
  primaryButton.addEventListener('click', function(event) {
    // Prevent clickjacking by requiring a delay before allowing proceed
    if (!proceedClicksEnabled) {
      event.preventDefault();
      return;
    }
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });

  const mainContent = document.querySelector('#main-content');
  mainContent.classList.remove(HIDDEN_CLASS);

  const dontProceedButton = document.querySelector('#dont-proceed-button')
  dontProceedButton.addEventListener('click', function() {
    // Don't proceed is always allowed (safer action)
    sendCommand(SecurityInterstitialCommandId.CMD_DONT_PROCEED);
  });

  // Enable proceed clicks after delay to prevent clickjacking
  setTimeout(function() {
    proceedClicksEnabled = true;
  }, PROCEED_CLICK_DELAY_MS);
}

document.addEventListener('DOMContentLoaded', setupEvents);
