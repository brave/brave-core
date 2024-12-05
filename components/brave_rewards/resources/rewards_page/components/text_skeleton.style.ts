/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    --self-animation-color: rgba(0, 0, 0, 0.04);

    display: block;
    color: transparent;
    background: ${color.container.disabled};
    border-radius: 4px;
    position: relative;
    overflow: hidden;
    opacity: .7;

    animation: text-skeleton-fade-in 1s ease-in-out both 1s;

    @media (prefers-color-scheme: dark) {
      --self-animation-color: rgba(255, 255, 255, 0.08);
    }
  }

  &:after {
    content: '';
    position: absolute;
    transform: translateX(-100%);
    inset: 0;
    background: linear-gradient(
      90deg, transparent, var(--self-animation-color), transparent);
    animation: text-skeleton-background-cycle 2s linear 0.5s infinite;
  }

  @keyframes text-skeleton-fade-in {
    0% { opacity: 0; }
    100% { opacity: .7; }
  }

  @keyframes text-skeleton-background-cycle {
    0% { transform: translateX(-100%); }
    50% { transform: translateX(100%); }
    100% { transform: translateX(100%); }
  }
`
