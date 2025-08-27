// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  const getUserId = () => {
    return "user1"
  }
  document.title = 'a_user-';
  return {
      "user": getUserId(),
      "tasks": [
        {"name": "privacy setting #1"},
        {"name": "privacy setting #2"},
      ]
  }
})();