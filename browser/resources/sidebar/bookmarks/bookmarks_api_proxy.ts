// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ChromeEvent} from '/tools/typescript/definitions/chrome_event.js';
import {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';

import {BookmarksPageHandlerFactory, BookmarksPageHandlerRemote} from './sidebar.mojom-webui.js';

let instance: BookmarksApiProxy|null = null;

export class BookmarksApiProxy {
  callbackRouter: {[key: string]: ChromeEvent<Function>};
  handler: BookmarksPageHandlerRemote;

  constructor() {
    this.callbackRouter = {
      onChanged: chrome.bookmarks.onChanged,
      onChildrenReordered: chrome.bookmarks.onChildrenReordered,
      onCreated: chrome.bookmarks.onCreated,
      onMoved: chrome.bookmarks.onMoved,
      onRemoved: chrome.bookmarks.onRemoved,
    };

    this.handler = new BookmarksPageHandlerRemote();

    const factory = BookmarksPageHandlerFactory.getRemote();
    factory.createBookmarksPageHandler(
        this.handler.$.bindNewPipeAndPassReceiver());
  }

  cutBookmark(id: string): Promise<void> {
    chrome.bookmarkManagerPrivate.cut([id]);
    return Promise.resolve();
  }

  copyBookmark(id: string): Promise<void> {
    return new Promise(resolve => {
      chrome.bookmarkManagerPrivate.copy([id], resolve);
    });
  }

  getFolders(): Promise<chrome.bookmarks.BookmarkTreeNode[]> {
    return new Promise(resolve => chrome.bookmarks.getTree(results => {
      if (results[0] && results[0].children) {
        resolve(results[0].children);
        return;
      }
      resolve([]);
    }));
  }

  openBookmark(url: string, depth: number, clickModifiers: ClickModifiers) {
    this.handler.openBookmark({url}, depth, clickModifiers);
  }

  pasteToBookmark(parentId: string, destinationId?: string): Promise<void> {
    const destination = destinationId ? [destinationId] : [];
    return new Promise(resolve => {
      chrome.bookmarkManagerPrivate.paste(parentId, destination, resolve);
    });
  }

  showContextMenu(id: string, x: number, y: number) {
    this.handler.showContextMenu(id, {x, y});
  }

  static getInstance() {
    return instance || (instance = new BookmarksApiProxy());
  }

  static setInstance(obj: BookmarksApiProxy) {
    instance = obj;
  }
}