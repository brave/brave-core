/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    flex: 1 1 auto;
    display: flex;
    align-items: stretch;
    pointer-events: auto;
  }

  a {
    --leo-icon-size: 20px;

    flex: 1 1 auto;
    display: flex;
    flex-direction: column;
    align-items: end;
    color: ${color.white};

    leo-icon {
      opacity: 0;
      transition: opacity 200ms;
    }

    &:hover {
      leo-icon {
        opacity: .7;
      }
    }
  }
`
