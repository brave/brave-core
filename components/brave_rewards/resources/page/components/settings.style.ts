/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import selectArrow from '../assets/select_arrow.svg'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  font-family: Poppins;
  min-height: 100vh;
  background: #f2f4f7;
  padding: 48px 20px;

  .layout-narrow & {
    padding: 80px 10px 24px;
  }

  select {
    -webkit-appearance: none;
    background: url(/${selectArrow}) calc(100% - 12px) center no-repeat;
    background-size: 12px;
    width: 100%;
    border-radius: 4px;
    border: 1px solid #DFDFE8;
    color: #686978;
    font-size: 14px;
    padding: 8px 41px 8px 13px;
  }

  /* Use :where to reduce selector specificity and allow overriding. */
  & :where(a) {
    color: #4C54D2;
  }
`

export const onboarding = styled.div`
  margin: 0 auto;
  width: 600px;

  .layout-narrow & {
    width: 100%;
    max-width: 600px;
    min-width: 320px;
    margin-top: -20px;
  }
`

export const unsupportedRegionNoticeTitle = styled.div`
  display: none;

  .layout-narrow & {
    display: block;
  }
`

export const unsupportedRegionNotice = styled.div`
  margin: 0 auto;
  width: 600px;
  background-color: var(--brave-palette-white);
  background-repeat: no-repeat;
  background-position: 0 0;
  background-size: auto 220px;
  border-radius: 16px;
  padding: 64px 16px 313px 16px;

  .layout-narrow & {
    width: 100%;
    max-width: 600px;
    min-width: 320px;
    margin-top: -12px;
    margin-left: auto;
    margin-right: auto;
    padding-bottom: 99px;
    border-radius: 8px;
  }
`

export const content = styled.div`
  margin: 0 auto;
  max-width: 1024px;
  display: flex;
  justify-content: center;
  gap: 32px;

  .layout-narrow & {
    width: 100%;
    max-width: 450px;
    min-width: 320px;
    flex-direction: column;
  }
`

export const main = styled.div`
  flex: 0 1 620px;
`

export const sidebar = styled.div`
  flex: 0 0 373px;
  display: flex;
  flex-direction: column;
  gap: 32px;

  &:empty {
    display: none;
  }

  .layout-narrow & {
    flex: 0 0 auto;
    order: -1;
    width: 100%;
  }
`

export const header = styled.div`
  border-radius: 6px;
  box-shadow: 0px 0px 1px rgba(0, 0, 0, 0.11),
              0px 0.5px 1.5px rgba(0, 0, 0, 0.1);
  background-color: #fff;
  padding: 20px 32px;
  margin-bottom: 24px;
  display: flex;
  align-items: center;

  .layout-narrow & {
    border-radius: 0;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    height: auto;
    z-index: 10;
    padding: 18px 24px;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    white-space: nowrap;
  }
`

export const title = styled.div`
  flex: 1 1 auto;
  font-size: 28px;
  font-weight: 600;
  line-height: 48px;

  .icon {
    vertical-align: middle;
    width: auto;
    height: 48px;
    margin-right: 8px;
    margin-top: -4px;
  }

  .layout-narrow & {
    font-size: 22px;
    line-height: 24px;

    .icon {
      height: 26px;
    }
  }
`

export const manageAction = styled.div`
  margin-top: 4px;

  button {
    ${mixins.buttonReset}
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;
    color: #4C54D2;

    .icon {
      height: 18px;
      width: auto;
      vertical-align: middle;
      margin-top: -3px;
      margin-right: 8px;
    }
  }
`

export const settingGroup = styled.div``

export const vbatNotice = styled.div`
  margin-bottom: 32px;
  background: #fff;
  box-shadow:
    0px 0px 1px rgba(0, 0, 0, 0.11),
    0px 0.5px 1.5px rgba(0, 0, 0, 0.1);
  border-radius: 8px;
  overflow: hidden;
`
