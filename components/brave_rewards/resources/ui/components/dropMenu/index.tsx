/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledAdMenuDropContent,
  StyledAdStatBulletMenuIcon,
  StyledAdMenuOptionDropContent,
  StyledAdMenuOptionDropContentText
} from './style'
import { MoreVertLIcon } from '../../../components/icons'
import { getLocale } from '../../../helpers'

interface State {
  menuOpen: boolean
}

export interface Props {
  id?: string
  currentlySaved: boolean
  currentlyFlagged: boolean
  onMenuSave?: () => void
  onMenuFlag?: () => void
}

export default class DropMenu extends React.PureComponent<Props, State> {
  private container: React.RefObject<HTMLDivElement>
  constructor (props: Props) {
    super(props)
    this.state = {
      menuOpen: false
    }
    this.container = React.createRef<HTMLDivElement>()
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutside)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutside)
  }

  handleClickOutside = (event: MouseEvent) => {
    if (this.container.current && !this.container.current.contains(event.target as Element)) {
      this.setState({
        menuOpen: false
      })
    }
  }

  showMenu = () => {
    this.setState({
      menuOpen: !this.state.menuOpen
    })
  }

  render () {
    const { onMenuFlag, onMenuSave, currentlySaved, currentlyFlagged } = this.props
    return (
      <StyledAdStatBulletMenuIcon onClick={this.showMenu}>
        <div ref={this.container}>
          <MoreVertLIcon />
          {
            this.state.menuOpen ?
              <StyledAdMenuDropContent>
                <StyledAdMenuOptionDropContent>
                  {
                    !currentlySaved ?
                    <StyledAdMenuOptionDropContentText onClick={onMenuSave}>
                      {
                        getLocale('saveAd')
                      }
                    </StyledAdMenuOptionDropContentText>
                    :
                    <StyledAdMenuOptionDropContentText onClick={onMenuSave}>
                      {
                        getLocale('removeAdFromSaved')
                      }
                    </StyledAdMenuOptionDropContentText>
                  }
                </StyledAdMenuOptionDropContent>
                <StyledAdMenuOptionDropContent>
                  {
                    !currentlyFlagged ?
                    <StyledAdMenuOptionDropContentText onClick={onMenuFlag}>
                      {
                        getLocale('markAsInappropriate')
                      }
                    </StyledAdMenuOptionDropContentText>
                    :
                    <StyledAdMenuOptionDropContentText onClick={onMenuFlag}>
                      {
                        getLocale('markAsInappropriateChecked')
                      }
                    </StyledAdMenuOptionDropContentText>
                  }
                </StyledAdMenuOptionDropContent>
              </StyledAdMenuDropContent>
              :
              null
          }
        </div>
      </StyledAdStatBulletMenuIcon>
    )
  }
}
