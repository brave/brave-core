/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    margin: 80px auto 0;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  .image {
    --leo-icon-size: 50px;
    color: ${color.systemfeedback.errorIcon};
  }

  .title {
    padding-top: 25px;
    font: ${font.heading.h3};
    color: ${color.text.primary};
    text-align: center;
  }

  .details {
    margin: 30px auto 0;
    white-space: pre;
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    max-width: 100%;
    padding: 0 20px 20px;
    overflow-x: auto;
  }
`
