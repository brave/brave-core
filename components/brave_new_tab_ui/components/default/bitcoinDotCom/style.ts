/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  contentShowing?: boolean
}

export const WidgetWrapper = styled<StyleProps, 'div'>('div')`
  padding: 6px 20px 12px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  background: #fff;
`

export const Header = styled<{}, 'div'>('div')`
  text-align: left;
`

export const Title = styled<StyleProps, 'div'>('div')`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: ${p => p.contentShowing ? '#000' : '#fff'};
  font-family: ${p => p.theme.fontFamily.heading};
`

export const BitcoinDotComIcon = styled<{}, 'div'>('div')`
  width: 27px;
  height: 27px;
  margin-right: 7px;
  margin-left: 2px;
`

export const TitleText = styled<{}, 'div'>('div')`
  margin-top: 4px;
`

export const BitcoinDotComIconImg = styled<{}, 'img'>('img')`
`
