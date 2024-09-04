/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.secondary};
    border-radius: 8px;
    background: ${color.page.background};

    > * {
      display: flex;
      gap: 8px;
      align-items: center;
      padding: 8px;
      border-bottom: solid 1px ${color.container.background};

      > :first-child {
        flex: 1 0 auto;
      }
    }

    > :last-child {
      border-bottom: none;
    }
  }

  .toggle-text {
    display: flex;
    min-width: 24px;
    justify-content: center;
  }

  .value {
    font: ${font.default.semibold};
    color: ${color.text.primary};
  }
`
