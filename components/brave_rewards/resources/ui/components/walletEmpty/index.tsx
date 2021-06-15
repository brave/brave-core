/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContent, StyledCenter } from './style'
import { getLocale } from 'brave-ui/helpers'
import coins from './assets/coins'

interface Props {
  id?: string
  hideAddFundsText?: boolean
}

export default class WalletEmpty extends React.PureComponent<Props, {}> {
  getEmptyWalletMessage = () => {
    return (
      <>
        <b>{getLocale('rewardsPanelEmptyText2')}</b><br/>
        • {getLocale('rewardsPanelEmptyText3')}<br/>
        • {getLocale('rewardsPanelEmptyText4')}<br/>
        • {getLocale('rewardsPanelEmptyText5')}
      </>
    )
  }

  render () {
    const { id, hideAddFundsText } = this.props

    return (
      <StyledWrapper id={id}>
        {coins}
        <StyledTitle>
          {getLocale('rewardsPanelEmptyText1')}
        </StyledTitle>
        <StyledContent>
          {
            hideAddFundsText
            ? <StyledCenter>
                {getLocale('rewardsPanelEmptyText5')}
              </StyledCenter>
            : this.getEmptyWalletMessage()
          }
        </StyledContent>
      </StyledWrapper>
    )
  }
}
