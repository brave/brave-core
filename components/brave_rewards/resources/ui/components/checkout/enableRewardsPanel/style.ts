/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import { ComponentType } from 'react'

export const Container = styled.div`
  text-align: center;
  min-height: 485px;
  padding-top: 90px;
`

export const Title = styled.h1`
  font-size: 16px;
  font-weight: 500;
  margin: 0;
`

export const Text = styled.p`
  font-size: 14px;
  line-height: 22px;
  max-width: 300px;
  margin: 20px auto 0;
`

export const LearnMore = styled.div`
  font-size: 14px;
  line-height: 22px;
`

export const EnableRewardsButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 52px auto 0;
  min-width: 265px;
`

export const TermsOfService = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 13px;
  line-height: 16px;
  max-width: 282px;
  margin: 24px auto 0;

  a {
    font-weight: bold;
    color: ${p => p.theme.palette.black};
  }
`
