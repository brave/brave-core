// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

"use strict";

window.__firefox__.includeOnce("ForcePasteScript", function() {
  Object.defineProperty(window.__firefox__, "forcePaste", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: function(contents, securityToken) {
      if (securityToken !== SECURITY_TOKEN) {
        return;
      }
      var element = document.activeElement;
      if (element instanceof HTMLIFrameElement) {
        // If the element is an iframe, grab its active element
        element = element.contentWindow.document.activeElement;
      }
      if (element === undefined) {
        return;
      }
      var start = element.selectionStart;
      // Paste into expected position, replacing contents if any are selected
      element.value = element.value.slice(0, start) + contents + element.value.slice(element.selectionEnd);
      // Reset caret position to expected position
      var newSelection = start + contents.length;
      element.selectionStart = newSelection;
      element.selectionEnd = newSelection;
    }
  });
});
