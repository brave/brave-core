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

  .label .subtext {
    font: ${font.small.regular};
    color: ${color.text.secondary};

    a {
      color: inherit;
    }
  }

  .brave-backgrounds-description {
    margin: 0;
    padding: ${spacing['2Xl']} ${spacing['2Xl']} 0;
    font: ${font.small.regular};
    color: ${color.text.secondary};
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
    opacity: 1;
    filter: grayscale(0);
    transition:
      opacity 0.2s ease,
      filter 0.2s ease,
      box-shadow 0.2s ease;

    &.selected {
      box-shadow: ${effect.elevation['01']};
    }

    &.inactive {
      opacity: 0.5;
      filter: grayscale(1);
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
    &:focus-within,
    &.disabled {
      .overlay-button {
        visibility: visible;
      }
    }
  }

  .overlay-button {
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

    &:disabled {
      opacity: 0.5;
    }
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
