/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  color,
  duration,
  easing,
  icon,
  radius,
  spacing,
} from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    --leo-checkbox-size: ${icon.l};

    display: flex;
    gap: ${spacing['2Xl']};
    align-items: center;
    padding: ${spacing.xl} ${spacing['2Xl']} ${spacing.xl} ${spacing.xl} ;
    border-radius: ${radius.xl};
    background: ${color.material.regular};
    cursor: pointer;
    transition:
      opacity ${duration.s} ${easing.out},
      filter ${duration.s} ${easing.out};
  }

  &.unchecked {
    opacity: 0.5;
    filter: grayscale(1);
  }

  img {
    width: 100px;
    height: auto;
    border-radius: ${radius.l};
  }

  p {
    color: ${color.text.secondary};
  }

  a {
    color: inherit;
    text-decoration: underline;
  }

  .text {
    flex: 1;
  }
`
