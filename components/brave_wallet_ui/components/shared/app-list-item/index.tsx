import * as React from 'react'
import { AppItem } from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  AppDescColumn,
  AppDesctription,
  AppIconWrapper,
  AppIcon,
  AppName,
  IconAndInfo,
  SelectedIcon,
  UnSelectedIcon
} from './style'

export interface Props {
  appInfo: AppItem
  isStared: boolean
  toggleFavorite: () => void
}

export default class AppListItem extends React.PureComponent<Props> {

  getSrc (src?: string) {
    return src ? src : ''
  }

  openApp = () => {
    window.open(this.props.appInfo.url, '_blank')
  }

  render () {
    const { appInfo, isStared, toggleFavorite } = this.props
    return (
      <StyledWrapper>
        <IconAndInfo>
          <AppIconWrapper>
            <AppIcon src={this.getSrc(appInfo.icon)} />
          </AppIconWrapper>
          <AppDescColumn>
            <AppName onClick={this.openApp}>{appInfo.name}</AppName>
            <AppDesctription>{appInfo.description}</AppDesctription>
          </AppDescColumn>
        </IconAndInfo>
        {isStared ? <SelectedIcon onClick={toggleFavorite} /> : <UnSelectedIcon onClick={toggleFavorite} />}
      </StyledWrapper>
    )
  }
}
