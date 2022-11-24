/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

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
    text-decoration: underline;
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
  background: top 4px left no-repeat url(/${greenCheckIcon});
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

    &.disabled {
      background: var(--brave-palette-neutral000);
    }
  }

  .layout-narrow & {
    padding: 32px;

    button {
      margin-left: 0;
    }
  }
`

export const grid = styled.div`
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

export const learnMoreLink = styled.div`
  margin-left: 21px;

  .layout-narrow & {
    margin-left: 0;
  }
`
