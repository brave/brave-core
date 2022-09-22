/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import selectArrow from '../assets/select_arrow.svg'

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

export const content = styled.div`
  margin: 0 auto;
  max-width: 1024px;
  display: flex;
  gap: 32px;

  .layout-narrow & {
    width: 100%;
    max-width: 450px;
    min-width: 320px;
    flex-direction: column;
  }
`

export const main = styled.div`
  flex: 0 1 auto;
`

export const sidebar = styled.div`
  flex: 0 0 auto;
  width: 373px;

  .layout-narrow & {
    flex: 0 0 auto;
    order: -1;
    width: 100%;
  }
`

export const title = styled.div`
  border-radius: 4px;
  background-color: #fff;
  padding: 20px 32px;
  margin-bottom: 24px;
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
    border-radius: 0;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    height: auto;
    z-index: 10;
    font-size: 22px;
    line-height: 24px;
    padding: 18px 24px;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    white-space: nowrap;

    .icon {
      height: 26px;
    }
  }
`

export const settingGroup = styled.div``

export const grants = styled.div``

export const rewardsCard = styled.div``

export const promotions = styled.div``
