/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as Styled from './style'

// Shared components
import { Card } from 'brave-ui'

import { getLocale } from 'brave-ui/helpers'
import { AlertCircleIcon } from 'brave-ui/components/icons'
import { Button } from 'brave-ui/components'

export interface Props {
  amount: string
  onAction: () => void
}

export default class GrantTransitionBanner extends React.PureComponent<Props, {}> {
  render () {
    const amount = parseFloat(this.props.amount).toFixed(1)

    return (
      <Styled.Wrapper>
        <Card emphasis={'60'}>
          <Styled.Content>
            <Styled.Icon>
              <AlertCircleIcon />
            </Styled.Icon>
            <Styled.Text>
              <Styled.Read>
                {getLocale('transitionBannerRead')}
              </Styled.Read>
              <Styled.Body>
                <span dangerouslySetInnerHTML={{ __html: getLocale('transitionBannerBody', { amount }) }} />
              </Styled.Body>
            </Styled.Text>
            <Styled.Button>
              <Button
                level={'primary'}
                type={'accent'}
                text={getLocale('transitionBannerCTA')}
                onClick={this.props.onAction}
              />
            </Styled.Button>
          </Styled.Content>
        </Card>
      </Styled.Wrapper>
    )
  }
}
