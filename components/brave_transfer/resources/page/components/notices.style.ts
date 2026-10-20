/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  &.permissions {
    display: flex;
    flex-direction: column;
    gap: ${spacing.m};
    padding: ${spacing.xl};
    border-radius: ${radius.l};
    border: 1px solid ${color.divider.subtle};
    background: ${color.container.background};
  }

  h3 {
    font: ${font.heading.h4};
  }

  .permissions-text {
    color: ${color.text.secondary};
  }

  ul {
    display: flex;
    flex-direction: column;
    gap: ${spacing.s};
    margin: 0;
    padding: 0;
    list-style: none;
  }

  li {
    display: flex;
    align-items: center;
    gap: ${spacing.m};
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.icon.default};
  }

  .permission-name {
    flex: 1 1 auto;
  }

  .permission-status {
    font: ${font.small.semibold};
    color: ${color.systemfeedback.warningText};
  }

  .permission-status[data-allowed=true] {
    color: ${color.systemfeedback.successText};
  }

  .permission-actions {
    display: flex;
    flex-wrap: wrap;
    gap: ${spacing.m};
  }
`
