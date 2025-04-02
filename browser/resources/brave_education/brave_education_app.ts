/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// based on //chrome/browser/resources/whats_new/whats_new_app.ts

import '/strings.m.js';

import {Command} from 'chrome://resources/brave/brave_browser_command.mojom-webui.js';
import {BrowserCommandProxy} from 'chrome://resources/brave/js/brave_browser_command/brave_browser_command_proxy.js';
import {EventTracker} from 'chrome://resources/js/event_tracker.js';
import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';
import type {Url} from 'chrome://resources/mojo/url/mojom/url.mojom-webui.js';

import {getCss} from './brave_education_app.css.js';
import {getHtml} from './brave_education_app.html.js';
import {BraveEducationProxyImpl} from './brave_education_proxy.js';

enum EventType {
  BROWSER_COMMAND = 'browser-command',
}

interface BrowserCommand {
  messageType: EventType.BROWSER_COMMAND;
  command: string;
  commandId: number;
}

function handleBrowserCommand(commandId: number) {
  const handler = BrowserCommandProxy.getInstance().handler;
  handler.canExecuteCommand(commandId).then(({canExecute}) => {
    if (canExecute) {
      handler.executeCommand(commandId);
    } else {
      console.warn('Received invalid command: ' + commandId);
    }
  });
}

export class BraveEducationAppElement extends CrLitElement {
  static get is() {
    return 'brave-education-app';
  }

  static override get styles() {
    return getCss();
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      url_: {type: String},
    };
  }

  protected accessor url_: string = '';

  private eventTracker_: EventTracker = new EventTracker();

  constructor() {
    super();

    // There are no subpages in What's New. Also remove the query param here
    // since its value is recorded.
    window.history.replaceState(undefined /* stateObject */, '', '/');
  }

  override connectedCallback() {
    super.connectedCallback();

    BraveEducationProxyImpl.getInstance()
        .handler.getServerUrl()
        .then(({url}: {url: Url}) => this.handleUrlResult_(url.url));
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    this.eventTracker_.removeAll();
  }

  /**
   * Handles the URL result of sending the initialize WebUI message.
   * @param url The What's New URL to use in the iframe.
   */
  private handleUrlResult_(url: string|null) {
    if (!url) {
      // This occurs in the special case of tests where we don't want to load
      // remote content.
      return;
    }

    this.url_ = url;

    this.eventTracker_.add(
        window, 'message',
        (event: Event) => this.handleMessage_(event as MessageEvent));
  }

  // Reads and validates a message posted from an education page iframe and
  // returns a Command ID that can be dispatched to the browser. If the
  // message is invalid, an error is thrown.
  private readCommandMessage(data: BrowserCommand): number {
    if (data.messageType !== 'browser-command') {
      throw new Error(`Invalid messageType "${ data.messageType }"`)
    }

    const commandNames = new Map<string, Command>(Object.entries({
      'open-rewards-onboarding': Command.kOpenRewardsOnboarding,
      'open-wallet-onboarding': Command.kOpenWalletOnboarding,
      'open-vpn-onboarding': Command.kOpenVPNOnboarding,
      'open-ai-chat': Command.kOpenAIChat
    }))

    const commandId = commandNames.get(String(data.command || ''))
    if (commandId === undefined) {
      throw new Error(`Unrecognized command "${ data.command }"`)
    }

    return commandId
  }

  private handleMessage_(event: MessageEvent) {
    if (!this.url_) {
      return;
    }

    const iframeUrl = new URL(this.url_);
    if (!event.data || event.origin !== iframeUrl.origin) {
      return;
    }

    const commandId = this.readCommandMessage(event.data as BrowserCommand);
    handleBrowserCommand(commandId);
  }
}

customElements.define(BraveEducationAppElement.is, BraveEducationAppElement);
