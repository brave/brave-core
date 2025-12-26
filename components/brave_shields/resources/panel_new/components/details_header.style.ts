/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    background: ${color.page.background};
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    border-bottom: solid 1px ${color.divider.subtle};
  }

  .header {
    --leo-icon-size: 20px;

    display: flex;
    gap: 8px;
    align-items: flex-start;

    button {
      margin-top: 4px;
    }
  }

  .text {
    flex: 1 1 auto;
  }

  .host {
    color: ${color.text.secondary};
  }
`
