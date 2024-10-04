/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import successBackground1URL from '../../assets/success_background_1.svg'
import successBackground1DarkURL from '../../assets/success_background_1_dark.svg'

import successBackground2URL from '../../assets/success_background_2.svg'
import successBackground2DarkURL from '../../assets/success_background_2_dark.svg'

export const style = scoped.css`
  & {
    overflow: auto;

    @container style(--is-wide-view) {
      min-width: 350px;
    }
  }
`

export const backgroundStyle = scoped.css`
  & {
    --success-background-1-url: url(${successBackground1URL});
    --success-background-2-url: url(${successBackground2URL});

    @media (prefers-color-scheme: dark) {
      --success-background-1-url: url(${successBackground1DarkURL});
      --success-background-2-url: url(${successBackground2DarkURL});
    }
  }

  &.payment-form::before {
    position: absolute;
    top: 0;
    left: -9999px;
    height: 0;
    width: 0;
    background:
      no-repeat var(--success-background-1-url),
      no-repeat var(--success-background-2-url);
  }

  &.success {
    --modal-background:
      var(--success-background-1-url) no-repeat center top / auto 366px,
      var(--success-background-2-url) no-repeat center top / cover,
      ${color.container.background};

    @container style(--is-wide-view) {
      --modal-background:
        var(--success-background-1-url) no-repeat center 22px / auto 366px,
        var(--success-background-2-url) no-repeat center top / cover,
        ${color.container.background};
    }
  }
`
