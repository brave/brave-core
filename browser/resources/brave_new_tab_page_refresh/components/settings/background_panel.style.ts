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
    display: flex;
    flex-direction: column;
  }

  .background-options {
    padding: ${spacing['2Xl']};
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: ${spacing.xl};
  }

  .preview {
    position: relative;
    background: var(--preview-background, ${color.page.background});
    background-size: cover;
    background-repeat: no-repeat;
    background-position: center center;
    border-radius: ${radius.m};
    border: solid 1px ${color.divider.faint};
    width: 100%;
    height: auto;
    aspect-ratio: 4 / 3;

    &.selected {
      box-shadow: ${effect.elevation['01']};
    }
  }

  .background-option {
    position: relative;
    text-align: center;

    > button:first-child {
      display: flex;
      flex-direction: column;
      width: 100%;
      gap: ${spacing.m};
      padding-bottom: ${spacing.m};
    }

    &:hover,
    &:focus-within {
      .remove-image {
        visibility: visible;
      }
    }
  }

  .remove-image {
    --leo-icon-color: ${color.icon.default};
    --leo-icon-size: ${icon.m};

    position: absolute;
    inset-block-start: ${spacing.m};
    inset-inline-end: ${spacing.m};
    display: flex;
    align-items: center;
    justify-content: center;
    width: ${icon.l};
    height: ${icon.l};
    background-color: ${color.white};
    border-radius: ${radius.full};
    visibility: hidden;
  }

  .upload {
    --leo-icon-size: 24px;
    --leo-progressring-size: 24px;

    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    gap: ${spacing.xl};
    border: solid 2px ${color.divider.faint};
    font: ${font.small.regular};
  }
`
