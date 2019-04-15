/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'
import { StyledComponentClass } from 'styled-components'
import { CaratStrongDownIcon, ShieldAlertIcon } from '../../../components/icons'

import { ComponentType } from 'react'
import palette from '../../../theme/colors'

// rotated variants
function RotatedIconComponent (
  iconComponent: StyledComponentClass<any, any>,
  degrees: number
) {
  return styled(iconComponent)`
    transform: rotate(${degrees}deg);
  `
}

export const ShieldIcon = styled(ShieldAlertIcon as ComponentType)`
  box-sizing: border-box;
  display: block;
  width: 36px;
  margin: auto;
  color: ${palette.grey500};
`

export const ArrowDownIcon = styled(CaratStrongDownIcon as ComponentType)`
  width: 24px;
  height: 24px;
  padding: 4px;
  color: ${p => p.theme.color.text};

  &:focus {
    outline-width: 2px;
    outline-offset: -6px;
    outline-color: ${p => p.theme.color.brandBrave};
  }

  &:active {
    outline: none;
  }
`

export const ArrowUpIcon = RotatedIconComponent(ArrowDownIcon, 90)
