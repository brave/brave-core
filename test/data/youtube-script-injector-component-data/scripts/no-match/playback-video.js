// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

(function() {
  const script = document.createElement('script');
  // Modify the listener by ignoring visibilitychange
  // so when the app goes in background the video
  // is not paused.
  script.textContent = `
  if (document._addEventListener === undefined) {
    document._addEventListener = document.addEventListener;
    document.addEventListener = function(a,b,c) {
      if(a != 'visibilitychange') {
        document._addEventListener(a,b,c);
      }
    };
  }
  `;
  
  document.head.appendChild(script);
  script.remove();
}());
