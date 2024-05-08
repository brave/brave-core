/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as leo from '@brave/leo/tokens/css/variables'

import { buttonReset } from '../../lib/css_mixins'
import selfCustodyImage from '../../assets/self_custody_invitation.svg'

export const root = styled.div.attrs({
  'data-theme': 'dark'
})`
  background: rgba(24, 25, 30, 0.6);
  backdrop-filter: blur(27.5px);
  border-radius: 6px;
  color: ${leo.color.text.primary};
  font-family: Poppins;
  padding: 8px 20px 14px;

  button {
    font-family: Poppins;
  }
`

export const cardHeader = styled.div`
  margin-top: 6px;
  font-weight: 600;
  font-size: 18px;
  line-height: 22px;
  color: ${leo.color.white};
  font-family: Poppins;
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

export const optInIcon = styled.div`
  margin-top: 14px;
  margin-left: auto;
  margin-right: auto;
  width: 60%;
  .icon {
    margin-left: auto;
    margin-right: auto;
    height: 126px;
    width: 130px;
  }
`

export const optInHeaderText = styled.div`
  color: ${leo.color.white};
  text-align: center;
  font-weight: 600;
  font-size: 16px;
  line-height: 28px;
`

export const optInText = styled.div`
  margin-top: 10px;
  text-align: center;
  font-size: 12px;
  line-height: 18px;
`

export const optInAction = styled.div`
  margin-top: 10px;
  display: flex;
  width: 100%;
  justify-content: center;

  button {
    ${buttonReset}
    color: ${leo.color.white};
    background: ${leo.color.button.background};
    border-radius: 48px;
    padding: 12px 24px;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
    cursor: pointer;
  }
`

export const optInLearnMore = styled.div.attrs({
  'data-theme': 'dark'
})`
  margin-top: 12px;
  margin-bottom: 20px;
  text-align: center;

  a {
    color: ${leo.color.text.interactive};
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
    text-decoration: none;
  }
`

export const optInTerms = styled.div.attrs({
  'data-theme': 'dark'
})`
  color: ${leo.color.text.tertiary};
  text-align: center;
  font-size: 11px;
  line-height: 16px;

  a {
    color: ${leo.color.text.interactive};
    text-decoration: none;
  }
`

export const selectCountry = styled.div`
  margin: 16px -12px 0;
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

  &.flat {
    margin-bottom: -10px;
  }
`

export const balanceTitle = styled.div.attrs({
  'data-theme': 'dark'
})`
  color: ${leo.color.text.disabled};
`

export const balanceSpinner = styled.div`
  .icon {
    height: 36px;
    vertical-align: middle;
    margin-right: 4px;
  }
`

export const loading = styled.span`
  font-size: 12px;
  line-height: 18px;
  vertical-align: middle;
`

export const balanceAmount = styled.div`
  display: flex;
  align-items: stretch;
  gap: 4px;

  .amount {
    font-size: 36px;
    line-height: 54px;
  }

  .currency {
    font-size: 16px;
    line-height: 21px;
    margin-top: 20px;
  }
`

export const balanceExchange = styled.div.attrs({
  'data-theme': 'dark'
})`
  margin-top: 4px;
  color: ${leo.color.text.disabled};
  display: flex;
  gap: 6px;
`

export const balanceExchangeAmount = styled.div`
  flex: 0 0 auto;
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

  .rewards-payment-completed {
    background: #E7FDEA;
  }

  .rewards-payment-link {
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

export const earningsHeader = styled.div`
  margin: 24px 0 8px;
  display: flex;
  align-items: center;
  gap: 5px;
`

export const earningsHeaderText = styled.div`
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
`

export const earningsInfo = styled.span`
  position: relative;

  .icon {
    height: 12px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 2px;
    margin-left: 5px;
  }

  .tooltip {
    position: absolute;
    bottom: 100%;
    left: -119px;
    width: 207px;
    padding-bottom: 8px;
    visibility: hidden;
    transition: visibility 0s linear 300ms;
  }

  &:hover .tooltip {
    visibility: initial;
  }
`

export const earningsTooltip = styled.div.attrs({
  'data-theme': 'light'
})`
  position: relative;
  padding: 16px;
  background: ${leo.color.white};
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 8px;
  color: ${leo.color.text.primary};
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;

  &:before {
    content: '';
    position: absolute;
    bottom: -3px;
    left: 123px;
    background: inherit;
    height: 15px;
    width: 15px;
    transform: rotate(45deg);
  }
`

export const manageAds = styled.div.attrs({
  'data-theme': 'light'
})`
  margin-top: 14px;

  a {
    color: ${leo.color.text.interactive};
    font-weight: 600;
    font-size: 12px;
    line-height: 16px;
    text-decoration: none;
  }

  .icon {
    height: 9px;
    width: auto;
    color: ${leo.color.icon.interactive};
  }
`

export const earningsHeaderBorder = styled.div.attrs({
  'data-theme': 'dark'
})`
  flex: 1 1 auto;
  border-top: 1px solid ${leo.color.text.disabled};
  height: 0px;
`

export const earningsDisplay = styled.div`
  display: flex;
  align-items: center;
  gap: 5px;
  font-size: 14px;
  line-height: 24px;
  color: #F8F9FA;
`

export const earningsMonth = styled.div`
  padding: 2px 4px;
  font-size: 11px;
  line-height: 16px;
  color: rgba(255, 255, 255, 0.65);
  background: rgba(255, 255, 255, 0.15);
  border-radius: 4px;
`

export const hiddenEarnings = styled.div.attrs({
  'data-theme': 'dark'
})`
  display: flex;
  align-items: center;

  padding-left: 3px;
  font-weight: 500;
  font-size: 24px;
  line-height: 30px;
  color: ${leo.color.text.disabled};

  a {
    font-weight: 600;
    font-size: 11px;
    line-height: 18px;
    color: ${leo.color.text.primary};
    text-decoration: none;
  }
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

export const selfCustodyInvite = styled.div.attrs({
  'data-theme': 'light'
})`
  margin: 12px -12px -6px;
  border-radius: 12px;
  background: no-repeat center 16px/auto 67px url(${selfCustodyImage}),
              linear-gradient(137deg, #346FE1 33.4%, #5844C3 82.8%);
  padding: 91px 16px 16px;
  position: relative;
  display: flex;
  flex-direction: column;
  gap: 8px;

  color: ${leo.color.container.background};
  text-align: center;
`

export const selfCustodyInviteClose = styled.div`
  --leo-icon-size: 18px;
  position: absolute;
  top: 12px;
  right: 12px;

  button {
    ${buttonReset}
    cursor: pointer;
  }
`

export const selfCustodyInviteHeader = styled.div`
  font: ${leo.font.default.semibold};
`

export const selfCustodyInviteText = styled.div`
  font: ${leo.font.small.regular};
  padding: 0 16px;
  margin-bottom: -4px;
`

export const selfCustodyInviteDismiss = styled.div`
  button {
    ${buttonReset}
    cursor: pointer;
    padding: 10px 0;
    opacity: 0.85;
    font: ${leo.font.components.buttonSmall};
  }
`

export const tosUpdateNotice = styled.div.attrs({
  'data-theme': 'light'
})`
  margin: 8px -10px 0;

  --tos-update-notice-border-radius: 8px;
  --tos-update-notice-padding: 16px;
  --tos-update-heading-padding-bottom: 0;
  --tos-update-notice-heading-font: ${leo.font.heading.h4};
  --tos-update-notice-text-font: ${leo.font.small.link};
`;

