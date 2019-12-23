/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import palette from 'brave-ui/theme/colors'

export const Wrapper = styled<{}, 'div'>('div')`
  position: relative;
  margin-bottom: 24px;
`
export const Content = styled<{}, 'div'>('div')`
  display: flex;
  flex-grow: 1;
`

export const Icon = styled<{}, 'div'>('div')`
  flex: 0 0 36px;
  color: ${palette.yellow600};
`

export const Text = styled<{}, 'div'>('div')`
  flex: 1 1 80%;
  font-size: 14px;
  padding-left: 25px;
  line-height: 1.5;
`

export const Button = styled<{}, 'div'>('div')`
  flex: 1 0 200px;
  display: flex;
  justify-content: flex-end;
  align-items: center;
`

export const Read = styled<{}, 'span'>('span')`
  display: inline;
  color: ${palette.blurple500};
  text-transform: uppercase;
  margin-right: 5px;
  font-weight: bold;
`

export const Body = styled<{}, 'span'>('span')`
  display: inline;
  margin-right: 5px;
  color: ${palette.grey700};
`
