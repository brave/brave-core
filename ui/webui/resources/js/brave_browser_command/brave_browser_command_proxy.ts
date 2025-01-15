/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// based on //ui/webui/resources/js/browser_command/browser_command_proxy.ts

import {BraveBrowserCommandHandlerFactory, BraveBrowserCommandHandlerRemote} from '../../brave_browser_command.mojom-webui.js';

/**
 * @fileoverview This file provides a class that exposes the Mojo handler
 * interface used for sending the browser commands to the browser and
 * receiving the browser response.
 */

let instance: BrowserCommandProxy|null = null;

export class BrowserCommandProxy {
  static getInstance(): BrowserCommandProxy {
    return instance || (instance = new BrowserCommandProxy());
  }

  static setInstance(newInstance: BrowserCommandProxy) {
    instance = newInstance;
  }

  handler: BraveBrowserCommandHandlerRemote;

  constructor() {
    this.handler = new BraveBrowserCommandHandlerRemote();
    const factory = BraveBrowserCommandHandlerFactory.getRemote();
    factory.createBrowserCommandHandler(
        this.handler.$.bindNewPipeAndPassReceiver());
  }
}
