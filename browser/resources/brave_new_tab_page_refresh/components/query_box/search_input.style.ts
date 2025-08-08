/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`

  & {
    display: flex;
    flex-direction: column;
  }

  input {
    margin: 16px;
    border: none;
    padding: 0;
    font: ${font.large.regular};
    outline: none;
    background: inherit;
  }

  .search-actions {
    --search-engine-icon-size: 20px;

    display: flex;
    align-items: center;
    gap: 8px;
    padding: 0 8px 8px;
  }

  .search-button {
    --leo-button-padding: 8px;
    --leo-button-radius: 50%;

    flex: 0 0 auto;
  }

  .results-container:not(:empty) {
    border-top: solid 1px ${color.divider.subtle};
  }

`
