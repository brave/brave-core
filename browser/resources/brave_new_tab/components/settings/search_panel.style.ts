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

  .search-engines {
    --leo-checkbox-flex-direction: row-reverse;
    --leo-checkbox-label-gap: 16px;
    --leo-icon-size: 20px;

    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .engine-name {
    flex: 1 1 auto;
  }

  .engine-icon {
    width: 20px;
    height: 20px;
  }

  h4 {
    font: ${font.default.semibold};
  }

  .divider {
    height: 1px;
    background: ${color.divider.subtle};
  }

  .customize-link {
    --leo-icon-size: 20px;

    display: inline-flex;
    align-items: center;
    gap: 8px;
    text-decoration: none;
    color: ${color.text.primary};
  }
`
