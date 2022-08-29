/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import warningCircleImage from '../../shared/assets/warning_circle.svg'
import greenCheckIcon from '../assets/green_check.svg'

export const root = styled.div`
  position: relative;
  font-family: var(--brave-font-heading);
  max-width: 880px;
  min-width: 820px;
  min-height: 545px;
  margin: 0 10px;
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 8px;
  display: flex;
  overflow: hidden;

  a {
    color: var(--brave-color-brandBat);
    text-decoration: none;
  }

  .layout-narrow & {
    display: block;
    max-width: 400px;
    min-width: unset;
    min-height: unset;
  }
`

export const close = styled.div`
  position: absolute;
  top: 32px;
  right: 46px;

  .layout-narrow & {
    top: 28px;
    right: 28px;
  }
`

export const leftPanel = styled.div`
  flex: 0 1 448px;
  padding: 70px 28px 31px 80px;

  .layout-narrow & {
    padding: 60px 32px 32px;
  }
`

export const rightPanel = styled.div`
  flex: 1 1 auto;
  padding: 64px 42px 32px;
  background: var(--brave-palette-neutral000);

  .layout-narrow & {
    padding: 0;
  }
`

export const panelHeader = styled.div`
  font-weight: 600;
  font-size: 22px;
  line-height: 28px;
  color: var(--brave-palette-black);
`

export const panelText = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: var(--brave-palette-neutral700);
  margin-top: 8px;
`

export const infoPanel = styled.div``

export const infoListItem = styled.div`
  color: var(--brave-palette-neutral900);
  font-weight: 600;
  margin-top: 16px;
  background: top 4px left no-repeat url("${greenCheckIcon}");
  padding-left: 23px;
`

export const infoNote = styled.div`
  color: var(--brave-palette-neutral600);
  font-size: 12px;
  line-height: 18px;
  margin-top: 16px;
`

export const continueButton = styled.div`
  margin-top: 16px;

  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 19px;
    cursor: pointer;
    border: none;
    outline-style: none;

    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);
    padding: 10px 39px;
    border-radius: 20px;

    .icon {
      height: 12px;
      vertical-align: middle;
      margin: 0 -6px 2px 2px;
    }

    &:focus-visible {
      outline-style: auto;
    }

    &:active {
      background: var(--brave-color-brandBatActive);
    }
  }
`

export const infoTerms = styled.div`
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral600);
  margin-top: 38px;

  strong {
    font-weight: 600;
  }
`

export const connectGraphic = styled.div`
  text-align: center;
  margin: 86px auto 0;
  width: 250px;

  .layout-narrow & {
    display: none;
  }
`

export const selectWalletLeftPanel = styled.div`
  margin-top: 30px;

  .layout-narrow & {
    margin-top: 0;
  }
`

export const selectWalletContent = styled.div`
  min-height: 236px;

  .layout-narrow & {
    min-height: unset;
  }
`

export const selectWalletNote = styled.div`
  margin-top: 48px;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral600);

  .layout-narrow & {
    display: none;
  }
`

export const providerButtons = styled.div`
  min-height: 72%;
  padding-top: 40px;
  display: flex;
  flex-direction: column;
  justify-content: center;

  button {
    margin-bottom: 17px;
    margin-left: 21px;
    height: 79px;
    display: flex;
    align-items: center;
    background: var(--brave-palette-white);
    border: 2px solid var(--brave-palette-grey200);
    border-radius: 8px;
    font-weight: 600;
    font-size: 16px;
    line-height: 28px;
    color: var(--brave-palette-black);
    padding: 14px 20px;
    cursor: pointer;

    &:active {
      border-color: var(--brave-palette-grey300);
    }

    &.selected {
      border-color: var(--brave-color-brandBat);
    }
  }

  .layout-narrow & {
    padding: 32px;

    button {
      margin-left: 0;
    }
  }
`

export const providerButtonIcon = styled.div`
  flex: 0 0 52px;
  background: rgb(218, 220, 232, .33);
  border-radius: 50%;
  height: 52px;
  width: 52px;
  padding: 14px 0;

  .icon {
    height: 25px;
    width: auto;
  }
`

export const providerButtonName = styled.div`
  flex: 1 1 auto;
  padding-left: 30px;
  text-align: left;
`

export const providerButtonCaret = styled.div`
  flex: 0 0 10px;

  .icon {
    height: 16px;
    width: auto;
    vertical-align: middle;
    margin-bottom: 3px;
  }
`
