/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    padding: ${spacing.l} ${spacing['2Xl']} ${spacing.l};
    border-top: solid 1px ${color.material.separator};
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
    gap: ${spacing.l};
  }

  .color-swatch {
    position: relative;
    appearance: none;
    margin: 0;
    padding: 2px;
    border: none;
    border-radius: ${radius.full};
    background: none;
    cursor: pointer;
    outline: solid 2px transparent;
    outline-offset: 2px;
    transition: outline-color 0.12s ease-in-out;

    .checkmark {
      --leo-icon-size: 28px;
      --leo-icon-color: ${color.icon.interactive};

      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      border-radius: ${radius.full};

      /* The check glyph is a transparent cut-out in the filled circle. A white
       * disc placed behind it (smaller than the circle, so it stays hidden by
       * the colored ring) shows through the cut-out as a white check. */
      &::before {
        content: '';
        position: absolute;
        inset: 20%;
        border-radius: ${radius.full};
        background: ${color.white};
        z-index: -1;
      }
    }

    &:hover {
      outline-color: ${color.divider.subtle};
    }

    &.selected {
      outline-color: ${color.icon.interactive};
    }

    &:focus-visible {
      outline-color: ${color.icon.interactive};
    }
  }

  .theme-color {
    display: block;
    width: 36px;
    height: 36px;
    border-radius: ${radius.full};
  }
`
