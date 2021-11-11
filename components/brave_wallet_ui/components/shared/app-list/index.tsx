import * as React from 'react'
import { AppItem, AppsListType } from '../../../constants/types'
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
  favApps: AppItem[]
  action: () => void
  addToFav: (app: AppItem) => void
  removeFromFav: (app: AppItem) => void
}

export default class AppList extends React.PureComponent<Props> {

  checkIsSelected = (app: AppItem) => {
    return this.props.favApps.some((a) => a.name === app.name)
  }

  toggleFavorite = (app: AppItem) => () => {
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
