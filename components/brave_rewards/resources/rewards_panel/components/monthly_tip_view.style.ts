/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div``

export const setBox = styled.div`
  margin-right: 4px;

  a {
    font-size: 13px;
    line-height: 21px;
  }
`

export const amountBox = styled.div`
  position: relative;
`

export const amount = styled.div`
  font-size: 14px;
  line-height: 22px;
  color: var(--brave-palette-neutral600);

  .amount {
    font-weight: 600;
    color: var(--brave-color-brandBatInteracting);

    .brave-theme-dark & {
      color: var(--brave-palette-blurple300);
    }
  }

  .currency {
    font-size: 12px;
  }

  .icon {
    width: 12px;
    height: auto;
    vertical-align: middle;
    margin-bottom: 1px;
    margin-right: 2px;
  }

  button {
    border: none;
    background: none;
    margin: 0;
    padding: 0;
    cursor: pointer;

    &:hover .icon {
      color: var(--brave-palette-neutral700);
    }

    .brave-theme-dark &:hover .icon {
      color: var(--brave-palette-grey400);
    }
  }
`

export const actionBubble = styled.div`
  position: absolute;
  top: 25px;
  right: -4px;
  overflow: hidden;
  box-shadow: 1px 1px 6px 0px rgba(0, 0, 0, .1);
  border: solid 1px rgba(0, 0, 0, .2);
  border-radius: 6px;
  z-index: 1;
`

export const actionBubbleContent = styled.div`
  background: var(--brave-palette-white);
  padding: 6px 12px;

  .brave-theme-dark & {
    background: var(--brave-palette-grey800);
    color: var(--brave-palette-grey000);
  }
`

export const action = styled.div`
  font-size: 12px;
  line-height: 24px;
  white-space: nowrap;
  text-align: left;
`

export const backdrop = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  z-index: -1;
`
