/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import rewardsBatCoinURL from '../../assets/rewards_bat_coin.svg'
import rewardsConnectURL from '../../assets/rewards_connect.svg'

export const style = scoped.css`
  & {
    flex-grow: 1;
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
    flex-grow: 1;
    display: flex;
    gap: 12px;
    align-items: center;
  }

  .text {
    flex: 1 1 auto;
  }

  &.onboarding .text {
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

  .coin-graphic {
    height: 62px;
    width: 62px;
    background-image: url(${rewardsBatCoinURL});
    background-position: left center;
    background-size: contain;
    background-repeat: no-repeat;
  }

  .connect-graphic {
    height: 62px;
    width: 71px;
    background-image: url(${rewardsConnectURL});
    background-position: left center;
    background-size: contain;
    background-repeat: no-repeat;
  }

  &.unconnected .text {
    font: ${font.small.regular};
    color: ${color.text.tertiary};

    .header {
      font: ${font.default.semibold};
      color: ${color.text.primary};
    }
  }

  &.connected .header {
    font: ${font.xSmall.regular};
    opacity: 0.5;
    padding-bottom: 4px;
  }

  &.login {
    --leo-icon-size: 40px;
    --provider-icon-color: currentcolor;

    svg {
      margin-right: 4px;
      height: 16px;
      width: auto;
      display: block;
    }

    .header {
      font: ${font.default.semibold};
      color: #fff;
    }

    .text {
      font: ${font.xSmall.regular};
    }
  }

  .balance {
    display: flex;
    align-items: center;
    gap: 4px;
    font: ${font.large.semibold};

    .bat-label {
      opacity: 0.5;
    }

    .exchange {
      font: ${font.xSmall.regular};
      padding-left: 4px;
    }

    &.skeleton {
      width: 160px;
      height: 24px;
    }
  }

  .payout-status {
    margin-top: 3px;
    display: flex;
    align-items: center;
    gap: 6px;
    font: ${font.xSmall.regular};

    a {
      color: inherit;
    }
  }

  .ads-viewed {
    --leo-icon-size: 14px;

    margin-top: 3px;
    font: ${font.xSmall.regular};
    color: rgba(255, 255, 255, 0.5);
    display: flex;
    gap: 8px;
    align-items: center;

    .ad-count {
      color: #fff;
      font: ${font.small.semibold};
    }

    leo-icon {
      color: #fff;
      opacity: 0.5;
    }

    leo-tooltip [slot='content'] {
      font: ${font.default.regular};
      max-width: 200px;
    }
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
