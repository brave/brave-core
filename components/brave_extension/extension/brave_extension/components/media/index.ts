/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import { StyledComponentClass } from 'styled-components'
import { CaratStrongDownIcon, ShieldAlertIcon, AlertMessageIcon } from 'brave-ui/components/icons'

import { ComponentType } from 'react'

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
  color: ${p => p.theme.color.lionLogo};
`

export const WarningIcon = styled(AlertMessageIcon as ComponentType)`
  box-sizing: border-box;
  display: block;
  width: 48px;
  height: auto;
  margin: 0 auto 12px;
  color: ${p => p.theme.color.lionLogo};
`

export const ArrowDownIcon = styled(CaratStrongDownIcon as ComponentType)`
  width: 24px;
  height: 24px;
  padding: 4px;
  color: ${p => p.theme.color.text};
  pointer-events: none;

  &:focus {
    outline-width: 2px;
    outline-offset: -6px;
    outline-color: ${p => p.theme.color.brandBrave};
  }

  &:active {
    outline: none;
  }
`

export const ArrowUpIcon = RotatedIconComponent(ArrowDownIcon, 180)
