/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface StyleProps {
  size?: string
  toggleTips?: boolean
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: block;
` as any

export const StyledContainer = styled<{}, 'div'>('div')`
  padding: 25px 30px 25px 30px;
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
  padding: 15px 0px 6px;
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
  margin-top: 6px;
` as any

export const StyledSelectWrapper = styled<{}, 'div'>('div')`
  width: 80px;
  margin: 2px 0px 0px;
` as any

export const StyledGrid = styled<{}, 'div'>('div')`
  display: flex;
  flex-direction: row;
`

export const StyledColumn = styled<StyleProps, 'div'>('div')`
  flex: ${p => p.size} 0 0;
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
`

export const StyledNoticeLink = styled<StyleProps, 'a'>('a')`
  color: #0089FF;
  font-weight: bold;
  text-decoration: none;
  display: inline-block;
  margin-left: 4px;
`

export const StyledChangeButton = styled<StyleProps, 'button'>('button')`
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: bold;
  background: none;
  border: none;
  padding: 0;
  margin-top: 2px;
`
