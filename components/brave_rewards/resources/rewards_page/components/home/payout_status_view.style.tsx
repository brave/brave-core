/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    --leo-icon-size: 20px;
    --leo-icon-color: ${color.systemfeedback.infoIcon};

    color: ${color.systemfeedback.infoText};
    background: ${color.systemfeedback.infoBackground};
    padding: 16px;
    border-radius: 12px;
    font: ${font.default.regular};
    display: flex;
    gap: 16px;
    align-items: center;

    a {
      color: inherit;
    }
  }
`
