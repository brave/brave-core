/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    overflow: auto;
    max-width: 500px;
    display: flex;
    flex-direction: column;
    gap: 16px;

    button {
      text-decoration: underline;
      cursor: pointer;
    }
  }
`

