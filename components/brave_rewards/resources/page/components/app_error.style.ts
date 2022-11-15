/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import errorBackground from '../assets/app_error.svg'

export const root = styled.div`
  margin: 80px auto 0;
  font-family: Poppins;
  display: flex;
  flex-direction: column;
  align-items: center;
`

export const title = styled.div`
  background-image: url(/${errorBackground});
  background-repeat: no-repeat;
  background-position: center top;
  background-size: auto 50px;
  padding-top: 75px;
  font-size: 18px;
  font-weight: 600;
  color: var(--brave-palette-neutral800);
  text-align: center;
`

export const details = styled.div`
  margin: 30px auto 0;
  white-space: pre;
  font-size: 11px;
  color: var(--brave-palette-neutral500);
  max-width: 100%;
  padding: 0 20px 20px;
  overflow-x: auto;
`
