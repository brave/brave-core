/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  color,
  effect,
  font,
  icon,
  radius,
  spacing,
} from '@brave/leo/tokens/css/variables'

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  h4 {
    padding-bottom: ${spacing.xl};
    color: ${color.text.primary};
  }

  .title {
    --leo-icon-size: ${icon.m};

    display: flex;
    align-items: center;
    gap: ${spacing.s};
    color: inherit;
    font: inherit;
  }

  .content {
    background: ${color.container.background};
    box-shadow: ${effect.elevation['01']};
    border-radius: ${radius.xl};
  }
`

style.passthrough.css`
  .selected-marker {
    --leo-icon-color: ${color.white};
    --leo-icon-size: ${icon.xs};

    position: absolute;
    inset-block-end: ${spacing.m};
    inset-inline-end: ${spacing.m};
    display: flex;
    align-items: center;
    justify-content: center;
    width: ${icon.l};
    height: ${icon.l};
    background: ${color.icon.interactive};
    border: 2px solid ${color.white};
    border-radius: ${radius.full};
  }

  .control-row,
  .toggle-row {
    padding: ${spacing.l} ${spacing['2Xl']};
    border-bottom: solid 1px ${color.divider.subtle};

    &:last-child {
      border-bottom: none;
    }
  }

  .control-row {
    display: flex;
    align-items: center;
    gap: ${spacing.m};

    label {
      flex: 1 1 auto;
    }
  }

  .toggle-row {
    --leo-toggle-label-flex-direction: row-reverse;
    --leo-toggle-label-gap: ${spacing.xl};

    .label {
      --leo-icon-size: ${icon.m};

      flex: 1 1 auto;
      display: flex;
      align-items: center;
      gap: ${spacing.xl};
      color: ${color.text.primary};
    }

    .subtext {
      font: ${font.small.regular};
      color: ${color.text.secondary};

      a {
        color: inherit;
      }
    }
  }
`
