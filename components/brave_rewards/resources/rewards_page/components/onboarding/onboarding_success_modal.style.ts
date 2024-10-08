/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import successIconLight from '../../assets/onboarding_success_icon.svg'
import successIconDark from '../../assets/onboarding_success_icon_dark.svg'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    margin-top: calc(-1 * var(--modal-header-padding-bottom) - 24px);

    @container style(--is-wide-view) {
      max-width: var(--onboarding-max-width);
    }
  }

  .success-icon {
    margin:  13px auto 50px auto;
    height: 90px;
    width: 315px;
    background-image: url(${successIconLight});
    background-position: center;
    background-repeat: no-repeat;
    background-size: contain;

    @media (prefers-color-scheme: dark) {
      background-image: url(${successIconDark});
    }
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

  .learn-more {
    margin-top: 16px;
    margin-bottom: 16px;
    text-align: center;

    a {
      color: ${color.text.interactive};
      font: ${font.components.buttonDefault};
      text-decoration: none;
    }
  }
`
