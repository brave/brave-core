// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

function setupEvents() {
  $('body').classList.add('ipfs');
  $('icon').classList.add('icon');

  $('primary-button').addEventListener('click', function() {
    sendCommand(SecurityInterstitialCommandId.CMD_PROCEED);
  });

  $('main-content').classList.remove(HIDDEN_CLASS);

  $('details-button').addEventListener('click', function(event) {
    const hiddenDetails = $('details').classList.toggle(HIDDEN_CLASS);
    $('details-button').innerText = hiddenDetails ?
        loadTimeData.getString('openDetails') :
        loadTimeData.getString('closeDetails');
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);
