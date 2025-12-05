/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .toggle-list {
    max-width: 380px;
    width: 100%;
    margin-left: auto;
    margin-right: auto;
    padding: 24px;
    display: flex;
    flex-direction: column;
    gap: 12px;

    > * {
      display: flex;
      align-items: center;
      gap: 8px;
      text-transform: capitalize;

      leo-toggle {
        margin-inline-start: auto;
      }
    }
  }
`

style.passthrough.css`
  p.header-text {
    font: ${font.small.regular};

    button {
      text-decoration: underline;
      color: ${color.text.interactive};
    }
  }
`
