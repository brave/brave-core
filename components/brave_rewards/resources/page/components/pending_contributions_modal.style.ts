/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  width: calc(100vw - 22px);
  max-width: 420px;
  min-height: 350px;
  max-height: calc(100vh - 40px);
  overflow: auto;
  border-radius: 8px;
  background: var(--brave-palette-white);
`

export const header = styled.div`
  display: grid;
  grid-template-columns: 1fr 18px;
  grid-template-rows: 1;
  align-items: center;
  border-bottom: 1px solid rgba(0, 0, 0, 0.12);
  padding: 21px 24px;

  button {
    grid-column-start: 2;
    justify-self: end;
  }

  div {
    display: flex;
    align-items: baseline;
    padding-top: 2px;
  }
`

export const title = styled.span`
  grid-column-start: 1;
  justify-self: start;
  line-height: 22px;
  font-size: 20px;
  font-weight: 500;
  font-color: rgba(33, 37, 41, 1);
`

export const constributionList = styled.div`
  height: calc(100% - 16px - 67px);
  overflow-y: auto;
`

/* Each row is a grid
    ---------------------------------------------
    |   | 1(29px) | 2(1fr) |  3(1fr)  | 4(14px) |
    ---------------------------------------------
    | 1 |  Icon     Site              |     |_| |
    | 2 |         | Pending Amount              |
    | 3 |         | Type  |                Date |
    ---------------------------------------------
*/

export const contributionItem = styled.div`
  display: grid;
  grid-template-columns: 29px 1fr 1fr 14px;
  grid-template-rows: 3;
  padding: 16px;
  align-items: center;
  justify-items: start;
  border-bottom: 1px solid rgba(0, 0, 0, 0.12);
  overflow-y: visible;
  white-space: nowrap;
`

export const publisher = styled.div`
  grid-row-start: 1;
  grid-column: 1 / 4;
  display: flex;
  font-size: 16px;
  font-weight: 500;

  a {
    text-decoration: none;
  }

  img {
    display: inline-block;
    vertical-align: middle;
    margin-top: -2px;
    margin-right: 8px;
    width: 20px;
    height: 20px;
  }
`

export const platform = styled.span``

export const deleteAction = styled.div`
  grid-row-start: 1;
  grid-column-start: 4;
  justify-self: end;
  align-self: center;
  line-height: 20px;
  font-size: 16px;

  button {
    padding: 2px 0px 0px 0px;
    border: none;
    background: none;
    cursor: pointer;
    text-align: center;
    width: 100%;

    .icon {
      color: var(--brave-palette-grey500);
      height: 16px;
      width: auto;
      display: inline-block;
    }
  }
`

export const amount = styled.div`
  grid-row-start: 2;
  grid-column: 2 / -1;
  font-size: 14px;
  font-weight: 500;
`

export const exchangeAmount = styled.span`
  color: #9E9FAB;
`

export const contributionType = styled.div`
  grid-row-start: 3;
  grid-column-start: 2;
  font-size: 14px;
  line-height: 22px;
`

export const contributionDate = styled.div`
  grid-row-start: 3;
  grid-column: 3 / -1;
  font-size: 12px;
  justify-self: end;
  text-align: right;
  line-height: 20px;
  padding-top: 2px;
`

export const deleteAllAction = styled.div`
  text-align: center;
  padding: 15px 0;
  line-height: 22px;
  font-size: 14px;
  font-weight: 600;

  button {
    ${mixins.buttonReset}
    cursor: pointer;
    color: var(--brave-color-brandBat);
  }
`

export const noContent = styled.div`
  text-align: center;
  padding: 30px 0;
  color: #989898;
  font-size: 14px;
`
