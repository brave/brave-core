/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  AllowedScriptsIcon,
  BlockedScriptsIcon,
  DismissOverlay,
  Option,
  Options,
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
} from '../../../../../src/features/shields'

// Shared components
import { Button } from '../../../../../src/components'
import { CaratDownIcon } from '../../../../../src/components/icons'

// Fake data
import locale from '../../fakeLocale'

interface Props {
  list: any[]
  onClickDismiss: () => void
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
    return this.props.list.filter(item => item.blocked === false).length
  }

  get allowedListSize () {
    return this.props.list.filter(item => item.blocked === true).length
  }

  onClickOutsideBounds = () => {
    this.setState({ showApplyOptions: false })
  }

  onClickBlockItem = (event: React.MouseEvent<HTMLAnchorElement>) => {
    console.log('do something')
    console.log('do something for', event.currentTarget.dataset.item)
  }

  onClickAllowItem = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    console.log('do something for', event.currentTarget.dataset.item)
  }

  onClickUndoAction = (event: React.MouseEvent<HTMLAnchorElement>) => {
    console.log('do something for', event.currentTarget.dataset.item)
  }

  onClickApplyOptions = () => {
    this.setState({ showApplyOptions: !this.state.showApplyOptions })
  }

  applyScriptsOnce = () => {
    console.log('do something')
  }

  applyScriptsAlways = () => {
    console.log('do something')
  }

  enabledList = (list: any[]) => {
    return list.map((item, index) => {
      if (item.blocked === false) {
        return null
      }
      return (
        <ResourcesListGrid hightlighted={item.hasUserInput} key={index}>
          {
            item.hasUserInput
            ? <ResourcesListBlockedLink>{locale.blocked}</ResourcesListBlockedLink>
            : <Link data-item={item.index} onClick={this.onClickAllowItem}>{locale.allow}</Link>
          }
          <ResourcesListItem title={item.name}>{item.name}</ResourcesListItem>
          {
            item.hasUserInput
            ? <Link onClick={this.onClickUndoAction}>{locale.undo}</Link>
            : null
          }
        </ResourcesListGrid>
      )
    })
  }

  disabledList = (list: any[]) => {
    return list.map((item, index) => {
      if (item.blocked === true) {
        return null
      }
      return (
        <ResourcesListGrid hightlighted={item.hasUserInput} key={index} >
          {
            item.hasUserInput
            ? <ResourcesListAllowedLink>{locale.allowed}</ResourcesListAllowedLink>
            : <Link onClick={this.onClickBlockItem}>{locale.block}</Link>
          }
          <ResourcesListItem title={item.name}>{item.name}</ResourcesListItem>
          {
            item.hasUserInput
            ? <Link onClick={this.onClickUndoAction}>{locale.undo}</Link>
            : null
          }
        </ResourcesListGrid>
      )
    })
  }

  render () {
    const { showApplyOptions } = this.state
    const { list, onClickDismiss } = this.props
    return (
      <>
        <ResourcesListScroll>
          {showApplyOptions ? <DismissOverlay onClick={this.onClickOutsideBounds} /> : null}
          {/* blocked scripts */}
          <ResourcesSubTitleGrid>
            <BlockedScriptsIcon />
            <ResourcesSubTitle accent='blocked'>
              {locale.blockedScripts} ({this.blockedListSize})
            </ResourcesSubTitle>
            <Link>{locale.allowAll}</Link>
          </ResourcesSubTitleGrid>
          {this.enabledList(list)}

          {/* allowed scripts */}
          <ResourcesSubTitleGrid>
            <AllowedScriptsIcon />
            <ResourcesSubTitle accent='allowed'>
              {locale.allowedScripts} ({this.allowedListSize})
            </ResourcesSubTitle>
            <Link>{locale.blockAll}</Link>
          </ResourcesSubTitleGrid>
          {this.disabledList(list)}

        </ResourcesListScroll>
        <ResourcesFooterGrid>
          <ResourcesFooterGridColumnLeft>
            <Link onClick={onClickDismiss}>{locale.cancel}</Link>
          </ResourcesFooterGridColumnLeft>
          <ResourcesFooterGridColumnRight>
            <Button
              level='primary'
              type='accent'
              text={locale.apply}
              onClick={this.onClickApplyOptions}
              icon={{ position: 'after', image: <CaratDownIcon /> }}
            />
            <Options visible={showApplyOptions}>
              <Option selected={false} onClick={this.applyScriptsOnce}>{locale.applyOnce}</Option>
              <Option selected={false} onClick={this.applyScriptsAlways}>{locale.alwaysApply}</Option>
            </Options>
          </ResourcesFooterGridColumnRight>
        </ResourcesFooterGrid>
      </>
    )
  }
}
