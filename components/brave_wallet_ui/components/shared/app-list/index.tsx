// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { AppsListType, BraveWallet } from '../../../constants/types'
import { NavButton } from '../../extension/'
import { AppListItem } from '../'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  CategoryRow,
  CategoryTitle,
  ButtonRow
} from './style'

export interface Props {
  list: AppsListType[]
  favApps: BraveWallet.AppItem[]
  action: () => void
  addToFav: (app: BraveWallet.AppItem) => void
  removeFromFav: (app: BraveWallet.AppItem) => void
}

export default class AppList extends React.PureComponent<Props> {
  checkIsSelected = (app: BraveWallet.AppItem) => {
    return this.props.favApps.some((a) => a.name === app.name)
  }

  toggleFavorite = (app: BraveWallet.AppItem) => () => {
    if (this.checkIsSelected(app)) {
      this.props.removeFromFav(app)
    } else {
      this.props.addToFav(app)
    }
  }

  render () {
    const { list, action } = this.props
    return (
      <StyledWrapper>
        {list.map((option) => (
          <StyledWrapper key={option.category}>
            <CategoryRow>
              <CategoryTitle>{option.category}</CategoryTitle>
            </CategoryRow>
            {option.appList.map((item) =>
              <AppListItem
                key={item.name}
                appInfo={item}
                isStared={this.checkIsSelected(item)}
                toggleFavorite={this.toggleFavorite(item)}
              />
            )}
            {option.category !== getLocale('braveWalletSearchCategory') &&
              <ButtonRow>
                <NavButton
                  disabled={false}
                  text={option.categoryButtonText}
                  onSubmit={action}
                  buttonType='secondary'

                />
              </ButtonRow>
            }
          </StyledWrapper>
        )
        )}
      </StyledWrapper>
    )
  }
}
