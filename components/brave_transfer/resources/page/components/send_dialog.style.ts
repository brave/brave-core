/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  &.send {
    display: flex;
    flex-direction: column;
    gap: ${spacing.l};
    min-width: min(480px, 80vw);
  }

  .text-actions {
    display: flex;
    justify-content: flex-end;
  }

  .drop-zone {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: ${spacing.m};
    padding: ${spacing['2Xl']};
    border: 2px dashed ${color.divider.strong};
    border-radius: ${radius.l};
    color: ${color.text.secondary};
    font: ${font.default.regular};
    --leo-icon-size: 24px;
  }

  .drop-zone[data-drag-over=true] {
    border-color: ${color.primary[40]};
    background: ${color.container.highlight};
  }
`
