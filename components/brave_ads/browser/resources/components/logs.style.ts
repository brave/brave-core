/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-dialog-padding: 24px;
  }

  h4 {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;

    > * {
      flex: 0 1 auto;
    }
  }

  .title {
    flex: 1 1 auto;
  }

  .log-view {
    font-family: monospace;
    font-size: 13px;
    padding: 8px;
    white-space: pre-wrap;
    /* Set inline in JS (see logs.tsx): a fixed calc(dvh - Npx) is only
       correct for whatever happens to render above this element right
       now, and silently breaks again the next time that changes. */
    width: 100%;
    border-radius: 12px;
    background: ${color.container.background};
    border: none;
    overflow-y: auto;
    box-sizing: border-box;
    /* Without this, the browser's own scroll anchoring can adjust
       scrollTop when a toggle (Errors only, Plain text) changes how many
       lines render, firing a scroll event that looks like the user
       scrolled up and incorrectly turns off auto-scroll. */
    overflow-anchor: none;
  }

  .log-line-error {
    color: ${color.systemfeedback.errorText};
  }

  .log-line-warning {
    color: ${color.systemfeedback.warningText};
  }

  .log-line-request-response {
    font-weight: bold;
  }

  .log-line-response-success {
    color: ${color.systemfeedback.successText};
  }

  .log-line-response-client-error {
    color: ${color.systemfeedback.warningText};
  }

  .log-line-response-server-error {
    color: ${color.systemfeedback.errorText};
  }

  .log-line-response-error {
    color: ${color.systemfeedback.errorText};
  }

  .log-line-prefix {
    color: ${color.text.tertiary};
  }

  /* Matches the full (unfiltered) log's line number, so it's usable to find
     the same line after downloading the full log. A distinct color from
     .log-line-prefix so it doesn't blend into the timestamp/file text next
     to it. */
  .log-line-number {
    display: inline-block;
    font-family: monospace;
    width: 5em;
    text-align: right;
    margin-inline-end: 8px;
    user-select: none;
    color: ${color.icon.disabled};
  }

  .log-line-jump-to-full {
    cursor: pointer;
    margin-inline-start: 4px;
  }

  .log-line-marker {
    --leo-icon-size: 8px;
    --leo-icon-color: ${color.icon.interactive};
    margin-inline-end: 4px;
  }

  .log-divider {
    border: none;
    border-top: solid 1px ${color.divider.subtle};
    margin: 8px 0;
  }

  leo-toggle {
    font: ${font.small.semibold};
    margin-inline-end: 4px;
  }

  .verbose-logging-info {
    --leo-icon-size: 48px;
    --leo-icon-color: ${color.systemfeedback.warningIcon};

    display: flex;
    flex-direction: column;
    gap: 16px;
    background: ${color.systemfeedback.warningBackground};
    padding: 24px;
    border-radius: 8px;
    border: solid 1px ${color.systemfeedback.warningIcon};
    color: ${color.systemfeedback.warningText};
  }
`
