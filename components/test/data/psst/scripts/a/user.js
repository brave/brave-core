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
      "user_id": getUserId(),
      "site_name": "site name",
      "tasks": [
        {"uid": "1", "url": "https://a.test/settings/privacy_setting_1", "description": "privacy setting #1"},
        {"uid": "2", "url": "https://a.test/settings/privacy_setting_2", "description": "privacy setting #2"},
      ]
  }
})();
