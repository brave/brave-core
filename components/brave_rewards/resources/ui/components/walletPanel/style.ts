/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

import { CaratStrongDownIcon } from 'brave-ui/components/icons'

interface StyleProps {
  size?: string
  shrink?: string
  toggleTips?: boolean
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: block;
` as any

export const StyledProfileWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 15px;
` as any

export const StyledContainer = styled<{}, 'div'>('div')`
  min-height: 250px;
  padding: 15px 30px 20px 30px;
` as any

export const StyledAttentionScore = styled<{}, 'span'>('span')`
  margin-left: 30px;
  font-weight: 500;
  color: #4B4C5C;
  font-size: 14px;
` as any

export const StyledAttentionScoreTitle = styled<{}, 'span'>('span')`
  font-weight: 400;
  color: #4B4C5C;
  font-size: 14px;
  letter-spacing: 0;
  margin: 0 0 0 2px;
` as any

export const StyledScoreWrapper = styled<{}, 'section'>('section')`
  padding: 0 0px 6px;
` as any

export const StyledControlsWrapper = styled<{}, 'section'>('section')`
  padding: 5px 0px;
  border-top: 1px solid #DBDFE3;
  border-bottom: 1px solid #DBDFE3;
` as any

export const StyledDonateText = styled<{}, 'span'>('span')`
  display: inline-block;
  font-size: 14px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 26px;
  margin-left: 2px;
  color: ${p => p.theme.color.subtleInteracting};
` as any

export const StyledDonateWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  padding: 15px 0 0;
  margin: 0 auto;
` as any

export const StyledToggleWrapper = styled<{}, 'div'>('div')`
  margin-top: 4px;
` as any

export const StyledSelectWrapper = styled<{}, 'div'>('div')`
  width: 87px;
  margin: 2px 0px 0px;
` as any

export const StyledGrid = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: row;
`

export const StyledColumn = styled<StyleProps, 'div'>('div')`
  flex: ${p => p.size} ${p => p.shrink || '0'} auto;
`

export const StyleToggleTips = styled<StyleProps, 'div'>('div')`
  display: ${p => p.toggleTips ? 'flex' : 'none'};
`

export const StyledNoticeWrapper = styled<StyleProps, 'div'>('div')`
  background: rgba(0, 0, 0, 0.04);
  color: #676283;
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: normal;
  letter-spacing: 0;
  line-height: 16px;
  padding: 10px 12px;
  border-radius: 4px;
  margin: 11px 0 10px;
  max-height: 84px;
  overflow-y: auto;
`

export const StyledNoticeLink = styled<StyleProps, 'a'>('a')`
  color: ${palette.blue400};
  font-weight: bold;
  text-decoration: none;
  display: inline-block;
`

export const StyledMonthlyWrapper = styled<StyleProps, 'div'>('div')`
  overflow-y: visible;
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 14px;
  color: ${p => p.theme.color.black};
  line-height: 21px;
  height: 27px;
`

export const StyledMonthlyBorder = styled<StyleProps, 'div'>('div')`
  border: solid 1px rgba(76, 84, 210, 0.55);
  border-radius: 12px;
  background: ${palette.white};
  position: relative;
  z-index: 1;

  &.expanded {
    box-shadow: 1px 1px 6px 0px rgba(0, 0, 0, .1);
    padding-top: 4px;
  }
`

export const StyledMonthlyAmount = styled<StyleProps, 'div'>('div')`
  button {
    background: none;
    border: none;
    width: 100%;
    text-align: left;
    padding: 2px 26px 1px 12px;
    outline-style: none;
  }

  button:focus-visible {
    outline-style: auto;
  }
`

export const StyledMonthlyDownIcon = styled(CaratStrongDownIcon)`
  color: ${palette.black};
  width: 10px;
  height: 10px;
  vertical-align: middle;
  position: absolute;
  top: 8px;
  right: 7px;
`

export const StyledMonthlyActions = styled<StyleProps, 'div'>('div')`
  position: relative;
  margin-top: 5px;
  margin-bottom: 3px;
  padding-top: 2px;

  &:before {
    content: ' ';
    display: block;
    height: 0;
    top: -3px;
    left: 12px;
    right: 12px;
    position: absolute;
    border-top: solid 1px #DBDFE3;
  }

  button {
    display: block;
    padding: 3px 12px;
    color: inherit;
    text-decoration: none;
    cursor: pointer;
    background: none;
    border: none;
    width: 100%;
    text-align: left;
  }

  button:hover {
    color: ${palette.blurple500};
  }
`

export const StyledSetButtonContainer = styled<{}, 'div'>('div')`
  text-align: right;
`

export const StyledSetButton = styled<{}, 'button'>('button')`
  color: ${palette.blurple500};
  font-size: 13px;
  border: 1px solid ${palette.blurple500};
  border-radius: 20px;
  padding: 4px 15px;
  cursor: pointer;
`
