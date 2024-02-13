// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { HIDDEN_CLASS, SecurityInterstitialCommandId, sendCommand } from 'chrome://interstitials/common/resources/interstitial_common.js';

function setupEvents() {
  const body = document.querySelector('#body');
  body.classList.add('decentralized_dns');
  const icon = document.querySelector('#icon');
  icon.classList.add('icon');

  const primaryButton = document.querySelector('#primary-button');
  primaryButton.addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });

  const mainContent = document.querySelector('#main-content');
  mainContent.classList.remove(HIDDEN_CLASS);

  const dontProceedButton = document.querySelector('#dont-proceed-button')
  dontProceedButton.addEventListener('click', function(event) {
    sendCommand(SecurityInterstitialCommandId.CMD_DONT_PROCEED);
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);
