/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import greenCheckIcon from '../assets/green_check.svg'
import connectWalletImage from '../assets/connect_wallet.svg'
import connectWalletNarrowImage from '../assets/connect_wallet_narrow.svg'

export const root = styled.div`
  position: relative;
  font-family: var(--brave-font-heading);
  max-width: 880px;
  min-width: 820px;
  min-height: 545px;
  margin: 0 10px;
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  border-radius: 16px;
  display: flex;
  overflow: hidden;

  a {
    color: var(--brave-color-brandBat);
    text-decoration: underline;
  }

  .layout-narrow & {
    display: block;
    max-width: 375px;
    min-width: unset;
    min-height: unset;
    border-radius: 8px;
  }
`

export const close = styled.div`
  position: absolute;
  top: 28px;
  right: 28px;

  .layout-narrow & {
    top: 16px;
    right: 16px;
  }
`

export const leftPanel = styled.div`
  flex: 0 1 448px;
  padding: 64px;

  .layout-narrow & {
    padding: 40px 24px 24px;
  }
`

export const rightPanel = styled.div`
  flex: 1 1 430px;
  background: var(--brave-palette-neutral000);

  .layout-narrow & {
    background: var(--brave-palette-white);
  }
`

export const panelHeader = styled.div`
  font-weight: 600;
  font-size: 20px;
  line-height: 30px;
  color: var(--brave-palette-black);

  .layout-narrow & {
    font-size: 18px;
    line-height: 26px;
  }
`

export const panelText = styled.div`
  margin-top: 8px;
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral700);
`

export const infoPanel = styled.div``

export const infoListItem = styled.div`
  margin-top: 24px;
  color: var(--brave-palette-black);
  font-weight: 600;
  font-size: 14px;
  line-height: 20px;
  background: top 2px left no-repeat url(/${greenCheckIcon});
  padding-left: 23px;
`

export const continueButton = styled.div`
  margin-top: 33px;

  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    cursor: pointer;
    border: none;
    outline-style: none;

    color: var(--brave-palette-white);
    background: var(--brave-color-brandBat);
    padding: 10px 22px;
    border-radius: 48px;

    .icon {
      height: 15px;
      width: auto;
      vertical-align: middle;
      margin-top: -1px;
      margin-left: 9px;
    }

    &:focus-visible {
      outline-style: auto;
    }

    &:active {
      background: var(--brave-color-brandBatActive);
    }

    .layout-narrow & {
      width: 100%;
    }
  }
`

export const infoNote = styled.div`
  color: var(--brave-palette-neutral600);
  font-size: 12px;
  line-height: 18px;
  margin-top: 16px;
`

export const connectGraphic = styled.div`
  text-align: center;
  margin-top: 65px;
  height: 419px;
  background: center no-repeat url(/${connectWalletImage});
  background-size: cover;

  .layout-narrow & {
    margin-top: 24px;
    height: 170px;
    background-image: url(/${connectWalletNarrowImage});
  }
`

export const selectWalletLeftPanel = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const selectWalletContent = styled.div`
  flex: 1 0 auto;
`

export const selectWalletNote = styled.div`
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral600);
`

export const providerButtons = styled.div`
  padding: 20px 40px 40px;
  height: 100%;
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 16px;

  button {
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

    &.disabled {
      background: var(--brave-palette-neutral000);
    }
  }

  .layout-narrow & {
    padding: 0 24px 24px;
  }
`

export const providerButtonGrid = styled.div`
  display: grid;
  width: 100%;

  grid-template-areas: "icon name caret";
  grid-template-columns: auto 1fr auto;

  .disabled & {
    grid-template-areas:
      "icon name"
      "icon message";
    grid-template-columns: auto 1fr;
    grid-template-rows: auto;
  }
`

export const providerButtonIcon = styled.div`
  align-self: center;
  display: flex;
  height: 52px;
  width: 52px;

  background: var(--brave-palette-neutral000);

  grid-area: icon;

  border-radius: 50%;

  .icon {
    height: 25px;
    margin: auto;
  }

  .disabled & {
    background: var(--brave-palette-white);

    --provider-icon-color: var(--brave-palette-neutral600);
  }
`

export const providerButtonName = styled.div`
  align-items: center;
  display: flex;
  padding-left: 14px;

  font-size: 16px;
  font-weight: 600;
  line-height: normal;
  text-align: left;

  color: var(--brave-palette-black);

  grid-area: name;

  .disabled & {
    align-items: end;

    color: var(--brave-palette-neutral700);
  }
`

export const providerButtonMessage = styled.div`
  align-items: start;
  display: flex;
  padding-left: 14px;

  font-size: 11px;
  font-weight: 400;
  line-height: normal;
  text-align: left;

  color: var(--brave-palette-neutral700);

  grid-area: message;
`

export const providerButtonCaret = styled.div`
  display: flex;

  grid-area: caret;

  .icon {
    height: 16px;
    margin: auto;

    fill: var(--brave-palette-blurple500);
    stroke: var(--brave-palette-blurple500);
  }
`

export const learnMoreLink = styled.div``
