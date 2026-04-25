/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  .list-header {
    padding: 8px 16px;
    display: flex;
    align-items: center;
    gap: 8px;
    font: ${font.default.semibold};

    button {
      margin-inline-start: auto;
      font: ${font.small.link};
      text-decoration: underline;
      color: ${color.text.primary};
    }
  }

  .allow-header {
    color: ${color.systemfeedback.successText};
    background: ${color.systemfeedback.successBackground};
  }

  .block-header {
    color: ${color.systemfeedback.errorText};
    background: ${color.systemfeedback.errorBackground};
  }

  .resource-list {
    padding: 8px;
  }
`
