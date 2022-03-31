/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

export const settings = styled.div`
  margin-top: 10px;
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
