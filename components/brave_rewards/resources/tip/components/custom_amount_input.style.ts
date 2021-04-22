/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  text-align: left;
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral700);
`

export const header = styled.div`
  font-size: 14px;
  line-height: 21px;
  font-weight: 600;
  text-align: center;
  position: relative;
  padding-bottom: 32px;

  button {
    position: absolute;
    top: 1px;
    left: 0;
    height: 16px;
    border: 0;
    padding: 0;
    background: none;
    cursor: pointer;

    .icon {
      color: var(--brave-palette-grey500);
      height: 100%;
      width: auto;
      display: block;
    }
  }
`

export const form = styled.div`
  display: flex;
  flex-flow: justify-content;
`

export const amountSection = styled.div`
  flex: 1 0 calc(50% - 56px);
`

export const amountBox = styled.div`
  position: relative;
  height: 40px;
  border-radius: 4px;
  border: solid 1px var(--brave-palette-grey500);
  display: flex;

  input {
    flex: 1 1 auto;
    width: 100%;
    border: 0;
    background: none;
    padding: 10px 40px 10px 18px;
  }

  .currency {
    position: absolute;
    top: 0;
    right: 0;
    font-weight: 600;
    padding: 9px 5px;
  }
`

export const example = styled.div`
  margin: 4px 0 0 10px;
  font-size: 12px;
  line-height: 18px;
  color: var(--brave-palette-neutral600);
`

export const swap = styled.div`
  flex: 0 0 40px;
  margin: 0 8px;

  button {
    border: 0;
    background: 0;
    padding: 0;
    cursor: pointer;
    height: 40px;
  }

  .icon {
    width: 100%;
    height: auto;
  }
`

export const exhangeBox = styled.div`
  flex: 1 0 calc(50% - 56px);
  height: 40px;
  color: var(--brave-palette-neutral600);
  border-radius: 4px;
  border: solid 1px var(--brave-palette-neutral200);
  background: rgba(233, 233, 244, 0.16);
  padding: 10px 9px 10px 18px;
  display: flex;

  > * {
    flex: 1 1 auto;
  }

  .currency {
    flex: 0 0 auto;
    font-weight: 600;
    margin-left: 5px;
  }
`
