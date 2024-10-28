/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../lib/scoped_css'

export const style = scoped.css`

  & {
    pointer-events: none;
    position: fixed;
    inset: 0;
    z-index: -1;
    display: flex;
    animation-name: fade-in;
    animation-timing-function: ease-in-out;
    animation-duration: 350ms;
    animation-delay: 0s;
    animation-fill-mode: both;

    > div {
      flex: 1 1 auto;
      background-size: cover;
      background-repeat: no-repeat;
      background-position: center center;
    }
  }

  .image-background {
    background:
      linear-gradient(
        rgba(0, 0, 0, 0.8), rgba(0, 0, 0, 0) 35%, rgba(0, 0, 0, 0) 80%,
        rgba(0, 0, 0, 0.6) 100%),
      var(--ntp-background);
  }

  .color-background {
    background: var(--ntp-background);
  }

  @keyframes fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }

`
