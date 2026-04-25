/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 24px;
    overflow: auto;
    max-width: 566px;
    padding-bottom: 24px;

    @container style(--is-wide-view) {
      max-width: 566px;
    }
  }

  .icon-header {
    --leo-icon-size: 24px;

    display: flex;
    align-items: center;

    .icon {
      height: 24px;
      width: auto;
    }

    > * {
      padding: 24px;
      border-radius: 50%;
    }

    .bat {
      display: flex;
      align-items: center;
      background: linear-gradient(
        180deg, rgba(243, 195, 5, 0.00) 0%, rgba(243, 195, 5, 0.20) 100%);
    }

    .link-broken {
      padding: 8px;
      border: solid 1px ${color.systemfeedback.warningVibrant};
      background: ${color.systemfeedback.warningBackground};
      margin: 0 -14px;
    }

    .provider {
      display: flex;
      align-items: center;
      background: linear-gradient(
        180deg, rgba(0, 220, 250, 0.00) 0%, rgba(0, 220, 250, 0.10) 100%);
    }
  }

  ul {
    color: ${color.text.secondary};
    padding: 0 16px;
    margin: 0 4px;
    list-style-type: disc;
  }

  li {
    padding: 2px 0;
  }

  .region-select {
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: stretch;

    label {
      font: ${font.small.semibold};
    }
  }
`
