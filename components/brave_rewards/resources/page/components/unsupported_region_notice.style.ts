/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  height: 100%;
  width: 100%;
  font-family: var(--brave-font-heading);
  text-align: center;

  .icon {
    height: 80px;
    margin-bottom: 32px;
  }

  a {
    color: var(--brave-color-brandBat);
    text-decoration: none;
  }
`

export const heading = styled.div`
  font-weight: 500;
  font-size: 16px;
  line-height: 24px;
  margin-bottom: 8px;
  margin-left: auto;
  margin-right: auto;

  .layout-narrow & {
    max-width: 312px;
  }
`

export const content = styled.div`
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  max-width: 325px;
  margin-left: auto;
  margin-right: auto;

  .layout-narrow & {
    max-width: 312px;
  }
`

export const spacing = styled.div`
  margin-top: 64px;
`

export const text = styled.div`
  margin-bottom: 8px;
`
