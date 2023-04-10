/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on Chromium code subject to the following license:
// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the Cookies and Local Storage Data
 * section.
 */

// clang-format off
import {sendWithPromise} from 'chrome://resources/js/cr.js';
import {CookieDetails} from './cookie_info.js';
// clang-format on

export interface LocalDataItem {
  localData: string;
  site: string;
}

export interface LocalDataBrowserProxy {

  /**
   * Removes all local data (local storage, cookies, etc.).
   * Note: on-tree-item-removed will not be sent.
   */
  removeAll(): Promise<void>;

  /**
   * Remove data for a specific site. Completion signaled by
   * on-tree-item-removed.
   */
  removeSite(site: string): void;

  /**
   * Gets the cookie details for a particular site.
   */
  getCookieDetails(site: string): Promise<CookieDetails[]>;


  /**
   * Removes a given piece of site data.
   * @param path The path to the item in the tree model.
   */
  removeItem(path: string): void;
}

export class LocalDataBrowserProxyImpl implements LocalDataBrowserProxy {
  removeAll() {
    return sendWithPromise('localData.removeAll');
  }

  removeSite(site: string) {
    chrome.send('localData.removeSite', [site]);
  }

  getCookieDetails(site: string) {
    return sendWithPromise('localData.getCookieDetails', site);
  }

  removeItem(path: string) {
    chrome.send('localData.removeItem', [path]);
  }

  static getInstance(): LocalDataBrowserProxy {
    return instance || (instance = new LocalDataBrowserProxyImpl());
  }

  static setInstance(obj: LocalDataBrowserProxy) {
    instance = obj;
  }
}

let instance: LocalDataBrowserProxy|null = null;
