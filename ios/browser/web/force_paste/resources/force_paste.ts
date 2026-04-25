// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from '//ios/web/public/js_messaging/resources/gcrweb.js';

function pasteIntoActiveElement(contents: string) {
  let element: Element | null = document.activeElement;
  while (element?.tagName === 'IFRAME') {
    // If the element is an iframe, recurse into it to find the active element
    element = (element as HTMLIFrameElement).contentDocument?.activeElement ?? null;
  }
  if (element === null) {
    return;
  }
  if (!(element.tagName === 'INPUT' || element.tagName === 'TEXTAREA')) {
    return;
  }
  const inputElement = element as HTMLInputElement | HTMLTextAreaElement;
  const start = inputElement.selectionStart ?? 0;
  // Paste into expected position, replacing contents if any are selected
  inputElement.value =
      inputElement.value.slice(0, start) + contents +
      inputElement.value.slice(inputElement.selectionEnd ?? start);
  // Reset caret position to expected position
  const newSelection = start + contents.length;
  inputElement.selectionStart = newSelection;
  inputElement.selectionEnd = newSelection;
}

const forcePasteApi = new CrWebApi('forcePaste');
forcePasteApi.addFunction('pasteIntoActiveElement', pasteIntoActiveElement);
gCrWeb.registerApi(forcePasteApi);
