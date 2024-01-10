// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce("PlaylistFolderSharingScript", function($) {
  let sendMessage = $(function(pageUrl) {
    $.postNativeMessage('$<message_handler>', {
      "securityToken": SECURITY_TOKEN,
      "pageUrl": pageUrl
    });
  });

  if (!window.brave) {
    window.brave = {};
  }

  if (!window.brave.playlist) {
    window.brave.playlist = {};
    window.brave.playlist.open = $(function(pageUrl) {
      sendMessage(pageUrl);
    });
  }
});
