/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledEnableTipsSection = styled<{}, 'section'>('section')`
  background: #F5F5F9;
  display: block;
  width: 100%;
  padding: 10px 30px;
` as any

export const StyledEnableTipsInner = styled<{}, 'div'>('div')`
  color: #838391;
  font-size: 14px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 28px;
  position: relative;
` as any

export const StyledEnableTips = styled<{}, 'span'>('span')`
  color: #4C54D2;
  font-size: 14px;
  font-weight: 500;
  letter-spacing: 0;
  line-height: 28px;
  margin-right: 5px;
` as any

export const StyledText = styled<{}, 'span'>('span')`
  margin-right: 5px;
` as any

export const StyledProviderImg = styled<{}, 'span'>('span')`
  margin-right: 5px;
  vertical-align: middle;
  width: 20px;
  height: 20px;
  margin-top: -3px;
  display: inline-block;
` as any

export const StyledProviderName = styled<{}, 'span'>('span')`
  font-weight: 600;
  margin-right: 5px;
` as any

export const StyledToggleOuter = styled<{}, 'div'>('div')`
  display: inline-block;
  vertical-align: middle;
  margin-top: 3px;
  text-align: right;
  position: absolute;
  right: 0px;
` as any

export const StyledToggleInner = styled<{}, 'div'>('div')`
  float: right;
  margin-top: 2px;
` as any

export const StyledThumbsUpIcon = styled<{}, 'span'>('span')`
  width: 20px;
  height: 20px;
  display: inline-block;
  margin-top: 3px;
  vertical-align: top;
` as any
