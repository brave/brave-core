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
  LOCAL_NODE_LAUNCHED: 1,
  NO_PEERS_AVAILABLE: 2,
  NO_PEERS_LIMIT: 3,
  INSTALLATION_ERROR: 4,
  THEME_CHANGED_DARK: 5,
  THEME_CHANGED_LIGHT: 6
};

const setTheme = (theme) => {
  document.body.className = `${theme.toLowerCase()}`;
}

function setup() {
  // No local node option on Android
  if ('$i18nRaw{os}' == 'Android') {
    $('local-node-box').style.display = 'none'
    $('footer').style.display = 'none'
  }

  $('local-node-button').addEventListener('click', function() {
    $('local-node-button').textContent = '$i18n{installationText}'
    $('error-container').className = 'error-container-hidden'
    $('local-node-button').className = 'button'
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

document.addEventListener('DOMContentLoaded', setup);

function showErrorMessage(text) {
  $('error-container').textContent = text
  $('error-container').className = 'error-container-visible'
}

function handleCommand(code, text) {
  if (code == IPFSOnboardingResponse.LOCAL_NODE_ERROR) {
    showErrorMessage('$i18n{localNodeError}')
    $('local-node-button').textContent = '$i18n{retryText}'
  } else if (code == IPFSOnboardingResponse.THEME_CHANGED_DARK) {
    setTheme("dark")
  } else if (code == IPFSOnboardingResponse.THEME_CHANGED_LIGHT) {
    setTheme("light")
  } else if (code == IPFSOnboardingResponse.LOCAL_NODE_LAUNCHED) {
    $('local-node-button').textContent = '$i18n{watingPeersText}'
  } else if (code == IPFSOnboardingResponse.NO_PEERS_AVAILABLE) {
    showErrorMessage('$i18n{peersError}'.replace('{value}', text))
  } else if (code == IPFSOnboardingResponse.NO_PEERS_LIMIT) {
    showErrorMessage('$i18n{retryLimitPeersText}')
    $('local-node-button').textContent = '$i18n{tryAgainText}'
    $('local-node-button').className = 'button button-retry'
  } else if (code == IPFSOnboardingResponse.INSTALLATION_ERROR) {
    showErrorMessage('$i18n{installationError}')
    $('local-node-button').textContent = '$i18n{tryAgainText}'
    $('local-node-button').className = 'button button-retry'
  }
  return true
}

function messageHandler (event) {
  if (event.type !== 'message' || event.origin !== document.location.origin) {
    return false
  }
  
  if (!event.data || event.data.command !== 'ipfs')
    return false
    
  if (isNaN(parseInt(event.data.code)) ||
      (event.data.value !== '' && isNaN(parseInt(event.data.value)))) {
    return false;
  }
  return handleCommand(event.data.code, event.data.value);
}

window.addEventListener("message", messageHandler, false);

// for testing purposes
if (typeof module !== 'undefined') {
  module.exports = messageHandler
}
