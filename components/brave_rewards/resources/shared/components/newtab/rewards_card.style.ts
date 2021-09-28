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

export const grant = styled.div`
  margin: 16px -4px 0;
  background: #17171F;
  border-radius: 8px;
  padding: 10px 12px 16px;
`

export const grantHeader = styled.div`
  display: flex;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: var(--brave-palette-grey000);

  > :first-child {
    flex: 1 1 auto;

    .icon {
      color: var(--brave-palette-yellow500);
      height: 16px;
      width: auto;
      vertical-align: middle;
      margin-right: 9px;
      margin-bottom: 2px;
    }
  }

  button {
    ${buttonReset}
    color: var(--brave-palette-grey400);
    cursor: pointer;

    .icon {
      height: 11px;
      width: auto;
    }

    &:hover {
      color: var(--brave-palette-grey200);
    }
  }
`

export const grantText = styled.div`
  margin-top: 8px;
  margin-bottom: 12px;
  font-size: 13px;
  line-height: 16px;
  color: var(--brave-palette-grey400);
  display: flex;
  gap: 10px;
  align-items: flex-end;

  .amount, .currency {
    font-weight: 600;
  }
`

export const grantDate = styled.div`
  white-space: nowrap;
  font-size: 11px;
  color: var(--brave-palette-grey000);
  opacity: 0.8;
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
  margin-top: 8px;
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  padding: 6px 12px;
  border-radius: 6px;
  color: var(--brave-palette-neutral900);
  font-size: 13px;
  line-height: 18px;

  strong, .amount {
    font-weight: 600;
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
