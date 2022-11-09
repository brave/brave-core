/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import * as React from 'react'
import {
  StyledWrapper,
  StyledLink
} from './style'
import { getLocaleWithTags } from '../../../../../common/locale'

export interface Props {
  testId?: string
  title: string
  className?: string
}

export default class TOSAndPP extends React.PureComponent<Props> {
  render () {
    const { testId, title, className } = this.props

    const text = getLocaleWithTags('tosAndPp', 2, { title })
    if (text.length !== 2) {
      return null
    }

    return (
      <StyledWrapper data-test-id={testId} className={className} >
        {text[0].beforeTag}
        <StyledLink href={'https://basicattentiontoken.org/user-terms-of-service'} target={'_blank'}>
          {text[0].duringTag}
        </StyledLink>
        {text[1].beforeTag}
        <StyledLink href={'https://brave.com/privacy/#rewards'} target={'_blank'}>
          {text[1].duringTag}
        </StyledLink>
        {text[1].afterTag}
      </StyledWrapper>
    )
  }
}
