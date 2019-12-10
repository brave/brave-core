/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'brave-ui/theme'
import Card, { CardProps } from 'brave-ui/components/layout/card'

export const DisabledContent = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: inherit;
  background-color: #efeff0;
`

export const EnabledContent = styled<{}, 'div'>('div')`
  height: inherit;
  background-color: #efeff0;
`

export const Main = styled<{}, 'main'>('main')`
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.color.defaultControl};
  padding: 50px 15px;
  max-width: 830px;
  margin: auto;
`

export const SyncCard: React.FC<CardProps> = (props) =>
  <Card useDefaultContentPadding={false} {...props} />

export const SyncCardContent = styled('div')`
  padding: 60px 80px;
`

export const TableRowId = styled<{}, 'span'>('span')`
  width: 5ch;
  text-align: center;
  display: block;
`

export const TableRowDevice = styled<{}, 'span'>('span')`
  max-width: 30ch;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  display: block;
`

export const TableRowRemove = styled<{}, 'span'>('span')`
  display: block;
  text-align: center;
`

export const TableRowRemoveButton = styled<{}, 'button'>('button')`
  text-align: center;
  padding: 0;
  border: 0;
  background: transparent;
  cursor: pointer;
  display: block;
  margin: auto;
  width: 24px;
`

export const TableRowToggleButton = styled<{}, 'span'>('span')`
  float: right;
  margin: 9px;
`
