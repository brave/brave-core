/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import warningCircleImage from '../../shared/assets/warning_circle.svg'

export const root = styled.div`
  font-family: var(--brave-font-heading);
  width: 880px;
  min-height: 545px;
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.2);
  border-radius: 8px;
  display: flex;
  overflow: hidden;

  a {
    color: var(--brave-color-brandBat);
    text-decoration: none;
  }
`

export const leftPanel = styled.div`
  flex: 1 1 50%;
  padding: 70px 35px 31px 80px;
`

export const rightPanel = styled.div`
  flex: 1 1 50%;
  padding: 32px 42px;
  background: var(--brave-palette-neutral000);
`

export const panelHeader = styled.div`
  font-weight: 600;
  font-size: 22px;
  line-height: 28px;
  color: var(--brave-palette-black);
`

export const panelText = styled.div`
  font-size: 16px;
  line-height: 24px;
  color: var(--brave-palette-neutral700);
  margin-top: 8px;

  em {
    font-style: normal;
    color: #953088;
  }
`

export const infoPanel = styled.div``

export const acceptTerms = styled.div`
  margin-top: 24px;
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral900);
  display: flex;
  align-items: flex-start;
`

export const acceptTermsCheckbox = styled.div`
  flex: 0 1 auto;

  button {
    background: var(--brave-palette-white);
    border: 1px solid var(--brave-palette-grey500);
    border-radius: 4px;
    height: 22px;
    width: 22px;
    overflow: hidden;
    padding: 0;
    text-align: center;

    .icon {
      width: 18px;
      height: auto;
      margin-top: 1px;
    }
  }

  &.needs-check button {
    border-color: var(--brave-color-warn);
  }
`

export const acceptTermsLabel = styled.div`
  padding-left: 12px;
  flex: 1 1 auto;
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
  margin-top: 20px;
`

export const alreadyConnected = styled.div`
  position: relative;
  top: 0;
  left: -20px;
  margin-top: 54px;
  text-align: center;
  font-weight: 600;
  font-size: 14px;
  line-height: 22px;
  color: var(--brave-palette-black);

  .login {
    font-weight: normal;
  }
`

export const connectGraphic = styled.div`
  text-align: center;
  margin-top: 110px;
  margin-left: -40px;
  margin-right: -40px;
`

export const minimumBalanceWarning = styled.div`
  margin-top: 48px;
  background: var(--brave-palette-neutral000);
  border-radius: 8px;
  padding: 16px 17px 26px 41px;
  font-size: 14px;
  line-height: 22px;

  background-repeat: no-repeat;
  background-position: 17px 21px;
  background-size: 16px 16px;
  background-image: url('${warningCircleImage}');

  .login {
    margin-top: 8px;
    font-weight: 600;
  }
`

export const selectWalletLeftPanel = styled.div`
  margin-top: 30px;
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
