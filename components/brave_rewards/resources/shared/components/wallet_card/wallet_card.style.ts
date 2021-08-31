/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%);
  border-radius: 14px;
  font-family: var(--brave-font-heading);
  color: var(--brave-palette-white);
`

export const overview = styled.div`
  display: flex;
  align-items: flex-end;
  margin-bottom: 7px;
  padding: 13px 13px 0;
`

export const balancePanel = styled.div`
  flex: 1 1 50%;
  padding-right: 5px;
`

export const rewardsBalance = styled.div`
  margin-left: 20px;
  margin-top: 6px;
`

export const balanceHeader = styled.div`
  font-weight: 500;
  font-size: 14px;
  line-height: 22px;
  opacity: 0.65;
`

export const batAmount = styled.div`
  margin-top: 2px;
  font-size: 24px;
  line-height: 32px;

  .currency {
    font-size: 14px;
    line-height: 14px;
    opacity: 0.66;
  }
`

export const exchangeAmount = styled.div`
  font-size: 12px;
  line-height: 14px;
  opacity: 0.66;
`

export const earningsPanel = styled.div`
  flex: 1 1 50%;
  padding-left: 5px;
`

export const dateRange = styled.div`
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
`

export const earningsHeader = styled.div`
  font-weight: 500;
  font-size: 14px;
  line-height: 22px;
  opacity: 0.65;
`

export const summaryBox = styled.div`
  margin-top: 17px;
  padding: 0 17px 16px;
`

export const summaryActions = styled.div`
  margin: 0 9px 24px 13px;
  display: flex;
  justify-content: space-between;

  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    padding: 6px 18px;
    border-radius: 48px;
    border: none;
    background: transparent;
    cursor: pointer;
  }
`

export const addFunds = styled.div`
  button {
    background: rgba(255, 255, 255, 0.24);

    &:active {
      background: rgba(255, 255, 255, 0.30);
    }
  }

  .icon {
    width: 17px;
    height: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }
`

export const viewStatement = styled.div`
  button {
    padding: 6px 13px;

    &:active {
      background: rgba(255, 255, 255, 0.05);
    }
  }

  .icon {
    width: 15px;
    height: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }
`

export const pendingBox = styled.div`
  margin-top: 17px;
  border-radius: 0 0 16px 16px;
  overflow: hidden;
`
