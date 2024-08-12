/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { buttonReset } from '../../shared/lib/css_mixins'

const commonStyles = `
  ${buttonReset}

  padding: 16px 20px;
  border: 1px solid transparent;
  border-radius: 1000px;
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;

  &:hover {
    cursor: pointer;
  }

  &[disabled] {
    cursor: default;
  }

  .icon {
    display: block;
    margin: 0 auto;
    height: 20px;
    width: auto;
  }
`

export const primaryButton = `
  ${commonStyles}

  --self-background-color: ${color.button.background};

  --self-lighten-opacity: 0;

  --self-lighten-gradient: linear-gradient(
    rgba(255, 255, 255, var(--self-lighten-opacity)) 0,
    rgba(255, 255, 255, var(--self-lighten-opacity)) 100%);

  color: ${color.schemes.onPrimary};
  background: var(--self-lighten-gradient), var(--self-background-color);

  &:hover {
    --self-lighten-opacity: 0.08;
  }

  &:active, &.pressed {
    --self-lighten-opacity: 0.14;
  }

  &[disabled]:not(.pressed) {
    --self-background-color: var(--leo-color-neutral-30);
    --self-lighten-opacity: 0.5;
    color: ${color.white};

    @media (prefers-color-scheme: dark) {
      --self-background-color: var(--leo-color-neutral-20);
      --self-lighten-opacity: 0;
    }
  }
`

export const secondaryButton = `
  ${commonStyles}

  border-color: var(--leo-color-primary-20);
  color: var(--leo-color-text-interactive);

  &:hover {
    border-color: var(--leo-color-primary-30);
  }

  @media (prefers-color-scheme: dark) {
    border-color: rgba(135, 132, 244, 0.4);

    &:hover {
      border-color: rgba(135, 132, 244, 0.6);
    }
  }
`
