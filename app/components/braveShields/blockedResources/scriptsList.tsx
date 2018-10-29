/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  AllowedScriptsIcon,
  BlockedScriptsIcon,
  DismissOverlay,
  ResourcesListScroll,
  ResourcesSubTitle,
  ResourcesFooterGrid,
  ResourcesFooterGridColumnLeft,
  ResourcesFooterGridColumnRight,
  ResourcesListItem,
  Link,
  ResourcesListAllowedLink,
  ResourcesListBlockedLink,
  ResourcesSubTitleGrid,
  ResourcesListGrid
} from 'brave-ui/features/shields'

// Shared components
import { Button } from 'brave-ui/components'

// Types
import * as shieldActions from '../../../types/actions/shieldsPanelActions'
import { BlockJSOptions } from '../../../types/other/blockTypes'
import { NoScriptInfo } from '../../../types/other/noScriptInfo'

// Utils
import { getMessage } from '../../../background/api/localeAPI'

interface Props {
  origin: string
  noScriptInfo: NoScriptInfo
  onClickDismiss: () => void
  javascript: BlockJSOptions
  blockJavaScript: shieldActions.BlockJavaScript
  allowScriptOriginsOnce: shieldActions.AllowScriptOriginsOnce
  changeNoScriptSettings: shieldActions.ChangeNoScriptSettings
  changeAllNoScriptSettings: shieldActions.ChangeAllNoScriptSettings
}

interface State {
  showApplyOptions: boolean
}

export default class StaticList extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { showApplyOptions: false }
  }

  get blockedListSize () {
    const { noScriptInfo } = this.props
    return Object.keys(noScriptInfo)
      .filter((origin: string) => noScriptInfo[origin].willBlock === true).length
  }

  get allowedListSize () {
    const { noScriptInfo } = this.props
    return Object.keys(noScriptInfo)
      .filter((origin: string) => noScriptInfo[origin].willBlock === false).length
  }

  onClickOutsideBounds = () => {
    this.setState({ showApplyOptions: false })
  }

  onClickToggleBlockOrAllowScript = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.props.changeNoScriptSettings(event.currentTarget.id)
  }

  onClickBlockAll = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    const { origin } = this.props
    this.props.changeAllNoScriptSettings(origin, true)
  }

  onClickAllowAll = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    const { origin } = this.props
    this.props.changeAllNoScriptSettings(origin, false)
    this.setState({ showApplyOptions: false })
  }

  onClickApplyScriptsOnce = () => {
    const { noScriptInfo } = this.props
    const allOrigins = Object.keys(noScriptInfo)
    this.props.allowScriptOriginsOnce(allOrigins.filter(key => noScriptInfo[key].willBlock === false))
  }

  renderEnabledList = (list: NoScriptInfo) => {
    return Object.keys(list).map((origin: string, key: number) => {
      if (list[origin].willBlock === false) {
        return null
      }
      return (
        <ResourcesListGrid hightlighted={true} key={key}>
          {
            list[origin].willBlock
              ? <ResourcesListBlockedLink>{getMessage('blocked')}</ResourcesListBlockedLink>
              : <Link id={origin} onClick={this.onClickToggleBlockOrAllowScript}>{getMessage('allow')}</Link>
          }
          <ResourcesListItem title={origin}>{origin}</ResourcesListItem>
          {
            list[origin].willBlock
              ? <Link id={origin} onClick={this.onClickToggleBlockOrAllowScript}>{getMessage('undo')}</Link>
              : null
          }
        </ResourcesListGrid>
      )
    })
  }

  renderDisabledList = (list: NoScriptInfo) => {
    return Object.keys(list).map((origin: string, key: number) => {
      if (list[origin].willBlock === true) {
        return null
      }
      return (
        <ResourcesListGrid hightlighted={list[origin].willBlock} key={key}>
          {
            list[origin].willBlock
              ? <ResourcesListAllowedLink>{getMessage('allowed')}</ResourcesListAllowedLink>
              : <Link id={origin} onClick={this.onClickToggleBlockOrAllowScript}>{getMessage('block')}</Link>
          }
          <ResourcesListItem title={origin}>{origin}</ResourcesListItem>
          {
            list[origin].willBlock
              ? <Link id={origin} onClick={this.onClickToggleBlockOrAllowScript}>{getMessage('undo')}</Link>
            : null
          }
        </ResourcesListGrid>
      )
    })
  }

  render () {
    const { showApplyOptions } = this.state
    const { noScriptInfo, onClickDismiss } = this.props
    return (
      <>
        <ResourcesListScroll>
          {showApplyOptions ? <DismissOverlay onClick={this.onClickOutsideBounds} /> : null}
          {/* blocked scripts */}
          <ResourcesSubTitleGrid>
            <BlockedScriptsIcon />
            <ResourcesSubTitle accent='blocked'>
              {getMessage('blockedScripts')} ({this.blockedListSize})
            </ResourcesSubTitle>
            <Link onClick={this.onClickAllowAll}>{getMessage('allowAll')}</Link>
          </ResourcesSubTitleGrid>
          {this.renderEnabledList(noScriptInfo)}

          {/* allowed scripts */}
          <ResourcesSubTitleGrid>
            <AllowedScriptsIcon />
            <ResourcesSubTitle accent='allowed'>
              {getMessage('allowedScripts')} ({this.allowedListSize})
            </ResourcesSubTitle>
            <Link onClick={this.onClickBlockAll}>{getMessage('blockAll')}</Link>
          </ResourcesSubTitleGrid>
          {this.renderDisabledList(noScriptInfo)}

        </ResourcesListScroll>
        <ResourcesFooterGrid>
          <ResourcesFooterGridColumnLeft>
            <Link onClick={onClickDismiss}>{getMessage('cancel')}</Link>
          </ResourcesFooterGridColumnLeft>
          <ResourcesFooterGridColumnRight>
            <Button
              level='primary'
              type='accent'
              text={getMessage('applyOnce')}
              onClick={this.onClickApplyScriptsOnce}
            />
          </ResourcesFooterGridColumnRight>
        </ResourcesFooterGrid>
      </>
    )
  }
}
