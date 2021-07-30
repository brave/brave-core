// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

function setupEvents() {
  $('primary-button').addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });
  $('back-button').addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_DONT_PROCEED);
  });
  $('dont-warn-again-checkbox').addEventListener('click', function() {
    sendCommand($('dont-warn-again-checkbox').checked ?
                SecurityInterstitialCommandId.CMD_DO_REPORT :
                SecurityInterstitialCommandId.CMD_DONT_REPORT);
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);
