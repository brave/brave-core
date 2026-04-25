// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from
    '//ios/web/public/js_messaging/resources/gcrweb.js';

const kRolesToSkip: string[] = [
  'audio', 'banner', 'button', 'complementary',
  'contentinfo', 'footer', 'img', 'label', 'navigation',
  'textbox', 'combobox', 'listbox', 'checkbox', 'radiobutton',
  'slider', 'spinbutton', 'searchbox',
];

const kTagsToSkip: string[] = [
  'AUDIO', 'HEADER', 'BUTTON', 'ASIDE',
  'FOOTER', 'IMG', 'PICTURE', 'LABEL', 'NAV',
  'INPUT', 'SEARCH', 'STYLE',
];

// Walk the node tree to find <main> and <article> tags and return them.
function getRootNodes(): Node[] {
  const result: Node[] = [];
  const queue: Node[] = [document.documentElement];
  while (queue.length !== 0) {
    const node = queue.pop()!;
    const el = node as HTMLElement;
    if ((el.role === 'main' || node.nodeName === 'MAIN') ||
        (el.role === 'article' || node.nodeName === 'ARTICLE')) {
      result.push(node);
      continue;
    }
    for (const child of node.childNodes) {
      queue.push(child);
    }
  }
  return result;
}

// Recursively collect text from root, skipping unwanted roles and tags.
function collectText(root: Node, out: string[]): void {
  const queue: Node[] = [root];
  while (queue.length !== 0) {
    const node = queue.pop()!;
    const el = node as HTMLElement;
    if (el.role && kRolesToSkip.includes(el.role)) {
      continue;
    }
    if (kTagsToSkip.includes(node.nodeName)) {
      continue;
    }
    if (node.nodeType === Node.TEXT_NODE) {
      out.push((node as Text).wholeText);
    }
    for (const child of node.childNodes) {
      queue.push(child);
    }
  }
}

function getMainArticle(): string {
  const rootNodes = getRootNodes();
  const textParts: string[] = [];
  for (const node of rootNodes) {
    collectText(node, textParts);
  }
  const text = textParts.join(' ');
  return text.length !== 0 ?
      text :
      (document.body ? document.body.innerText : '');
}

const aiChatDistillerApi = new CrWebApi('aiChatDistiller');
aiChatDistillerApi.addFunction('getMainArticle', getMainArticle);
gCrWeb.registerApi(aiChatDistillerApi);
