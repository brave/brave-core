/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled<{}, 'div'>('div')`
  display: block;
` as any

export const StyledContainer = styled<{}, 'div'>('div')`
  padding: 25px 30px 15px 30px;
` as any

export const StyledAttentionScore = styled<{}, 'span'>('span')`
  margin-left: 17px;
  font-weight: 300;
  color: #4B4C5C;
  font-size: 14px;
` as any

export const StyledAttentionScoreTitle = styled<{}, 'span'>('span')`
  margin: 0;
  font-weight: 300;
  color: #4B4C5C;
  font-size: 14px;
  letter-spacing: 0;
  margin-left: 10px;
` as any

export const StyledScoreWrapper = styled<{}, 'section'>('section')`
  padding: 15px 0px 6px;
` as any

export const StyledControlsWrapper = styled<{}, 'section'>('section')`
  padding: 5px 0px;
  border-top: 1px solid #DBDFE3;
  border-bottom: 1px solid #DBDFE3;
  min-height: 70px;
` as any

export const StyledDonateText = styled<{}, 'span'>('span')`
  display: inline-block;
  color: #838391;
  font-size: 12px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 26px;
  margin-left: 10px;
` as any

export const StyledIcon = styled<{}, 'span'>('span')`
  width: 15px;
  margin-left: 5px;
  vertical-align: middle;
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
  margin: 4px 0px 0px;
` as any
