/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  AllowedScriptsIcon,
  BlockedScriptsIcon,
  ResourcesListScroll,
  ResourcesBlockedLabelGrid,
  ResourcesBlockedLabelGrid2,
  ResourcesLabelScripts,
  ResourcesFooterGrid,
  ResourcesFooterGridColumn1,
  ResourcesFooterGridColumn2,
  ResourcesBlockedLabel2,
  ResourcesLink,
  ResourcesLabelScriptsAllowed,
  ResourcesLabelScriptsBlocked,
  ResourcesLinkUndo
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

export default class StaticList extends React.PureComponent<Props, {}> {
  get blockedListSize () {
    return this.props.list.filter(item => item.blocked === false).length
  }

  get allowedListSize () {
    return this.props.list.filter(item => item.blocked === true).length
  }

  render () {
    const { list, onClickDismiss } = this.props
    return (
      <>
        <ResourcesListScroll>
          {/* blocked scripts */}
          <ResourcesBlockedLabelGrid>
            <BlockedScriptsIcon />
            <ResourcesLabelScripts accent='blocked'>
              {locale.blockedScripts} ({this.blockedListSize})
            </ResourcesLabelScripts>
            <ResourcesLink>{locale.allowAll}</ResourcesLink>
          </ResourcesBlockedLabelGrid>
          {list.map((item, index) => {
            if (item.blocked === false) {
              return null
            }
            return (
              <ResourcesBlockedLabelGrid2 key={index}>
                <ResourcesLink>{locale.allow}</ResourcesLink>
                <ResourcesLabelScriptsAllowed>{locale.allowed}</ResourcesLabelScriptsAllowed>
                <ResourcesBlockedLabel2>{item.name}</ResourcesBlockedLabel2>
                <ResourcesLinkUndo>{locale.undo}</ResourcesLinkUndo>
              </ResourcesBlockedLabelGrid2>
              )
          })}
          {/* allowed scripts */}
          <ResourcesBlockedLabelGrid>
            <AllowedScriptsIcon />
            <ResourcesLabelScripts accent='allowed'>
              {locale.allowedScripts}  ({this.allowedListSize})
            </ResourcesLabelScripts>
            <ResourcesLink>{locale.blockAll}</ResourcesLink>
          </ResourcesBlockedLabelGrid>
          {list.map((item, index) => {
            if (item.blocked === true) {
              return null
            }
            return (
              <ResourcesBlockedLabelGrid2 key={index}>
                <ResourcesLink>{locale.block}</ResourcesLink>
                <ResourcesLabelScriptsBlocked>{locale.blocked}</ResourcesLabelScriptsBlocked>
                <ResourcesBlockedLabel2>{item.name}</ResourcesBlockedLabel2>
                <ResourcesLinkUndo>{locale.undo}</ResourcesLinkUndo>
              </ResourcesBlockedLabelGrid2>
              )
          })}
        </ResourcesListScroll>
        <ResourcesFooterGrid>
          <ResourcesFooterGridColumn1>
            <ResourcesLink onClick={onClickDismiss}>{locale.cancel}</ResourcesLink>
          </ResourcesFooterGridColumn1>
          <ResourcesFooterGridColumn2>
            <Button
              level='primary'
              type='accent'
              text={locale.apply}
              onClick={onClickDismiss}
              icon={{ position: 'after', image: <CaratDownIcon />}}
            />
          </ResourcesFooterGridColumn2>
        </ResourcesFooterGrid>
      </>
    )
  }
}
