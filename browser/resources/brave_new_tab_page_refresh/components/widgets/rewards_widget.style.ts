/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import rewardsBatCoinURL from '../../assets/rewards_bat_coin.svg'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .title {
    --leo-icon-size: 16px;
    font: ${font.components.buttonSmall};
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .content {
    display: flex;
    gap: 12px;
    align-items: center;
  }

  .text {
    flex: 1 1 auto;
  }

  &.onboarding {
    .text {
      --leo-icon-size: 12px;
      --leo-icon-color: ${color.icon.disabled};

      font: ${font.xSmall.regular};
      display: flex;
      flex-direction: column;
      gap: 4px;

      > * {
        display: flex;
        align-items: center;
        gap: 8px;
      }
    }
  }

  .coin-graphic {
    height: 62px;
    width: 62px;
    background-image: url(${rewardsBatCoinURL});
    background-position: left center;
    background-size: cover;
  }

  &.connected .text {
    flex: 1 1 auto;
    font: ${font.small.regular};
  }

  .actions {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;

    leo-button {
      --leo-button-color: rgba(255, 255, 255, 0.2);
      white-space: nowrap;
      flex-grow: 0;
    }

    a {
      font: ${font.xSmall.link};
      text-decoration: underline;
      color: rgba(255, 255, 255, .5);
    }
  }
`
