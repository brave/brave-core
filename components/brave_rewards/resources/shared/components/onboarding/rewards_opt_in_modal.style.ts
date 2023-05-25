/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import selectCaret from './assets/select_caret.svg'

import { enableRewardsButton } from './css_mixins'

import * as leo from '@brave/leo/tokens/css'

export const root = styled.div`
  flex: 0 0 auto;
  width: 100%;
  height: calc(100% + 29px);
  padding: 32px;
  font-family: var(--brave-font-heading);
  text-align: center;
  background-color: var(--brave-palette-white);
  border-radius: 16px;
`

export const optInIcon = styled.div`
  .icon {
    width: 150px;
    height: 144px;
  }
`
export const optInHeader = styled.div`
  margin-top: 36px;
  color: ${leo.color.light.text.primary};
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
`

export const optInText = styled.div`
  margin-top: 8px;
  color: ${leo.color.light.text.secondary};
  font-size: 14px;
  line-height: 24px;
`

export const learnMore = styled.div`
  margin-top: 12px;
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

export const geoPinIcon = styled.div`
  margin-top: 11px;

  .icon {
    width: 276px;
    height: 145px;
  }
`

export const header = styled.div`
  margin-top: 48px;
  color: ${leo.color.light.text.primary};
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;

  &.onboarding-success .icon {
    display: block;
    margin:  13px auto 90px auto;
    height: 96px;
    width: auto;
  }

  &.onboarding-error .icon {
    display: block;
    margin: 0 auto 16px;
    height: 24px;
    width: auto;
  }
`

export const text = styled.div`
  margin-top: 8px;
  color: ${leo.color.light.text.secondary};
  font-size: 14px;
  line-height: 24px;

  a {
    padding-left: 4px;
    text-decoration: underline;
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
    color: ${leo.color.light.text.primary};
    font-size: 14px;
    line-height: 24px;
    padding: 10px 44px 10px 16px;

    &.empty {
      color: ${leo.color.light.text.secondary};
    }

    > option {
      color: ${leo.color.light.text.primary};
      background: ${leo.color.container.background};
    }
  }
`

export const mainAction = styled.div`
  margin-top: 32px;

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
  color: var(--brave-palette-neutral600);
  font-size: 10px;
  line-height: 15px;
  margin-top: 12px;
  padding-bottom: 7px;
`
