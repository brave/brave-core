// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendCommand } from 'chrome://interstitials/common/resources/interstitial_common.js';

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

function setupEvents() {
  // No local node option on Android
  if ('$i18nRaw{os}' == 'Android') {
    document.querySelector('#local-node-box').style.display = 'none'
    document.querySelector('#footer').style.display = 'none'
  }

  const localNodeButton = document.querySelector('#local-node-button')
  localNodeButton.addEventListener('click', function() {
    localNodeButton.textContent = '$i18n{installationText}'
    const errorContainer = document.querySelector('#error-container')
    errorContainer.className = 'error-container-hidden'
    localNodeButton.className = 'button'
    sendCommand(IPFSOnboardingCommandId.USE_LOCAL_NODE);
  });

  const publicGatewayButton = document.querySelector('#public-gateway-button')
  publicGatewayButton.addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.USE_PUBLIC_GATEWAY);
  });

  const learnMode = document.querySelector('#learn-more')
  learnMode.addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.LEARN_MORE);
  });

  const openSettings = document.querySelector('#open-settings')
  openSettings.addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.OPEN_SETTINGS);
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);

function showErrorMessage(text) {
  const errorContainer = document.querySelector('#error-container')
  errorContainer.textContent = text
  errorContainer.className = 'error-container-visible'
}

function handleCommand(code, text) {
  const localNodeButton = document.querySelector('#local-node-button')
  if (code == IPFSOnboardingResponse.LOCAL_NODE_ERROR) {
    showErrorMessage('$i18n{localNodeError}')
    localNodeButton.textContent = '$i18n{retryText}'
  } else if (code == IPFSOnboardingResponse.THEME_CHANGED_DARK) {
    setTheme("dark")
  } else if (code == IPFSOnboardingResponse.THEME_CHANGED_LIGHT) {
    setTheme("light")
  } else if (code == IPFSOnboardingResponse.LOCAL_NODE_LAUNCHED) {
    localNodeButton.textContent = '$i18n{watingPeersText}'
  } else if (code == IPFSOnboardingResponse.NO_PEERS_AVAILABLE) {
    showErrorMessage('$i18n{peersError}'.replace('{value}', text))
  } else if (code == IPFSOnboardingResponse.NO_PEERS_LIMIT) {
    showErrorMessage('$i18n{retryLimitPeersText}')
    localNodeButton.textContent = '$i18n{tryAgainText}'
    localNodeButton.className = 'button button-retry'
  } else if (code == IPFSOnboardingResponse.INSTALLATION_ERROR) {
    showErrorMessage('$i18n{installationError}')
    localNodeButton.textContent = '$i18n{tryAgainText}'
    localNodeButton.className = 'button button-retry'
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
