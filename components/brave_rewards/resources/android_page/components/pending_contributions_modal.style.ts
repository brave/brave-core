/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: absolute;
  left: 0px;
  top: 8px;
  width: 100%;
  height: calc(100% - 16px);
  max-width: 700px;
  min-height: 545px;

  border-radius: 8px;
  background: var(--brave-palette-white);

  display: block;
  overflow-y: hidden;

  font-family: var(--brave-font-heading);
  line-height: 24px;
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
  line-height: 24px;
  font-size: 22px;
  font-weight: 500;
  font-color: rgba(33, 37, 41, 1);
`

export const constributionsList = styled.div`
  height: calc(100% - 16px - 67px);
  overflow-y: scroll;
`

export const constributionsListItem = styled.div`
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

export const contributionReceiver = styled.div`
  grid-row-start: 1;
  grid-column: 1 / 4;
  display: flex;
  font-size: 16px;
  font-weight: 500;

  a {
    text-decoration: none;
  }
`

export const contributionDelete = styled.div`
  grid-row-start: 1;
  grid-column-start: 4;
  justify-self: end;
  align-self: center;
  line-height: 16px;

  button {
    padding: 2px 0px 0px 0px;
    border: none;
    background: none;
    cursor: pointer;
    text-align: center;
    width: 100%;

    .icon {
      color: var(--brave-palette-grey500);
      width: 14px;
      height: 16px;
      display: inline-block;
    }
  }
`

export const contributionAmount = styled.div`
  grid-row-start: 2;
  grid-column: 2 / -1;
  font-size: 14px;
  font-weight: 500;
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

export const noContent = styled.div`
  text-align: center;
  padding: 30px 0;
  color: #989898;
  font-size: 14px;
`
