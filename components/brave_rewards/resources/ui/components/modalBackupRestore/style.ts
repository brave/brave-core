/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { ComponentType } from 'react'
import styled from '../../../theme'
import Button, { Props as ButtonProps } from '../../../components/buttonsIndicators/button'
import { Props } from './index'

export const StyledWrapper = styled<{}, 'div'>('div')`
  margin-top: 33px;
`

export const StyledContent = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 16px;
  font-weight: 300;
  line-height: 1.63;
  color: #686978;
  margin-bottom: 39px;
`

export const StyledImport = styled<{}, 'label'>('label')`
  color: #4c54d2;
  cursor: pointer;
`

export const StyleButtonWrapper = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
`

export const GroupedButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 0 4px;
`

export const StyledDoneWrapper = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
  margin-top: 59px;
`

export const StyledStatus = styled<Partial<Props>, 'div'>('div')`
  display: ${p => p.error ? 'block' : 'none'};
  margin: -16px 0 16px;
  border-radius: 6px;
  overflow: hidden;
`

export const StyledActionsWrapper = styled<{}, 'div'>('div')`
  margin-top: 108px;
  display: flex;
  justify-content: center;
`

export const ActionButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 0 8px;
`
