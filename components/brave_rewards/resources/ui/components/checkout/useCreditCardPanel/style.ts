/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { CaratStrongRightIcon } from 'brave-ui/components/icons'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const ContinueBox = styled.div`
  font-size: 12px;
  line-height: 18px;
  display: flex;
`

export const ContinueBoxText = styled.div`
  flex-grow: 1;
`

export const ContinueBoxLink = styled.div`
  padding-right: 13px;
  flex-grow: 1;
  text-align: right;
  min-width: 215px;
`

export const RightIcon = styled(CaratStrongRightIcon)`
  position: relative;
  top: 2px;
  left: 12px;
  height: 11px;
  width: 11px;
`

export const ConfirmButtonRow = styled.div<{ showBackLink?: boolean }>`
  margin: 30px 0 0;
  display: flex;
  align-items: center;
  justify-content: ${p => p.showBackLink ? 'space-between' : 'center'};
}
`

interface ConfirmButtonProps extends ButtonProps {
  showBackLink?: boolean
}

export const ConfirmButton = styled(Button as ComponentType<ConfirmButtonProps>)`
  min-width: ${p => p.showBackLink ? '231px' : '322px'};
`

export const TermsOfSale = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  text-align: center;
  padding-top: 12px;

  a {
    font-weight: bold;
    color: ${p => p.theme.palette.black};
  }
`
