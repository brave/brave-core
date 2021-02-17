/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  src?: string
}

export const StyledWrapper = styled<{}, 'a'>('a')`
  color: #fff;
  width: 100%;
  background: #fff;
  margin-top: 30px;
  border-radius: 8px;
  min-height: 157px;
  cursor: pointer;
  display: flex;
  overflow: hidden;
`

export const StyledCloseIcon = styled<{}, 'div'>('div')`
  width: 12px;
  height: 12px;
  color: ${palette.grey600};
  position: relative;
  z-index: 2;
  float: right;
  margin: 11px 11px 0 0;
`

export const StyledBackground = styled<StyleProps, 'div'>('div')`
  width: 117px;
  background: url(${p => p.src}) no-repeat;
`

export const StyledContent = styled<{}, 'div'>('div')`
  max-width: 222px;
  margin-top: 14px;
  padding-left: 15px;
`

export const StyledTitle = styled<{}, 'span'>('span')`
  color: #000;
  display: block;
  font-size: 15px;
  font-weight: 600;
  margin-bottom: 9px;
  font-family: Poppins, sans-serif;
  line-height: 1.6;
`

export const StyledInfo = styled<{}, 'div'>('div')`
  font-size: 11.5px;
  line-height: 18px;
  color: ${palette.grey600};
  font-family: Muli, sans-serif;
  line-height: 1.57;
`

export const StyledLink = styled<{}, 'a'>('a')`
  margin-left: 2px;
  font-weight: bold;
  display: inline-block;
  color: ${palette.blurple400};
  text-decoration: none;
`

export const StyledDisclaimer = styled<{}, 'span'>('span')`
  display: block;
  margin: 10px 0px;
  font-size: 11px;
  font-weight: bold;
  color: ${palette.grey500};
  font-family: Muli, sans-serif;
`

export const StyleRight = styled<{}, 'div'>('div')`
  flex-grow: 1;
`
