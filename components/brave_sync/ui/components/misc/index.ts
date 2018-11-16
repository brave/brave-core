/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

export const Main = styled<{}, 'main'>('main')`
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.color.defaultControl};
  max-width: 840px;
  padding: 40px;
  margin: auto;
`

export const QRCode = styled<{}, 'img'>('img')`
  display: block;
  max-width: 180px;
`

export const TableRowId = styled<{}, 'span'>('span')`
  width: 5ch;
  text-align: center;
  display: block;
`

export const TableRowDevice = styled<{}, 'span'>('span')`
  max-width: 15ch;
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
  padding: 0 6px;
  border: 0;
  background: transparent;
  cursor: pointer;
  display: block;
  margin: auto;
`
