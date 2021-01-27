/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Heading from 'brave-ui/components/text/heading'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import { ComponentType } from 'react'

interface StyleProps {
  compact?: boolean
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.heading};
  background-image: linear-gradient(180deg, ${p => p.theme.palette.blurple500} 0%, #563195 100%);
  padding: 42px 42px 30px;
  margin: -48px;
  position: relative;
`

export const StyledClose = styled<{}, 'div'>('div')`
  position: absolute;
  top: 20px;
  right: 20px;
  cursor: pointer;
  width: 15px;
  height: 15px;
  color: ${p => p.theme.palette.white};
`

export const StyledHeader = styled<StyleProps, 'div'>('div')`
  margin-bottom: 40px;
  padding-left: 26px;
`

export const StyledWalletIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: middle;
  width: 50px;
  height: 50px;
`

export const StyledHeaderText = styled<{}, 'div'>('div')`
  vertical-align: middle;
  display: inline-block;
  padding: 3px 0 0 22px;
`

export const StyledTitle = styled(Heading)`
  color: ${p => p.theme.palette.white};
  font-size: 22px;
  line-height: 24px;
  font-weight: 600;
`

export const StyledListTitle = styled(Heading)`
  font-weight: 600;
  font-size: 17px;
  line-height: 25px;
  padding: 0;
  color: ${p => p.theme.palette.white};
`

export const StyledListItem = styled<StyleProps, 'div'>('div')`
  color: ${p => p.theme.palette.whiteFade15};
  margin: 17px 0;
`

export const StyledListIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 24px;
  height: 24px;
  vertical-align: top;
`

export const StyledListItemText = styled<{}, 'div'>('div')`
  vertical-align: middle;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 15px;
  line-height: 20px;
  display: inline-block;
  width: calc(100% - 30px);
  padding-left: 12px;
`

export const StyledButton = styled(Button as ComponentType<ButtonProps>)`
  padding: 11px 40px;
  width: auto;
  min-height: auto;
  margin-top: -25px;
`

export const StyledFooter = styled<StyleProps, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  color: ${p => p.theme.palette.whiteFade30};
  margin-top: 32px;
`

export const StyledFooterIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: middle;
  width: 20px;
  height: 20px;
  .icon {
    margin-left: 2px;
  }
`

export const StyledContent = styled<{}, 'div'>('div')`
  display: flex;
  margin-bottom: 10px;
`

export const StyledLeftSide = styled<{}, 'div'>('div')`
  width: 50%;
  padding-right: 10px;
  margin-top: 9px;
`

export const StyledRightSide = styled<{}, 'div'>('div')`
  width: 50%;
  padding-left: 40px;
`

export const StyledNote = styled<{}, 'div'>('div')`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 12px;
  line-height: 20px;
  color: ${p => p.theme.palette.whiteFade30};
  margin-top: 15px;
`

export const NoteText = styled<{}, 'div'>('div')`
  color: ${p => p.theme.palette.whiteFade15};
  font-size: 12px;
  margin-top: 15px;
`
