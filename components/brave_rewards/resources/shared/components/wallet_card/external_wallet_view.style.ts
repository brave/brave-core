/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  position: relative;
`

export const verifyWallet = styled.div`
  button {
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    padding: 6px 18px;
    background: rgba(255, 255, 255, 0.24);
    border-radius: 48px;
    border: none;
    cursor: pointer;

    &:active {
      background: rgba(255, 255, 255, 0.30);
    }
  }

  .icon {
    width: 17px;
    height: auto;
    vertical-align: middle;
    margin-left: 8px;
    margin-bottom: 2px;
  }
`

export const reconnectWallet = styled.div`
  button.connect {
    font-size: 12px;
    line-height: 18px;
    padding: 6px 10px;
  }

  .icon {
    height: 17px;
    width: auto;
    vertical-align: middle;
    margin-right: 6px;
    margin-bottom: 2px;
  }
`

export const bubbleAction = styled.div`
  button {
    background: none;
    font-weight: 600;
    font-size: 12px;
    line-height: 18px;
    padding: 6px 18px;
    border-radius: 48px;
    border: none;
    cursor: pointer;

    &.pressed, &:hover {
      background: rgba(0, 0, 0, 0.14);
    }

    .provider .icon {
      height: 14px;
      width: auto;
      vertical-align: middle;
      margin-left: 4px;
      margin-bottom: 2px;
    }

    .status .icon {
      width: 14px;
      height: auto;
      vertical-align: middle;
      margin-left: 2px;
      margin-bottom: 3px;
      margin-right: -3px;
    }

    .caret .icon {
      width: 11px;
      height: auto;
      vertical-align: middle;
      margin-left: 4px;
      margin-bottom: 2px;
    }
  }
`
