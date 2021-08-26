/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const main = styled.div`
  flex: 1 1 auto;
  padding: 0 32px;
  align-self: center;
  width: 100%;
  max-width: 450px;
`

export const customAmount = styled.div`
  margin-top: 24px;
`

export const customInput = styled.div`
  border-top: 1px solid rgba(174, 177, 194, 0.5);
  margin-top: 11px;
  padding-top: 16px;
`

export const amounts = styled.div`
  padding-top: 24px;
  margin-top: 11px;
  border-top: 1px solid rgba(174, 177, 194, 0.5);
}
`

export const customAmountButton = styled.div`
  margin-top: 34px;
  text-align: center;

  button {
    border: 0;
    background: none;
    padding: 0;
    font-size: 13px;
    line-height: 20px;
    font-weight: 600;
    color: var(--brave-color-brandBatInteracting);
    cursor: pointer;
  }
`

export const footer = styled.div`
  flex: 0 0 auto;

  .submit-monthly {
    --icon-margin-top: -4px;
  }
`

export const terms = styled.div`
  padding: 0 30px 7px;
`

export const minimumAmount = styled.div`
  padding: 19px 0;
  text-align: center;
  color: var(--brave-palette-neutral900);
  background: var(--brave-palette-green200);
  font-size: 14px;
  line-height: 21px;
  font-weight: 600;

  .icon {
    display: inline-block;
    height: 22px;
    width: 22px;
    vertical-align: middle;
    margin: -1.5px 2px 0 0;
    color: var(--brave-palette-teal600);
  }
`

export const notEnoughFunds = styled.div`
  padding: 19px 0;
  text-align: center;
  color: var(--brave-palette-white);
  background: var(--brave-palette-neutral600);
  font-size: 14px;
  line-height: 21px;
  font-weight: 600;

  a {
    color: var(--brave-color-brandBatInteracting);
  }

  .icon {
    display: inline-block;
    height: 22px;
    width: 22px;
    vertical-align: middle;
    margin: -1.5px 2px 0 0;
    color: var(--brave-palette-neutral300);
  }
`

export const feeNote = styled.div`
  text-align: center;
  font-size: 11px;
  line-height: 16px;
  color: var(--brave-palette-neutral600);
`
