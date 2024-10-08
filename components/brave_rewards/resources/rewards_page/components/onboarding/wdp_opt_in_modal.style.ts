/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { scoped } from '../../lib/scoped_css'

import graphicImage from '../../assets/wdp_onboarding.svg'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    text-align: center;
    gap: 16px;

    @container style(--is-wide-view) {
      max-width: var(--onboarding-max-width);
    }
  }

  .graphic {
    flex: 0 0 120px;
    background-image: url(${graphicImage});
    background-position: center;
    background-size: auto 100%;
    background-repeat: no-repeat;
  }

  .checkbox-text {
    text-align: left;
  }

  .actions {
    padding-top: 16px;
    display: flex;
    flex-direction: column;
  }
`
