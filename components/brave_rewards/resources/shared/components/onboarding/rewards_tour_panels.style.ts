/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import warningCircleImage from '../../assets/warning_circle.svg'

export const formText = styled.div`
  color: var(--brave-color-brandBat);
  font-size: 14px;
  line-height: 20px;
  margin-top: -5px;
`

export const verifySubtext = styled.div`
  position: relative;
  text-align: left;
  margin-top: 11px;
  background: #F8F9FA;
  border-radius: 8px;
  padding: 14px 14px 14px 38px;
  font-size: 13px;
  line-height: 19px;
  color: #495057;

  background-repeat: no-repeat;
  background-position: 14px 18px;
  background-size: 16px 16px;
  background-image: url('${warningCircleImage}');
`

export const verifyNote = styled.div`
  margin-top: 8px;
  font-size: 11px;
  line-height: 17px;
  color: #868E96;
`

export const verifyActions = styled.div`
  display: flex;
  justify-content: space-between;

  button {
    color: #212529;
    font-size: 13px;
    font-weight: 600;
    line-height: 19px;
    padding: 5px 20px;
    border: 1px solid #AEB1C2;
    background: none;
    border-radius: 30px;
    cursor: pointer;
  }

  button.verify-now {
    border: none;
    background: var(--brave-color-brandBat);
    color: var(--brave-palette-white);
  }

  button:active {
    background: var(--brave-color-brandBatActive);
  }
`

export const verifyLearnMore = styled.div`
  margin-top: 11px;
  font-size: 13px;
  line-height: 19px;
  padding-bottom: 20px;

  a {
    color: var(--brave-color-brandBat);
    text-decoration: none;
    font-weight: 600;
  }
`
