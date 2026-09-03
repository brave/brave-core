// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Media setting: rewrite YouTube PREF so the site prefers desktop.
// Gated by Settings → Media → Always use YouTube desktop site.

(function () {
  const host = location.hostname;
  if (!/(^|\.)youtube\.com$/.test(host) || host === "music.youtube.com") {
    return;
  }

  const desktopF6 = "40000000";
  let pref = "";
  for (const pair of document.cookie.split(";")) {
    const trimmed = pair.trim();
    if (trimmed.startsWith("PREF=")) {
      pref = trimmed.slice(5);
      break;
    }
  }

  const map = {};
  if (pref) {
    for (const part of pref.split("&")) {
      const eq = part.indexOf("=");
      if (eq === -1) {
        continue;
      }
      map[part.slice(0, eq)] = part.slice(eq + 1);
    }
  }

  if (map.f6 === desktopF6) {
    return;
  }

  map.f6 = desktopF6;
  const newPref = Object.keys(map)
    .map((key) => key + "=" + map[key])
    .join("&");
  document.cookie =
    "PREF=" + newPref +
    "; domain=.youtube.com; path=/; Secure; SameSite=Lax; max-age=31536000";
})();
