// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Send a synchronous message to the browser process, serializing the request
// message & response.
//
// Note: This is one-time use per navigation, further prompts will be ignored
export function sendPromptMessage(handlerName: string, message: object): object|null {
  const response = window.prompt(JSON.stringify({
    'handler': handlerName,
    'message': message
  }));
  if (!response) {
    return null;
  }
  try {
    return JSON.parse(response)
  } catch (err) {
    console.error('Failed to parse window.prompt response ' + err);
    return null;
  }
}
