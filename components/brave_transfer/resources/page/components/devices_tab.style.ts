/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: ${spacing['2Xl']};
  }

  section {
    display: flex;
    flex-direction: column;
    gap: ${spacing.m};
  }

  h2 {
    font: ${font.heading.h4};
  }

  ul {
    display: flex;
    flex-direction: column;
    margin: 0;
    padding: 0;
    list-style: none;
    border-radius: ${radius.l};
    border: 1px solid ${color.divider.subtle};
    background: ${color.container.background};
    overflow: hidden;
  }

  li + li {
    border-top: 1px solid ${color.divider.subtle};
  }

  .device {
    display: flex;
    align-items: center;
    gap: ${spacing.l};
    padding: ${spacing.l} ${spacing.xl};
  }

  .device[data-drag-over=true] {
    background: ${color.container.highlight};
    outline: 2px dashed ${color.primary[40]};
    outline-offset: -4px;
  }

  .device-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    flex: 0 0 auto;
    width: 40px;
    height: 40px;
    border-radius: ${radius.m};
    background: ${color.container.highlight};
    --leo-icon-size: 24px;
    --leo-icon-color: ${color.icon.secondary};
  }

  .device[data-connected=true] .device-icon {
    --leo-icon-color: ${color.icon.interactive};
  }

  .device-info,
  .transfer-info {
    display: flex;
    flex-direction: column;
    gap: ${spacing.xs};
    flex: 1 1 auto;
    min-width: 0;
  }

  .device-name,
  .transfer-summary {
    font: ${font.default.semibold};
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .device-status,
  .transfer-meta {
    display: flex;
    align-items: center;
    gap: ${spacing.s};
    font: ${font.small.regular};
    color: ${color.text.tertiary};
  }

  .device[data-connected=true] .device-status {
    color: ${color.systemfeedback.successText};
  }

  .status-dot {
    width: 8px;
    height: 8px;
    border-radius: ${radius.full};
    background: ${color.systemfeedback.successIcon};
  }

  .device-error {
    font: ${font.small.regular};
    color: ${color.systemfeedback.errorText};
  }

  .transfer {
    display: flex;
    align-items: center;
    gap: ${spacing.l};
    padding: ${spacing.m} ${spacing.xl};
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  .transfer-info leo-progressbar {
    margin-top: ${spacing.xs};
  }
`
