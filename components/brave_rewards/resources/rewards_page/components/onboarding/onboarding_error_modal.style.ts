/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  & {
    max-width: var(--onboarding-max-width);
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    margin-top: calc(-1 * var(--modal-header-padding-bottom) - 12px);
  }

  .icon {
    --leo-icon-size: 40px;
    --leo-icon-color: ${color.systemfeedback.warningIcon};
    margin-bottom: 16px;
  }

  .title {
    margin: 0 3px 0;
    font: ${font.heading.h3};
    color: ${color.text.primary};
  }

  .text {
    margin: 8px 3px 0 3px;
    color: ${color.text.secondary};
    font: ${font.default.regular};

    a {
      padding-left: 4px;
      text-decoration: underline;
    }
  }

  .action {
    width: 100%;
    margin-top: 24px;
    margin-bottom: 4px;
    display: flex;
    align-items: stretch;
  }
`
