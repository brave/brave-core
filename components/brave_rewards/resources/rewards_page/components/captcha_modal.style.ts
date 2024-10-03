/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../lib/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  iframe {
    width: 332px;
    height: 275px;
    background: ${color.container.background};
    border: 0;
  }

  .content {
    --leo-icon-size: 40px;

    display: flex;
    flex-direction: column;
    gap: 16px;
    margin-top: calc(-1 * var(--modal-header-padding-bottom) - 24px);
    text-align: center;

    @container style(--is-wide-view) {
      max-width: 375px;
    }

    .icon {
      display: flex;
      justify-content: center;
    }
  }

  .solved {
    --leo-icon-color: ${color.systemfeedback.successIcon};

  }

  .max-attempts-exceeded {
    --leo-icon-color: ${color.systemfeedback.warningIcon};
  }
`

