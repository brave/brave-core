// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Should match ipfs::IPFSOnboardingPage::IPFSOnboardingCommandId
/** @enum {number} */
const IPFSOnboardingCommandId = {
  USE_LOCAL_NODE: 0,
  USE_PUBLIC_GATEWAY: 1
};

// Should match ipfs::IPFSOnboardingPage::IPFSOnboardingResponse
/** @enum {number} */
const IPFSOnboardingResponse = {
  LOCAL_NODE_ERROR: 0
};


function setupEvents() {
  $('body').classList.add('ipfs');
  $('icon').classList.add('icon');

  $('local-node-button').addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.USE_LOCAL_NODE);
  });

  $('public-gateway-button').addEventListener('click', function() {
    sendCommand(IPFSOnboardingCommandId.USE_PUBLIC_GATEWAY);
  });

  $('main-content').classList.remove(HIDDEN_CLASS);

}

document.addEventListener('DOMContentLoaded', setupEvents);

function handleError(code) {
  if (code == IPFSOnboardingResponse.LOCAL_NODE_ERROR) {
    console.log("TODO(Serge): Show message about fallback to public gateway");
  }
}

window.addEventListener("message", function(event) {
  if (!event.data || event.data.command != "ipfs-error")
    return
  handleError(event.data.value);
}, false);