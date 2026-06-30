/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
  }

  .custom-widgets {
    display: flex;
    flex-direction: column;
    gap: ${spacing.m};
    margin-top: ${spacing['2Xl']};
    padding-top: ${spacing['2Xl']};
    border-top: 1px solid ${color.divider.subtle};
  }

  .custom-widgets-header {
    font: ${font.default.semibold};
    color: ${color.text.primary};
  }

  .custom-widgets-description {
    font: ${font.small.regular};
    color: ${color.text.secondary};
  }

  .custom-widget-name,
  .custom-widget-html {
    width: 100%;
    box-sizing: border-box;
    padding: ${spacing.m};
    border-radius: ${spacing.m};
    border: 1px solid ${color.divider.strong};
    background: ${color.container.background};
    color: ${color.text.primary};
    font: ${font.small.regular};
  }

  .custom-widget-html {
    min-height: 120px;
    resize: vertical;
    font-family: monospace;
    white-space: pre;
  }

  .custom-widget-list {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
  }

  .custom-widget-list li {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: ${spacing.m};
  }

  .custom-widget-list-name {
    font: ${font.small.regular};
    color: ${color.text.primary};
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
`
