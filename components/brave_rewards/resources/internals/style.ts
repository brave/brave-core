/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import Heading, { HeadingProps } from 'brave-ui/components/text/heading'
import { ComponentType } from 'react'

export const Wrapper = styled('div')<{}>`
  width: 100%;
  max-width: 1350px;
  margin: 15px auto 0;
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 14px;
`

export const MainTitle = styled(Heading as ComponentType<HeadingProps>)`
  text-align: center;
  margin-bottom: 30px;
`

export const LogTextArea = styled('textarea')<{}>`
  width: 100%;
  height: 80vh;
  margin-top: 10px;
`

export const LogControls = styled('div')<{}>`
  display: flex;
  align-items: self-start;
  justify-content: space-between;
`

export const ButtonWrapper = styled('div')<{}>`
  margin: 0 auto;
  display: flex;
  justify-content: flex-end;
`

export const ButtonGroup = styled('div')<{}>`
  display: flex;

  > button {
    margin-left: 10px;
  }
`

export const Notice = styled('div')<{}>`
  font-size: 12px;
  margin-top: 5px;
`

export const ContributionPublishersWrapper = styled('div')<{}>`
  display: flex;
  flex-wrap: wrap;
  margin: 0 0 10px -20px;
`

export const Publisher = styled('div')<{}>`
  margin: 0 20px 10px;
`

export const PublisherKey = styled('h4')<{}>`
  margin: 15px 0 5px;
`

export const Disclaimer = styled('p')<{}>`
  font-size: 13px;
  color: ${p => p.theme.palette.grey800};
  margin: 0 0 10px;
  padding: 0px;
  text-align: center;
`

export const EventTable = styled('table')<{}>`
  border-spacing: 0;
  margin: 0 auto;
`

export const EventCell = styled('td')<{}>`
  border-bottom: 1px solid ${p => p.theme.palette.blackFade85};
  padding: 10px;
`

export const DiagnosticsEntry = styled('div')<{}>`
  margin: 0 20px 10px;
`
