/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import selectCaret from './assets/select_caret.svg'
import optInIconLight from './assets/optin_icon.svg'
import optInIconDark from './assets/optin_icon_dark.svg'
import geoPinIconLight from './assets/geo_pin_icon.svg'
import geoPinIconDark from './assets/geo_pin_icon_dark.svg'
import successIconLight from './assets/success_icon.svg'
import successIconDark from './assets/success_icon_dark.svg'

import { enableRewardsButton } from './css_mixins'

import * as leo from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  flex: 0 0 auto;
  width: 100%;
  padding: 32px;
  font-family: var(--brave-font-heading);
  text-align: center;
  background-color: ${leo.color.container.background};
`

export const optInIcon = styled.div`
  width: 150px;
  height: 144px;
  margin-left: auto;
  margin-right: auto;
  background-image: url('${optInIconLight}');
  background-position: center;
  background-repeat: no-repeat;
  background-size: cover;

  .brave-theme-dark & {
    background-image: url('${optInIconDark}');
  }
`
export const optInHeader = styled.div`
  margin: 32px 3px 0px 3px;
  color: ${leo.color.text.primary};
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
`

export const optInText = styled.div`
  margin: 8px 3px;
  color: ${leo.color.text.secondary};
  font-size: 14px;
  line-height: 24px;
`

export const learnMore = styled.div`
  margin-top: 8px;
  margin-bottom: 37px;
  text-align: center;

  &.learn-more-success {
    margin-top: 16px;
    margin-bottom: 8px;
  }

  a {
    color: ${leo.color.text.interactive};
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    letter-spacing: 0.03em;
    text-decoration: none;
  }
`

export const terms = styled.div`
  color: ${leo.color.text.tertiary};
  font-size: 11px;
  line-height: 16px;
  margin-bottom: -8px;
  padding: 0 10px;

  a {
    color: ${leo.color.text.interactive};
    text-decoration: none;
  }
`

export const geoPinIcon = styled.div`
  margin-top: 11px;
  width: 276px;
  height: 145px;
  margin-left: auto;
  margin-right: auto;
  background-image: url('${geoPinIconLight}');
  background-position: center;
  background-repeat: no-repeat;
  background-size: cover;

  .brave-theme-dark & {
    background-image: url('${geoPinIconDark}');
  }
`

export const header = styled.div`
  margin: 48px 3px 0px 3px;
  color: ${leo.color.text.primary};
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;

  &.onboarding-result .icon {
    display: block;
    margin: 0 auto 16px;
    height: 24px;
    width: auto;
  }
`

export const text = styled.div`
  margin: 8px 3px 0px 3px;
  color: ${leo.color.text.secondary};
  font-size: 14px;
  line-height: 24px;

  a {
    padding-left: 4px;
    text-decoration: underline;
  }
`

export const successIcon = styled.div`
  display: block;
  margin:  13px auto 90px auto;
  height: 90px;
  width: 315px;
  background-image: url('${successIconLight}');
  background-position: center;
  background-repeat: no-repeat;
  background-size: contain;

  .brave-theme-dark & {
    background-image: url('${successIconDark}');
  }
`

export const selectCountry = styled.div`
  margin-top: 24px;
  padding: 0 72px;

  select {
    -webkit-appearance: none;
    background: url(${selectCaret}) calc(100% - 12px) center no-repeat,
      ${leo.color.container.highlight};
    background-size: 12px;
    width: 100%;
    border-radius: 8px;
    color: ${leo.color.text.primary};
    font-size: 14px;
    line-height: 24px;
    padding: 10px 44px 10px 16px;

    &.empty {
      color: ${leo.color.text.secondary};
    }

    > option {
      color: ${leo.color.text.primary};
      background: ${leo.color.container.background};
    }
  }
`

export const mainAction = styled.div`
  margin-top: 24px;
  margin-bottom: 4px;

  button {
    ${enableRewardsButton}
    font-size: 13px;
    line-height: 20px;
    padding: 12px 24px;

    .icon {
      vertical-align: middle;
      height: 18px;
      width: auto;
    }
  }
`

export const errorCode = styled.div`
  color: ${leo.color.text.secondary};
  font-size: 10px;
  line-height: 15px;
  margin-top: 12px;
  padding-bottom: 7px;
`
