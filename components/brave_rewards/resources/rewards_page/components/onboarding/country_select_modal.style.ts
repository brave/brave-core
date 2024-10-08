/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import selectCountry from '../../assets/select_country.svg'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;

    @container style(--is-wide-view) {
      max-width: var(--onboarding-max-width);
    }
  }

  .graphic {
    flex: 0 0 145px;
    background-image: url(${selectCountry});
    background-position: center;
    background-size: auto 100%;
    background-repeat: no-repeat;
  }

  .title {
    margin: 16px 0;
    font: ${font.heading.h3};
    color: ${color.text.primary};
    text-align: center;
  }

  .text {
    margin: 16px 0;
    font: ${font.default.regular};
    color: ${color.text.secondary};
    text-align: center;
  }

  .selector {
    margin-bottom: 16px;

    select {
      width: 100%;
      color: ${color.text.primary};
      font: ${font.default.regular};

      &.empty {
        color: ${color.text.secondary};
      }
    }
  }

  .actions {
    display: flex;
    flex-direction: column;
    gap: 8px;
    align-items: stretch;
  }
`
