// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: block;
    overflow: auto;
    max-height: 70vh;
    border: 1px solid var(--leo-color-divider-subtle);
    border-radius: var(--leo-radius-m);
    background: var(--leo-color-container-background);
  }

  table {
    width: 100%;
    border-collapse: collapse;
  }

  th {
    position: sticky;
    top: 0;
    z-index: 1;
    text-align: start;
    white-space: nowrap;
    padding: var(--leo-spacing-m);
    font: var(--leo-font-small-semibold);
    background: var(--leo-color-container-background);
    border-bottom: 1px solid var(--leo-color-divider-subtle);
  }

  td {
    padding: var(--leo-spacing-m);
    vertical-align: top;
    border-bottom: 1px solid var(--leo-color-divider-subtle);
    font: var(--leo-font-small-regular);
  }

  .time,
  .tab,
  .site {
    white-space: nowrap;
    color: var(--leo-color-text-secondary);
    font: var(--leo-font-monospace-small);
  }

  .level {
    display: flex;
    align-items: center;
    gap: var(--leo-spacing-s);
    white-space: nowrap;
  }

  .level leo-icon {
    --leo-icon-size: var(--leo-icon-s);
  }

  .level-debug leo-icon {
    color: var(--leo-color-icon-default);
  }

  .level-info leo-icon {
    color: var(--leo-color-systemfeedback-info-icon);
  }

  .level-warn leo-icon {
    color: var(--leo-color-systemfeedback-warning-icon);
  }

  .level-error leo-icon {
    color: var(--leo-color-systemfeedback-error-icon);
  }

  .message {
    white-space: pre-wrap;
    word-break: break-word;
  }

  .data {
    margin-top: var(--leo-spacing-s);
    font: var(--leo-font-monospace-small);
    color: var(--leo-color-text-secondary);
  }

  .data pre {
    margin: var(--leo-spacing-s) 0 0;
    white-space: pre-wrap;
    word-break: break-all;
  }

  .empty {
    padding: var(--leo-spacing-2xl);
    text-align: center;
    color: var(--leo-color-text-tertiary);
  }
`
