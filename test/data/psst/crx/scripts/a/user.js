// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  const getUserId = () => {
    return document.getElementById("current_user_id")?.textContent
  }
  const curUrl = window.location.href
  console.log("[PSST USER SCRIPT] Current URL: " + curUrl);
  return {
    user_id: getUserId(),
    share_experience_link: "https://a.test:1111/?text=$1",
    name: 'a.test',
    tasks: [
      {
        url: 'https://a.test:1111/a_test_1.html',
        description: 'a_test_1.html'
      },
      {
        url: 'https://a.test:1111/a_test_2.html',
        description: 'a_test_2.html'
      }
    ]
  }
})();
