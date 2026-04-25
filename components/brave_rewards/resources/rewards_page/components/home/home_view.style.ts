/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;

    @container style(--is-wide-view) {
      gap: 24px;
    }
  }

  .columns {
    display: flex;
    gap: 24px;

    > * {
      flex: 1 1 50%;
      display: flex;
      flex-direction: column;
      gap: 24px;
    }
  }
`
