/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import tipAnimation from '../assets/tip_animation.gif'

export const root = styled.div`
  text-align: center;
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const success = styled.div`
  height: 100%;
  background-repeat: no-repeat;
  background-size: 100% auto;
  background-position: top;
  background-image: url(${tipAnimation});
`

export const main = styled.div`
  flex: 1 1 auto;
`

export const header = styled.div`
  display: inline-block;
  padding-top: 63px;
  padding-bottom: 9px;
  text-align: center;

  border-bottom: 1px solid rgba(174, 177, 194, 0.5);
  font-weight: 600;
  font-size: 22px;
  line-height: 33px;
`

export const message = styled.div`
  padding-top: 46px;
  text-align: center;

  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  color: #212529;
`

export const table = styled.div`
  margin-top: 17px;
  text-align: left;

  font-size: 14px;
  line-height: 21px;

  table {
    width: calc(100% - 110px);
    max-width: 300px;
    margin: 0 auto;
  }

  tr:nth-of-type(n+2) td {
    padding-top: 16px;
  }

  td:last-of-type {
    text-align: right;
  }
`

export const share = styled.div`
  margin-bottom: 64px;

  button {
    padding: 10px 39px;
    font-weight: 600;
    font-size: 13px;
    line-height: 19px;
    color: #fff;
    border: none;
    background: #5CB5FC;
    border-radius: 20.5px;
    text-align: center;
  }

  button:active {
    background: rgb(129 193 245 / 89%);
  }

  .icon {
    width: 20px;
    vertical-align: middle;
    margin-right: 5px;
  }

  .icon * {
    fill: #fff;
  }
`

export const cancelMain = styled.div`
  flex: 1 1 auto;
`

export const cancelHeader = styled.div`
  margin-top: 152px;

  font-weight: 600;
  font-size: 16px;
  line-height: 24px;
  color: #212529;
`

export const cancelText = styled.div`
  margin: 24px 53px 0;

  color: #212529;
`
