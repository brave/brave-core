/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    width: 500px;
    max-width: 100%;
    padding: 24px;
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .label {
    font: ${font.small.semibold};
  }

  .actions {
    padding-top: 8px;
    display: flex;
    justify-content: flex-end;
    align-items: center;
    gap: 8px;

    leo-button {
      flex: 0 0 auto;
    }
  }
`
