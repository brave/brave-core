/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .toggle-row {
    display: flex;
    align-items: center;

    label {
      flex: 1 1 auto;
    }
  }

  .background-options {
    display: flex;
    flex-wrap: wrap;
    gap: 16px;

    button {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
  }

  .preview {
    background: var(--preview-background, ${color.container.highlight});
    background-size: cover;
    background-repeat: no-repeat;
    background-position: center center;
    border-radius: 10px;
    width: 198px;
    height: 156px;
  }

  .background-option {
    position: relative;

    &:hover .remove-image {
      visibility: visible;
    }
  }

  .selected-marker {
    --leo-icon-color: #fff;
    --leo-icon-size: 24px;

    position: absolute;
    inset-block-start: 10px;
    inset-inline-end: 10px;
    background: ${color.icon.interactive};
    border-radius: 50%;
    padding: 6px;


    .allow-remove:hover & {
      visibility: hidden;
    }
  }

  .remove-image {
    --leo-icon-size: 24px;

    position: absolute;
    inset-block-start: 10px;
    inset-inline-end: 10px;
    background-color: #fff;
    border-radius: 50%;
    box-shadow: rgba(0, 0, 0, 0.5) 0px 0px 5px;
    padding: 6px;
    visibility: hidden;

    &:hover {
      color: ${color.icon.interactive};
    }
  }

  .upload {
    --leo-icon-size: 36px;
    --leo-progressring-size: 36px;

    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    gap: 16px;
    border: solid 2px ${color.divider.subtle};
    font: ${font.small.regular};
  }

  h4 button {
    --leo-icon-size: 20px;

    display: flex;
    align-items: center;
    gap: 4px;

    &:hover {
      color: ${color.text.interactive};
    }
  }
`
