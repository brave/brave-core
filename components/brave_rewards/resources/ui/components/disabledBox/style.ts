/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import { ComponentType } from 'react'
import styled from 'brave-ui/theme'
import Card, { CardProps } from 'brave-ui/components/layout/card'
import palette from 'brave-ui/theme/colors'

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: block;
  width: 100%;
  margin-bottom: 28px;
`

export const StyledCard = styled<CardProps>(Card as ComponentType<CardProps>)`
  font-family: Poppins, sans-serif;
`

export const StyledTitle = styled<{}, 'span'>('span')`
  width: 541px;
  color: ${palette.blurple500};
  font-size: 22px;
  font-weight: 600;
  letter-spacing: 0;
  line-height: 28px;
  padding-bottom: 5px;
`

export const StyledText = styled<{}, 'p'>('p')`
  color: ${palette.grey600};
  font-size: 16px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 26px;
  padding-bottom: 0px;
  font-family: Muli, sans-serif;

  &:first-of-type {
    padding-bottom: 10px;
  }
`
