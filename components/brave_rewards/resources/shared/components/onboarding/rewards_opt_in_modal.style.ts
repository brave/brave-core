/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import selectCaret from './assets/select_caret.svg'
import modalBackground from './assets/opt_in_modal_bg.svg'

import { enableRewardsButton } from './css_mixins'

export const root = styled.div`
  flex: 0 0 auto;
  width: 335px;
  padding: 17px;
  font-family: var(--brave-font-heading);
  text-align: center;
  background-color: var(--brave-palette-white);
  background-image: url(${modalBackground});
  background-repeat: no-repeat;
  background-position: 4px -11px;
  background-size: auto 200px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 16px;
`

export const header = styled.div`
  margin-top: 17px;
  color: var(--brave-palette-black);
  font-weight: 600;
  font-size: 18px;
  line-height: 26px;

  .icon {
    vertical-align: middle;
    width: 26px;
    margin-right: 5px;
    margin-bottom: 3px;
  }

  &.country-select .icon {
    color: var(--brave-palette-neutral900);
    height: 20px;
    width: auto;
    margin-right: 6px;
  }

  &.onboarding-result .icon {
    display: block;
    margin: 0 auto 16px;
    height: 24px;
    width: auto;
  }
`

export const text = styled.div`
  margin-top: 8px;
  padding: 0 23px;
  color: var(--brave-palette-neutral700);
  font-size: 14px;
  line-height: 23px;

  a {
    padding-left: 4px;
    text-decoration: underline;
  }

  strong {
    font-weight: 600;
  }
`

export const selectCountry = styled.div`
  margin-top: 16px;
  padding: 0 23px;

  select {
    -webkit-appearance: none;
    background: url(${selectCaret}) calc(100% - 12px) center no-repeat, #fff;
    background-size: 12px;
    width: 100%;
    border-radius: 4px;
    border: 1px solid var(--brave-palette-grey500);
    color: var(--brave-palette-neutral900);
    font-size: 12px;
    line-height: 18px;
    padding: 7px 36px 7px 12px;

    &.empty {
      color: var(--brave-palette-neutral600);
    }
  }
`

export const mainAction = styled.div`
  margin-top: 24px;
  padding: 0 23px;

  button {
    ${enableRewardsButton}
    width: 100%;
    font-size: 13px;
    line-height: 20px;

    .icon {
      vertical-align: middle;
      height: 18px;
      width: auto;
    }
  }
`

export const takeTour = styled.div`
  margin-top: 18px;
  margin-bottom: 42px;
  color: var(--brave-color-brandBat);

  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    border: 0;
    background: 0;
    margin: 0;
    padding: 0;
    cursor: pointer;
  }
`

export const terms = styled.div`
  margin: 32px 0 15px;
  color: var(--brave-palette-neutral600);
  font-size: 12px;
  line-height: 16px;

  a {
    color: var(--brave-color-brandBat);
    font-weight: 600;
    text-decoration: none;
  }
`

export const errorCode = styled.div`
  color: var(--brave-palette-neutral600);
  font-size: 10px;
  line-height: 15px;
  margin-top: 12px;
  padding-bottom: 7px;
`
