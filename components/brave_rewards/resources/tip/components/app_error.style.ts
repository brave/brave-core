/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import unknownErrorImage from '../assets/unknown_error.svg'
import serverErrorImage from '../assets/server_error.svg'
import tippingErrorImage from '../assets/tipping_error.svg'

export const root = styled.div`
  text-align: center;
  display: flex;
  height: 100%;
  flex-direction: column;
`

export const graphic = styled.div`
  height: 174px;
  background: url(${unknownErrorImage});
  background-repeat: no-repeat;
  background-position: bottom center;
  background-size: auto 95px;

  &.server-error {
    background-image: url(${serverErrorImage});
    background-size: auto 77px;
  }

  &.tipping-error {
    background-image: url(${tippingErrorImage});
    background-size: auto 77px;
  }
`

export const heading = styled.div`
  padding-top: 21px;
  padding-left: 25px;
  padding-right: 25px;
  font-weight: 600;
  font-size: 18px;
  line-height: 27px;
  color: var(--brave-palette-neutral900);
`

export const message = styled.div`
  flex: 1 0 auto;
  padding-top: 4px;
  max-width: 198px;
  margin: 0 auto;
  font-size: 14px;
  line-height: 20px;
  color: var(--brave-palette-neutral700);
`

export const details = styled.div`
  font-size: 10px;
  padding: 5px;
  color: var(--brave-palette-grey500);
`
