/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'

import { buttonReset } from '../../lib/css_mixins'
import selfCustodyImage from '../../assets/self_custody_invitation.svg'

export const root = styled.div.attrs({ 'data-theme': 'dark' })`
  font: ${font.default.regular};
  color: ${color.white};

  --self-info-gradient-background:
      linear-gradient(96.98deg, #4641EE 0%, #6261FF 100%);
`

export const cardHeader = styled.div`
  margin-bottom: 24px;
  font: ${font.heading.h4};
  display: flex;
  align-items: center;
  gap: 8px;
`

export const cardHeaderIcon = styled.div`
  height: 24px;

  .icon {
    height: 24px;
    width: auto;
  }
`

export const optInIcon = styled.div`
  display: flex;
  justify-content: center;

  .icon {
    height: 126px;
    width: 130px;
  }
`

export const optInHeaderText = styled.div`
  text-align: center;
  font: ${font.heading.h4};
`

export const optInText = styled.div`
  margin-top: 8px;
  text-align: center;
  font: ${font.small.regular};
`

export const optInAction = styled.div`
  margin-top: 10px;
  display: flex;
  width: 100%;
  justify-content: center;
`

export const selectCountry = styled.div`
  margin: 0 -12px;
`

export const disconnected = styled.div`
  margin: 0 -12px;
  padding: 16px;
  font-size: 14px;
  line-height: 20px;
  background: var(--self-info-gradient-background);
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
  &.flat {
    margin-bottom: -10px;
  }
`

export const balanceTitle = styled.div`
  font: ${font.components.buttonSmall};
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
  margin: 4px 0;
  display: flex;
  align-items: center;
  gap: 4px;
  font-family: Poppins;

  .amount {
    font-size: 36px;
    font-weight: 400;
    line-height: 36px;
  }

  .currency {
    font-size: 18px;
    font-weight: 500;
    line-height: 28px;
    letter-spacing: 0.02em;
    align-self: flex-end;
  }
`

export const balanceExchange = styled.div`
  margin-top: 4px;
  font: ${font.small.semibold};
`

export const pendingRewards = styled.div.attrs({ 'data-theme': 'light' })`
  margin-top: 16px;
  color: ${color.text.secondary};
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
    color: ${color.icon.interactive};
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
  margin: 0 -12px;
  padding: 16px;
  background: var(--self-info-gradient-background);
  border-radius: 8px;
  font: ${font.small.semibold};
`

export const needsBrowserUpdateContentBody = styled.div`
  margin-top: 8px;
  font-weight: 400;
`

export const earnings = styled.div`
  margin: 24px 0 8px;
  display: flex;
  justify-content: space-between;
  align-items: center;
`

export const earningsHeader = styled.div`
  display: flex;
  align-items: center;
  gap: 5px;
`

export const earningsHeaderText = styled.div`
  font: ${font.small.semibold};
`

export const earningsInfo = styled.span`
  position: relative;
  display: none;

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
    left: -98px;
    width: 207px;
    padding-bottom: 8px;
    visibility: hidden;
    transition: visibility 0s linear 300ms;
  }

  &:hover .tooltip {
    visibility: initial;
  }
`

export const earningsTooltip = styled.div.attrs({ 'data-theme': 'light' })`
  position: relative;
  padding: 16px;
  background: ${color.white};
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 8px;
  color: ${color.text.primary};
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;

  &:before {
    content: '';
    position: absolute;
    bottom: -3px;
    left: 102px;
    background: inherit;
    height: 15px;
    width: 15px;
    transform: rotate(45deg);
  }
`

export const manageAds = styled.div.attrs({ 'data-theme': 'light' })`
  margin-top: 14px;

  a {
    color: ${color.text.interactive};
    font-weight: 600;
    font-size: 12px;
    line-height: 16px;
    text-decoration: none;
  }

  .icon {
    height: 9px;
    width: auto;
    color: ${color.icon.interactive};
  }
`

export const earningsDisplay = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  font-family: Poppins;
  font-size: 16px;
  font-weight: 400;
  line-height: 24px;

  .currency {
    font: ${font.small.semibold};
  }
`

export const earningsMonth = styled.div`
  display: none;
  padding: 5px 6px;
  font: ${font.components.label};
  background: rgba(255, 255, 255, 0.10);
  border-radius: 4px;
  text-transform: uppercase;
`

export const connect = styled.div`
  margin: 0 -12px;
  padding: 16px;
  background: var(--self-info-gradient-background);
  border-radius: 8px;
  font: ${font.small.semibold};
  font-weight: 400;

  strong {
    display: block;
    font-weight: 600;
  }
`

export const connectAction = styled.div`
  margin-top: 8px;
  display: flex;
  justify-content: stretch;
`

export const publisherSupport = styled.div`
  margin-top: 16px;
  display: flex;
  gap: 8px;
  align-items: center;
  font: ${font.small.regular};
`

export const publisherCount = styled.div`
  font: ${font.heading.h1};
`

export const selfCustodyInvite = styled.div.attrs({ 'data-theme': 'light' })`
  margin: 0 -12px;
  border-radius: 12px;
  background: no-repeat center 16px/auto 67px url(${selfCustodyImage}),
              var(--self-info-gradient-background);
  padding: 91px 16px 16px;
  position: relative;
  display: flex;
  flex-direction: column;
  gap: 8px;

  color: ${color.container.background};
  text-align: center;
`

export const selfCustodyInviteClose = styled.div`
  --leo-icon-size: 18px;
  position: absolute;
  inset-block-start: 12px;
  inset-inline-end: 12px;
  button {
    ${buttonReset}
    cursor: pointer;
  }
`

export const selfCustodyInviteHeader = styled.div`
  font: ${font.default.semibold};
`

export const selfCustodyInviteText = styled.div`
  font: ${font.small.regular};
  padding: 0 16px;
  margin-bottom: -4px;
`

export const selfCustodyInviteDismiss = styled.div`
  button {
    ${buttonReset}
    cursor: pointer;
    padding: 10px 0;
    opacity: 0.85;
    font: ${font.components.buttonSmall};
  }
`

export const tosUpdateNotice = styled.div.attrs({ 'data-theme': 'light' })`
  margin: 0 -12px;

  --tos-update-notice-border-radius: 8px;
  --tos-update-notice-padding: 16px;
  --tos-update-heading-padding-bottom: 0;
  --tos-update-notice-heading-font: ${font.heading.h4};
  --tos-update-notice-text-font: ${font.small.link};
`;

