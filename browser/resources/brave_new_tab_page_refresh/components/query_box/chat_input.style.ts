/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    container-type: inline-size;
  }

  form {
    border: none !important;
  }

  .chat-tools {
    --leo-dialog-padding: 0;

    position: relative;

    > * {
      position: absolute;
      inset: 2px 0 auto;
    }

    leo-buttonmenu > * {
      max-width: 100cqi;
    }
  }
`
