// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.includeOnce("WindowRenderScript", function($) {
  Object.defineProperty(window.__firefox__, "$<window_render_script>", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: Object.freeze({
      "resizeWindow": $(function () {
        var evt = document.createEvent('UIEvents');
        evt.initUIEvent('resize', true, false, window, 0);
        window.dispatchEvent(evt);
      })
    })
  });

  document.addEventListener('readystatechange', $(function(){
    let eventHandler = $(function(e) {
      if (e.target.readyState === "interactive") {
        window.__firefox__.$<window_render_script>.resizeWindow();
      }

      if (e.target.readyState === "complete") {
        window.__firefox__.$<window_render_script>.resizeWindow();
      }
    });

    return eventHandler;
  })());
});
