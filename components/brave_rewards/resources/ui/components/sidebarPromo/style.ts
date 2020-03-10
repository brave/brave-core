/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  src?: string
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  color: #fff;
  width: 100%;
  padding: 7px;
  background: #fff;
  margin-top: 30px;
  border-radius: 8px;
  box-shadow: 0 0 13px 0 rgba(0,0,0,0.07);
`

export const StyledCloseIcon = styled<{}, 'div'>('div')`
  width: 12px;
  height: 12px;
  float: right;
  cursor: pointer;
  color: ${palette.grey600};
`

export const StyledBackground = styled<StyleProps, 'div'>('div')`
  float: left;
  width: 130px;
  height: 130px;
  position: absolute;
  border-radius: 8px;
  margin: 10px 0px 0px -7px;
  background: url(${p => p.src}) no-repeat top right;
`

export const StyledContent = styled<{}, 'div'>('div')`
  float: right;
  max-width: 190px;
  margin-top: 25px;
  text-align: left;
`

export const StyledTitle = styled<{}, 'span'>('span')`
  color: #000;
  display: block;
  font-size: 15px;
  font-weight: bold;
  margin-bottom: 5px;
  font-family: Poppins, sans-serif;
`

export const StyledInfo = styled<{}, 'span'>('span')`
  font-size: 13px;
  line-height: 18px;
  color: ${palette.grey600};
  font-family: Muli, sans-serif;
`

export const StyledLink = styled<{}, 'span'>('span')`
  cursor: pointer;
  margin-left: 2px;
  font-weight: bold;
  display: inline-block;
  color: ${palette.blurple400};
`

export const StyledDisclaimer = styled<{}, 'span'>('span')`
  display: block;
  margin: 10px 0px;
  font-size: 11px;
  font-weight: bold;
  color: ${palette.grey500};
  font-family: Muli, sans-serif;
`
