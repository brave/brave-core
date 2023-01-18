/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { tooltipMixin } from './tooltip_mixin'
import { buttonReset } from '../../lib/css_mixins'

export const root = styled.div`
  background: rgba(15, 28, 45, 0.7);
  backdrop-filter: blur(27.1828px);
  border-radius: 6px;
  color: var(--brave-palette-white);
  font-family: var(--brave-font-heading);
  padding: 8px 20px 14px;

  button {
    font-family: var(--brave-font-heading);
  }
`

export const cardHeader = styled.div`
  margin-top: 6px;
  font-weight: 600;
  font-size: 18px;
  line-height: 22px;
  color: var(--brave-palette-white);
  font-family: var(--brave-font-heading);
  display: flex;
  align-items: center;
  gap: 8px;
  padding-left: 1px;

  .icon {
    flex: 0 0 auto;
    height: 27px;
    width: auto;
  }
`

export const cardHeaderText = styled.div`
  margin-top: 2px;
`

export const unsupportedRegionCard = styled.div`
  margin-top: 24px;
`

export const rewardsOptIn = styled.div`
  margin-top: 11px;
  margin-bottom: 16px;
  color: rgba(255, 255, 255, 0.8);
  font-size: 12px;
  line-height: 18px;
  text-align: center;
  letter-spacing: 0.01em;
`

export const rewardsOptInHeader = styled.div`
  margin-bottom: 12px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
`

export const selectCountry = styled.div`
  margin: 16px -12px 0;
`

export const terms = styled.div`
  margin-top: 14px;
  padding-bottom: 10px;
  font-size: 12px;
  line-height: 18px;
  text-align: center;
  letter-spacing: 0.01em;
  color: rgba(255, 255, 255, 0.8);

  a {
    color: inherit;
    text-decoration: underline;
  }
`

export const disconnected = styled.div`
  margin-top: 20px;
  padding: 16px;
  font-size: 14px;
  line-height: 20px;
  background: linear-gradient(137.04deg, #346FE1 33.4%, #5844C3 82.8%);
  border-radius: 8px;
  cursor: pointer;

  strong {
    font-weight: 600;
  }
`

export const disconnectedArrow = styled.div`
  text-align: right;
  line-height: 15px;

  .icon {
    vertical-align: middle;
    width: 21px;
    height: auto;
  }
`

export const balance = styled.div`
  margin-top: 16px;
  font-size: 12px;
  line-height: 18px;
`

export const balanceTitle = styled.div`
  color: rgba(255, 255, 255, 0.8);
  letter-spacing: 0.01em;
`

export const balanceAmount = styled.div`
  .amount {
    font-size: 36px;
    line-height: 40px;
  }

  .currency {
    font-size: 14px;
    line-height: 21px;
  }
`

export const balanceExchange = styled.div`
  color: rgba(255, 255, 255, 0.8);
  letter-spacing: 0.01em;
  display: flex;
  gap: 6px;
  min-height: 24px;
`

export const balanceExchangeAmount = styled.div`
  flex: 0 0 auto;
`

export const balanceExchangeNote = styled.div`
  flex: 1 1 auto;
`

export const pendingRewards = styled.div`
  color: var(--brave-palette-neutral900);
  font-size: 13px;
  line-height: 19px;

  > div {
    margin-top: 8px;
    background: #E8F4FF;
    box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
    padding: 6px 12px;
    border-radius: 6px;
  }

  .icon {
    display: none;
  }

  a {
    color: var(--brave-palette-blurple500);
    font-weight: 600;
    text-decoration: none;
  }

  .rewards-payment-amount {
    font-weight: 600;

    .plus {
      margin-right: 2px;
    }
  }

  .rewards-payment-completed {
    background: #E7FDEA;
  }

  .rewards-payment-check-status {
    display: block;
    padding-top: 4px;
    font-size: 12px;
    line-height: 18px;
  }
`

export const needsBrowserUpdateView = styled.div`
  display: block;
  align-items: center;
  justify-content: start;
  background: #FDF1F2;
  padding: 5px;
  border-radius: 6px;
`

export const needsBrowserUpdateContentHeader = styled.div`
  margin: 5px;
  font-size: 13px;
  font-weight: 600;
  color: var(--brave-palette-neutral800);
`

export const needsBrowserUpdateContentBody = styled.div`
  margin: 5px;
  font-size: 13px;
  color: var(--brave-palette-neutral800);
`

export const progressHeader = styled.div`
  margin-top: 16px;
  display: flex;
  align-items: center;
  gap: 11px;
  color: rgba(255, 255, 255, 0.8);
`

export const progressHeaderText = styled.div`
  .date-range {
    font-weight: 600;
    color: var(--brave-palette-white);
  }
`

export const progressHeaderBorder = styled.div`
  flex: 1 1 auto;
  border-top: 1px solid rgba(255, 255, 255, 0.3);
  height: 0px;
`

export const dateRange = styled.span`
  font-weight: 600;
  color: var(--brave-palette-white);
`

export const adsOptIn = styled.div`
  margin: 8px 0 15px;
  color: rgba(255, 255, 255, 0.8);
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
`

export const primaryAction = styled.div`
  text-align: center;

  button {
    ${buttonReset}
    font-weight: 600;
    font-size: 12px;
    line-height: 24px;
    cursor: pointer;
    background: var(--brave-palette-blurple400);
    border-radius: 15px;
    padding: 4px 18px;

    &:hover {
      background: var(--brave-palette-blurple500);
    }

    &:active {
      background:
        linear-gradient(rgba(15, 28, 45, .05), rgba(15, 28, 45, .1)),
        var(--brave-palette-blurple500);
    }
  }
`

export const progress = styled.div`
  margin-top: 12px;
  display: flex;
`

export const earning = styled.div`
  flex: 1 1 50%;
`

export const earningInfo = styled.span`
  position: relative;
  margin-left: 4px;

  > .icon {
    height: 14px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 2px;
  }

  .tooltip {
    position: absolute;
    bottom: 100%;
    left: -24px;
    width: 175px;
    padding-bottom: 12px;
    display: none;
  }

  &:hover .tooltip {
    display: initial;
  }
`

export const earningTooltip = styled.div`
  ${tooltipMixin}
  padding: 12px 16px;
  color: #F6F6FA;
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;
`

export const hiddenEarnings = styled.div`
  padding-left: 3px;
  font-weight: 500;
  font-size: 24px;
  line-height: 30px;
  color: rgba(255, 255, 255, 0.66);

  a {
    display: block;
    margin-top: -7px;
    font-weight: 600;
    font-size: 11px;
    line-height: 18px;
    color: #fff;
    text-decoration: none;
  }
`

export const giving = styled.div`
  flex: 1 1 50%;
`

export const progressItemLabel = styled.div`
  color: rgba(255, 255, 255, 0.8);
  letter-spacing: 0.01em;
`

export const progressItemAmount = styled.div`
  font-size: 24px;
  line-height: 36px;

  .currency {
    font-size: 14px;
    line-height: 21px;
  }
`

export const connect = styled.div`
  margin: 16px -12px 0;
  padding: 16px;
  background: linear-gradient(137.04deg, #346FE1 33.4%, #5844C3 82.8%);
  border-radius: 8px;
  font-size: 12px;
  line-height: 18px;
  color: #fff;

  strong {
    font-weight: 600;
  }
`

export const connectAction = styled.div`
  margin-top: 8px;

  button {
    ${buttonReset}
    background: rgba(255, 255, 255, 0.24);
    border-radius: 48px;
    padding: 6px 13px;
    width: 100%;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;

    &:active {
      background: rgba(255, 255, 255, 0.18);
    }

    .icon {
      vertical-align: middle;
      height: 17px;
      width: auto;
      margin-left: 8px;
      margin-top: -2px;
    }
  }
`

export const connectLearnMore = styled.div`
  margin-top: 14px;

  a {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #FFFFFF;
    text-decoration: none;
  }
`

export const publisherSupport = styled.div`
  margin-top: 18px;
  display: flex;
  gap: 8px;
  align-items: center;
  font-size: 12px;
  line-height: 18px;
  color: #F0F2FF;
`

export const publisherCount = styled.div`
  font-size: 32px;
  line-height: 32px;
`

export const settings = styled.div`
  margin-top: 16px;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;

  a {
    color: inherit;
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }
  }

  .icon {
    width: 17px;
    height: auto;
    vertical-align: middle;
    margin-right: 8px;
    margin-bottom: 3px;
  }
`

export const vbatNotice = styled.div`
  margin: 10px -12px 0;
  border-radius: 8px;
  overflow: hidden;

  .vbat-notice-header {
    font-size: 14px;
    line-height: 20px;
    padding: 16px;

    .icon {
      width: 20px;
      height: auto;
    }
  }

  .vbat-notice-close .icon {
    width: 11px;
    height: auto;
  }

  .vbat-notice-content {
    font-size: 12px;
    line-height: 18px;
    padding: 16px;
  }

  .vbat-notice-actions {
    display: block;
    text-align: center;
  }

  .vbat-notice-connect button {
    width: 100%;
    margin-bottom: 10px;
  }
`
