/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../../ui/components/report_view/report_view.js'
import '../../ui/kit/kit.js'

import * as SDK from '../../core/sdk/sdk.js'
import * as Lit from '../../ui/lit/lit.js'
import * as UI from '../../ui/legacy/legacy.js'

const { render, html } = Lit

interface AdblockFilterRuleInfo {
  rawLine: string
  sourceIndex: number
  lineNumber: number
  sourceTitle?: string
}

interface AdblockInfo {
  requestUrl: string
  checkedUrl: string
  sourceHost: string
  resourceType?: string
  aggressive: boolean
  blocked: boolean
  didMatchImportantRule: boolean
  didMatchRule: boolean
  didMatchException: boolean
  hasMockData: boolean
  rewrittenUrl?: string
  filter?: AdblockFilterRuleInfo
  exception?: AdblockFilterRuleInfo
}

const styles = `
  .code {
    font-family: var(--monospace-font-family);
    font-size: var(--monospace-font-size);
    user-select: text;
  }
  .verdict-icon {
    margin: 0 0.3em var(--sys-size-2) 0;
    vertical-align: middle;
    &.blocked {
      color: var(--icon-error);
    }
    &.allowed {
      color: var(--icon-checkmark-green);
    }
  }
  .empty {
    padding: var(--sys-size-6) var(--sys-size-9);
    color: var(--sys-color-on-surface-subtle);
  }
`

function hasLocation(n: number): boolean {
  // Unknown locations arrive as -1, or 2^32-1 after the uint32 hop.
  return Number.isInteger(n) && n >= 0 && n !== 0xffffffff
}

function renderRow(
  key: string,
  value: string | Lit.LitTemplate | undefined,
  isCode = false
): Lit.LitTemplate {
  if (value === undefined || value === '') {
    return Lit.nothing
  }
  return html`
    <devtools-report-key>${key}</devtools-report-key>
    <devtools-report-value
      >${isCode
        ? html`<span class="code">${value}</span>`
        : value}</devtools-report-value
    >
  `
}

function renderVerdict(info: AdblockInfo): Lit.LitTemplate {
  let label = 'Allowed by exception'
  if (info.blocked) {
    label = info.hasMockData ? 'Blocked, replaced with a stub resource' : 'Blocked'
  }
  const notes = []
  if (info.blocked && info.didMatchImportantRule && info.didMatchException) {
    notes.push('an $important rule overrides the exception')
  } else if (info.didMatchImportantRule) {
    notes.push('$important rule')
  }
  if (!info.blocked && info.hasMockData) {
    notes.push('a redirect rule matched but was not applied')
  }
  return html`
    <devtools-icon
      class="verdict-icon medium ${info.blocked ? 'blocked' : 'allowed'}"
      name=${info.blocked ? 'cross-circle-filled' : 'check-circle'}
    ></devtools-icon>
    <strong>${label}</strong>${notes.length ? ` (${notes.join(', ')})` : ''}
  `
}

function renderRuleSection(
  title: string,
  rule: AdblockFilterRuleInfo | undefined
): Lit.LitTemplate {
  if (!rule) {
    return Lit.nothing
  }
  let list: string | undefined
  if (hasLocation(rule.sourceIndex)) {
    list = rule.sourceTitle
      ? `${rule.sourceTitle} (source ID ${rule.sourceIndex})`
      : `Source ID ${rule.sourceIndex}`
  }
  return html`
    <devtools-report-section-header>${title}</devtools-report-section-header>
    ${renderRow('Rule', rule.rawLine, true)}
    ${renderRow('Filter list', list)}
    ${renderRow(
      'Line',
      hasLocation(rule.lineNumber) ? `${rule.lineNumber}` : undefined
    )}
    <devtools-report-divider></devtools-report-divider>
  `
}

function renderReport(info: AdblockInfo): Lit.LitTemplate {
  // The engine only keeps rule text while adblock debug mode is on.
  const ruleUnavailable =
    !info.filter &&
    !info.exception &&
    (info.didMatchRule || info.didMatchException)
  return html`
    <devtools-report>
      <devtools-report-section-header>Result</devtools-report-section-header>
      ${renderRow('Verdict', renderVerdict(info))}
      ${renderRow('Blocking mode', info.aggressive ? 'Aggressive' : 'Standard')}
      ${renderRow('Rewritten URL', info.rewrittenUrl)}
      ${ruleUnavailable
        ? renderRow(
            'Matched rule',
            'Turn on Debug mode in brave://adblock-internals and restart ' +
              'the browser to see which rule matched.'
          )
        : Lit.nothing}
      <devtools-report-divider></devtools-report-divider>
      ${renderRuleSection('Matched filter', info.filter)}
      ${renderRuleSection('Matched exception', info.exception)}
      <devtools-report-section-header>Request</devtools-report-section-header>
      ${renderRow('Request URL', info.requestUrl)}
      ${info.checkedUrl !== info.requestUrl
        ? renderRow('Checked URL', info.checkedUrl)
        : Lit.nothing}
      ${renderRow('Initiator host', info.sourceHost)}
      ${renderRow('Resource type', info.resourceType)}
    </devtools-report>
  `
}

export class RequestAdblockView extends UI.Widget.VBox {
  readonly #request: SDK.NetworkRequest.NetworkRequest
  #manager: SDK.NetworkManager.NetworkManager | null

  constructor(request: SDK.NetworkRequest.NetworkRequest) {
    super()
    this.#request = request
    this.#manager = SDK.NetworkManager.NetworkManager.forRequest(this.#request)
  }

  override performUpdate(): void {
    const info = this.#manager
      ? ((this.#manager.dispatcher as any).adblockInfoForId(
          this.#request.requestId()
        ) as AdblockInfo | null)
      : null
    render(
      html`
        <style>
          ${styles}
        </style>
        ${info
          ? renderReport(info)
          : html`<div class="empty">
              No adblock rule blocked or allowed this request.
            </div>`}
      `,
      this.contentElement,
      { host: this }
    )
  }

  override wasShown(): void {
    super.wasShown()
    this.#request.addEventListener(
      SDK.NetworkRequest.Events.FINISHED_LOADING,
      this.requestUpdate,
      this
    )
    // Widget doesn't render on show.
    this.requestUpdate()
  }

  override willHide(): void {
    super.willHide()
    this.#request.removeEventListener(
      SDK.NetworkRequest.Events.FINISHED_LOADING,
      this.requestUpdate,
      this
    )
  }
}
