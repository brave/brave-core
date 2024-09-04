/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    max-width: 350px;
    margin: 0 auto;
    display: flex;
    flex-direction: column;
    gap: 48px;
  }

  .message {
    padding-top: 165px;
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    gap: 10px;
    width: 260px;
    margin: 0 auto;
  }

  .title {
    font: ${font.default.semibold};
    color: ${color.systemfeedback.successIcon};
  }

  h4 {
    color: ${color.text.interactive};
  }
`
