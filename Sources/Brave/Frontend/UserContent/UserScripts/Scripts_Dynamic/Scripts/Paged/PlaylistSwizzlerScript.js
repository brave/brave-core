// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
  window.MediaSource = null;
  window.WebKitMediaSource = null;
  //HTMLMediaElement.prototype.webkitSourceAddId = null;
  //window.SourceBuffer = null;
  
  delete window.MediaSource;
  delete window.WebKitMediaSource;
}
