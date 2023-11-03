// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Stub out the MediaSource API so video players do not attempt to use `blob` for streaming

if (window.MediaSource || window.WebKitMediaSource || window.ManagedMediaSource || (window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId)) {
  delete window.MediaSource;
  delete window.WebKitMediaSource;
  
  // This API is only availale in iOS 17.1+ and only available in WebKit atm. The proposal to get it in all browsers is currently still open.
  delete window.ManagedMediaSource;
  
//  window.MediaSource = undefined;
//  window.WebKitMediaSource = undefined;
//  window.ManagedMediaSource = undefined;
//
//  HTMLMediaElement.prototype.webkitSourceAddId = undefined;
//  window.SourceBuffer = undefined;
}
