// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Should match ipfs::IPFSOnboardingPage::IPFSOnboardingCommandId
/** @enum {number} */
const IPFSOnboardingCommandId = {
  USE_LOCAL_NODE: 0,
  USE_PUBLIC_GATEWAY: 1,
  LEARN_MORE: 2,
  OPEN_SETTINGS: 3
};

// Should match ipfs::IPFSOnboardingPage::IPFSOnboardingResponse
/** @enum {number} */
const IPFSOnboardingResponse = {
  LOCAL_NODE_ERROR: 0,
  THEME_CHANGED: 1,
  LOCAL_NODE_LAUNCHED: 2
};

const setTheme = (theme) => {
  document.body.className = `${theme.toLowerCase()}`;
}

function setupEvents() {
  $('local-node-button').addEventListener('click', function() {
    $('local-node-button').textContent = '$i18nRaw{installationText}'
    $('error-container').className = 'error-container-hidden'
    sendCommand(IPFSOnboardingCommandId.USE_LOCAL_NODE);
  });

  $('public-gateway-button').addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.USE_PUBLIC_GATEWAY);
  });

  $('learn-more').addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.LEARN_MORE);
  });

  $('open-settings').addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.OPEN_SETTINGS);
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);

function handleCommand(code, text) {
  if (code == IPFSOnboardingResponse.LOCAL_NODE_ERROR) {
    $('error-container').textContent = text
    $('error-container').className = 'error-container-visible'
    $('local-node-button').textContent = '$i18nRaw{retryText}'
  } else if (code == IPFSOnboardingResponse.THEME_CHANGED) {
    setTheme(text)
  } else if (code == IPFSOnboardingResponse.LOCAL_NODE_LAUNCHED) {
    $('local-node-button').textContent = '$i18nRaw{watingPeersText}'
  }
}

window.addEventListener("message", function(event) {
  if (!event.data || event.data.command != "ipfs")
    return
  handleCommand(event.data.value, event.data.text);
}, false);