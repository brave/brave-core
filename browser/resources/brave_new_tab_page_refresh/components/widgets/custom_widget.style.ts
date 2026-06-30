/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    position: relative;
    display: flex;
    width: 100%;
    height: 100%;
  }

  /* Position the (shared) widget menu 8px from the top/right for custom
     widgets. The wrapper div is the only direct <div> child here. */
  & > div {
    inset-block-start: 8px;
    inset-inline-end: 8px;
  }

  .widget-frame {
    border: 0;
    border-radius: 16px;
    width: 100%;
    height: 100%;
    min-height: 0;
    background: transparent;
    color-scheme: dark;
  }
`
