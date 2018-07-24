/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { setTheme } from '../../../helpers'
import { Props } from './index'

export const StyledWrapper = styled.div`
` as any

export const StyledImage = styled.img`
` as any

export const StyledContent = styled.div`
  font-family: Poppins;
  font-size: 16px;
  font-weight: 500;
  line-height: 1.5;
  color: ${(p: Props) => setTheme(p.theme, 'color') || '#4b4c5c'};

  b,
  a {
    font-weight: 500;
    color: ${(p: Props) => setTheme(p.theme, 'boldColor') || '#4b4c5c'};
  }

  h3 {
    margin: 0;
    display: inline-block;
    padding: 0;
    font-family: Poppins;
    font-size: 28px;
    font-weight: 500;
    line-height: 0.5;
    color: #ceb4e1;
  }
` as any
