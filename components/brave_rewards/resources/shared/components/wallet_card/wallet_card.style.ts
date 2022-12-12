/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  background: linear-gradient(125.83deg, #392DD1 0%, #A91B78 99.09%);
  border-radius: 14px;
  font-family: var(--brave-font-heading);
  color: var(--brave-palette-white);
`

export const grid = styled.div`
  padding: 13px 13px 0;
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: auto;
  grid-template-areas:
    "status-indicator earnings"
    "balance          earnings"
    "empty            view-statement";
`

export const statusIndicator = styled.div`
  grid-area: status-indicator;
  justify-self: start;
  align-self: start;
`

export const rewardsBalance = styled.div`
  grid-area: balance;
  justify-self: start;
  align-self: end;
  margin: 6px 5px 0 20px;
`

export const disconnectedBalance = styled.div`
  grid-area: balance;
  justify-self: start;
  align-self: start;
  margin: 6px 15px 0 4px;
  padding: 12px;
  border-radius: 8px;
  background: linear-gradient(137.04deg, #346FE1 33.4%, #5844C3 82.8%);
  font-size: 12px;
  line-height: 18px;
  cursor: pointer;
  position: relative;

  strong {
    font-weight: 600;
    color: #fff;
  }

  .icon {
    display: none;
    width: 20px;
    height: 20px;
    position: absolute;
    bottom: 1em;
    right: 1em;
  }

  &.cover-actions {
    grid-area: balance-start / balance-start / empty-end / balance-end;
    align-self: stretch;
    font-size: 14px;
    line-height: 20px;
    padding-bottom: 32px;

    .icon {
      display: block;
    }
  }
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
  grid-area: earnings;
  justify-content: start;
  align-self: end;
  margin-top: 20px;
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

export const hiddenEarnings = styled.div`
  font-weight: 500;
  font-size: 24px;
  line-height: 30px;
  color: rgba(255, 255, 255, 0.66);
  min-height: 47px;

  a {
    display: inline-block;
    margin-left: 8px;
    margin-top: -4px;
    vertical-align: middle;
    color: #fff;
    text-decoration: none;
    font-weight: 600;
    font-size: 12px;
    line-height: 18px;
  }
`

const summaryActionButton = `
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  padding: 6px 18px;
  border-radius: 48px;
  border: none;
  background: transparent;
  cursor: pointer;
`

export const viewStatement = styled.div`
  grid-area: view-statement;
  align-self: center;
  justify-self: end;
  margin-top: 9px;
  margin-right: 9px;

  button {
    ${summaryActionButton}
    padding: 6px 13px;
  }

  .icon {
    width: 15px;
    height: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }
`

export const summaryBox = styled.div`
  padding: 17px 17px 16px;
`

export const pendingBox = styled.div`
  margin-top: 17px;
  border-radius: 0 0 14px 14px;
  overflow: hidden;
  /* Moving this box down slightly ensures that the underlying background color
     will not show through the rounded corners. */
  position: relative;
  top: 2px;
`
