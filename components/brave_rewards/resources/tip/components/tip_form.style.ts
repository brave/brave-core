/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  flex-direction: column;
  height: 100%;
`

export const loading = styled.div``

export const header = styled.div`
  text-align: center;
  padding-top: 27px;
  font-size: 16px;
  font-weight: 600;
  line-height: 24px;
`

export const headerSubtitle = styled.div`
  padding-top: 2px;
  font-weight: normal;
`

export const socialIcon = styled.span`
  display: inline-block;
  width: 24px;
  height: 24px;
  vertical-align: middle;
  margin-bottom: 3px;
`

export const tipKind = styled.div`
  padding: 22px 32px 0;
  align-self: center;
  width: 100%;
  max-width: 450px;
  min-height: 5px;
`

export const monthlyIndicator = styled.div`
  text-align: center;
  padding-bottom: 4px;
  margin-top: -20px;
  font-size: 12px;
  line-height: 16px;
  color: var(--brave-palette-neutral600);
`

export const monthlyIndicatorStar = styled.span`
  color: var(--brave-color-brandBatInteracting);
`

export const main = styled.div`
  flex: 1 0 auto;
  margin-top: 17px;
  margin-left: 0;
`

export const tour = styled.div`
  padding: 46px 40px 20px;
  height: 100%;
`
