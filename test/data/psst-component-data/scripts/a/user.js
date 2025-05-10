// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  const getUserId = () => {
    return "user1"
  }
  return new Promise((resolve) => {
    document.title = 'user-';
    resolve({
      "user": getUserId(),
      "requests": [
        {"name": "privacy setting #1"},
        {"name": "privacy setting #2"},
      ]
    })
  })
})();
