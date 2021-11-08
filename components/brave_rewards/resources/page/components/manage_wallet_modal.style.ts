/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: relative;
  font-family: var(--brave-font-heading);
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral700);
  background: var(--brave-palette-white);
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 6px;
  width: 664px;
  padding: 40px 0;

  strong {
    font-weight: 600;
  }

  a {
    font-weight: 600;
    color: var(--brave-palette-blurple500);
    text-decoration: none;
    cursor: pointer;
  }
`

export const close = styled.div`
  position: absolute;
  top: 15px;
  right: 15px;
`

export const header = styled.div`
  font-weight: 500;
  font-size: 22px;
  line-height: 44px;
  letter-spacing: 0.02em;
  text-align: center;
  color: var(--brave-palette-neutral900);
`

export const paymentIDSection = styled.div`
  margin-top: 12px;
  display: flex;
  justify-content: center;
`

export const paymentIDBox = styled.div`
  border: 1px solid var(--brave-palette-neutral200);
  border-radius: 10px;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  padding: 4px 10px;
  color: var(--brave-palette-neutral600);
`

export const paymentIDValue = styled.span`
  font-weight: normal;
`

export const sectionText = styled.div`
  margin-top: 12px;

  label {
    font-weight: 600;
    color: var(--brave-palette-blurple500);
    cursor: pointer;
  }
`

export const sectionHeader = styled.div`
  font-weight: 600;
  font-size: 18px;
  line-height: 24px;
  margin-bottom: 12px;
`

export const devicesSection = styled.div`
  padding: 32px 55px 37px;
  border-bottom: 1px solid var(--brave-palette-neutral200);
`

export const settingsSection = styled.div`
  padding: 38px 55px 0;
`

export const tabstrip = styled.div`
  margin: 29px 0 29px;

  --slider-switch-height: 43px;
  --slider-switch-background: var(--brave-palette-neutral200);
  --slider-switch-font: 600 14px/30px var(--brave-font-heading);
  --slider-switch-color: var(--brave-palette-neutral600);
  --slider-switch-selected-color: var(--brave-palette-blurple400);
`

export const recoveryInput = styled.div`
  margin-top: 8px;

  textarea {
    font-family: var(--brave-font-body);
    border: 1px solid #DFDFE8;
    color: #686978;
    min-height: 140px;
    width: 100%;
    border-radius: 6px;
    padding: 15px 20px;
    font-size: 16px;
    line-height: 26px;
    outline: none;

    &:focus {
      border-color: #A1A8F2;
    }
  }
`

export const formAction = styled.div`
  text-align: center;
  margin-top: 24px;
`
