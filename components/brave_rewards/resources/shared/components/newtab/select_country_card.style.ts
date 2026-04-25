/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { color, font } from '@brave/leo/tokens/css/variables'

export const root = styled.div`
  padding: 20px 16px;
  text-align: center;
  background: linear-gradient(137.04deg, #346FE1 33.4%, #5844C3 82.8%);
  border-radius: 8px;
`

export const header = styled.div`
  font: ${font.heading.h4};

  .icon {
    vertical-align: middle;
    width: 16px;
    margin-right: 5px;
    margin-bottom: 3px;
  }
`

export const text = styled.div`
  margin-top: 9px;
  font: ${font.small.regular};

  a {
    color: ${color.white};
    text-decoration: underline;
    padding-left: 4px;
  }
`

export const enable = styled.div`
  margin-top: 16px;
  display: flex;
  justify-content: stretch;
`
