// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(() => {
  const getUserId = () => {
    return "user1";
  };

  const isInitial = () => {
    const psst = window.parent.localStorage.getItem('psst');
    if(!psst)
      return true

    const psstObj = JSON.parse(psst);
    if(!psstObj)
      return true

    return psstObj.state === "completed"
  }

  document.title = 'a_user-';
  return {
    user: getUserId(),
    is_initial: isInitial(),
    name: 'a.test',
    tasks: [
      { name: "privacy setting #1" },
      { name: "privacy setting #2" },
    ],
  };
})();