/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    position: fixed;
    inset: 0;
    z-index: 0;
    display: flex;
    animation-name: fade-in;
    animation-timing-function: ease-in-out;
    animation-duration: 350ms;
    animation-delay: 0s;
    animation-fill-mode: both;

    > * {
      flex: 1 1 auto;
    }
  }

  .image-background {
    background-size: cover;
    background-repeat: no-repeat;
    background-position: center center;
    background-image: var(--ntp-background);
  }

  .color-background {
    background: var(--ntp-background);
  }

  iframe {
    border: none;
    opacity: 1;
    transition: opacity 250ms;

    &.loading {
      opacity: 0;
    }
  }

  @keyframes fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }
`
