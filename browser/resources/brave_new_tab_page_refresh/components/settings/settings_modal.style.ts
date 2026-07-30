/* Copyright (c) 2025 The Brave Authors. All rights reserved.
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
  & {
    --leo-dialog-width: 860px;
    --leo-dialog-padding: 0;
    --leo-dialog-background: transparent;
    --leo-dialog-border-radius: 0;
    /* The dialog is now just a transparent frame; its own elevation would
       paint a shadow around empty space. Leo exposes no shadow variable. */
    --leo-effect-elevation-05: none;

    height: 0;
  }

  .dialog-frame {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: ${spacing.m};
  }

  .close {
    --leo-button-color: ${color.white};
    color: ${color.white};
  }

  .panel {
    width: 100%;
    max-height: 800px;
    display: flex;
    overflow: hidden;
    border-radius: ${radius.xl};
    background: ${color.container.background};
    box-shadow: ${effect.elevation['05']};
  }

  nav {
    flex: 0 0 228px;
    padding-bottom: ${spacing['2Xl']};
    white-space: nowrap;
  }

  h3 {
    padding: ${spacing['2Xl']};
    font: ${font.heading.h4};
    color: ${color.text.primary};
  }

  section {
    flex: 1 1 auto;
    min-width: 0;
    min-height: 0;
    padding: ${spacing.m} ${spacing.m} ${spacing.m} 0;
    display: flex;
    flex-direction: column;
  }

  section > .content {
    flex: 1 1 auto;
    min-height: 0;
    overflow-y: auto;
    overscroll-behavior: contain;
    scrollbar-width: thin;
    padding: ${spacing['4Xl']};
    border-radius: ${radius.xl};
    background: ${color.page.background};

    > h4 {
      padding-bottom: ${spacing.xl};
      color: ${color.text.primary};
    }

  }

  .section-title {
    --leo-icon-size: ${icon.m};

    display: flex;
    align-items: center;
    gap: ${spacing.s};
    color: inherit;
    font: inherit;
  }

  .settings-group {
    width: 100%;
    overflow: hidden;
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
    box-sizing: border-box;
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

    .label {
      flex: 1 1 auto;
    }
  }
`
