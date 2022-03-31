/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import modalBackground from './assets/opt_in_modal_bg.svg'

export const root = styled.div`
  flex: 0 0 auto;
  min-width: 345px;
  padding: 17px;
  font-family: var(--brave-font-heading);
  text-align: center;
  background-color: var(--brave-palette-white);
  .brave-theme-dark & {
    background-color: #1E2029;
  }
  background-image: url(${modalBackground});
  background-repeat: no-repeat;
  background-position: 4px -11px;
  background-size: auto 200px;
`

export const header = styled.div`
  margin-top: 17px;
  color: var(--brave-palette-black);
  .brave-theme-dark & {
    color: #F0F2FF;
  }
  font-weight: 600;
  font-size: 18px;
  line-height: 22px;

  .icon {
    vertical-align: middle;
    width: 26px;
    margin-right: 5px;
    margin-bottom: 3px;
  }
`

export const text = styled.div`
  margin: 8px 6px 0;
  color: var(--brave-palette-neutral700);
  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }
  font-size: 14px;
  line-height: 24px;
`

export const enable = styled.div`
  margin-top: 20px;
  margin-bottom: 20px;

  button {
    min-width: 290px;
  }
`

export const terms = styled.div`
  margin: 15px 14px 15px;
  color: var(--brave-palette-neutral700);
  font-size: 11px;
  line-height: 16px;
  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }

  a {
    color: var(--brave-color-brandBat);
    font-weight: 600;
    text-decoration: underline;
  }
`

export const learn = styled.div`
  margin: 15px 6px 15px;
  color: var(--brave-palette-neutral700);
  .brave-theme-dark & {
    color: var(--brave-palette-grey400);
  }
  font-size: 14px;
  line-height: 24px;
`

export const tour = styled.span`
  color: var(--brave-color-brandBat);
  cursor: pointer;
  text-decoration: underline;
`
