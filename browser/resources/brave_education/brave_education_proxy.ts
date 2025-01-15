/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// based on //chrome/browser/resources/whats_new/whats_new_proxy.ts

import {PageCallbackRouter, PageHandlerFactory, PageHandlerRemote} from './brave_education.mojom-webui.js';
import type {PageHandlerInterface} from './brave_education.mojom-webui.js';

export interface BraveEducationProxy {
  callbackRouter: PageCallbackRouter;
  handler: PageHandlerInterface;
}

export class BraveEducationProxyImpl implements BraveEducationProxy {
  handler: PageHandlerInterface
  callbackRouter: PageCallbackRouter;

  private constructor() {
    this.handler = new PageHandlerRemote();
    this.callbackRouter = new PageCallbackRouter();
    PageHandlerFactory.getRemote().createPageHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        (this.handler as PageHandlerRemote).$.bindNewPipeAndPassReceiver());
  }

  static getInstance(): BraveEducationProxy {
    return instance || (instance = new BraveEducationProxyImpl());
  }

  static setInstance(proxy: BraveEducationProxy) {
    instance = proxy;
  }
}

let instance: BraveEducationProxy|null = null;
