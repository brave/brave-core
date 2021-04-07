import * as React from 'react'
import { AppObjectType, AppsListType } from '../../../constants/types'
import { NavButton } from '../../extension/'
import { AppListItem } from '../'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  CategoryRow,
  CategoryTitle,
  ButtonRow
} from './style'

export interface Props {
  list: AppsListType[]
  favApps: AppObjectType[]
  action: () => void
  addToFav: (app: AppObjectType) => void
  removeFromFav: (app: AppObjectType) => void
}

export default class AppList extends React.PureComponent<Props> {

  checkIsSelected = (app: AppObjectType) => {
    return this.props.favApps.some((a) => a.name === app.name)
  }

  toggleFavorite = (app: AppObjectType) => () => {
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
            {option.category !== locale.searchCategory &&
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
