/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    position: absolute;
  }

  .toast {
    position: fixed;
    inset: 50px 0 0;
    margin: 0 auto;
    width: 450px;
    max-width: calc(100vw - 32px);

    border-radius: 8px;
    background: ${color.systemfeedback.infoBackground};
    box-shadow: ${effect.elevation['04']};
    transform: translateY(-50px);
    opacity: 0;

    transition:
      opacity 100ms,
      transform 100ms,
      display 100ms allow-discrete,
      overlay 100ms allow-discrete;

    &:popover-open {
      transform: translateY(0);
      opacity: 1;

      @starting-style {
        transform: translateY(-50px);
        opacity: 0;
      }
    }
  }

  .content {
    --leo-icon-color: ${color.systemfeedback.infoIcon};
    --leo-icon-size: 20px;

    display: flex;
    align-items: center;
    gap: 16px;

    padding: 16px;
    color: ${color.systemfeedback.infoText};
  }

  .text {
    flex-grow: 1;
  }

  leo-button {
    flex-grow: 0;
  }
`
