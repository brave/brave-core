/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ResourcesListText, ResourcesListScroll, ResourcesFooterFlex } from 'brave-ui/features/shields'

// Shared components
import { Button } from 'brave-ui/components'

// Utils
import { getMessage } from '../../../background/api/localeAPI'

interface Props {
  list: Array<string>
  onClickDismiss: () => void
}

export default class StaticList extends React.PureComponent<Props, {}> {
  render () {
    const { list, onClickDismiss } = this.props
    return (
      <>
        <ResourcesListScroll>
          {list.map((item: string, index: number) => <ResourcesListText title={item} key={index}>{item}</ResourcesListText>)}
        </ResourcesListScroll>
        <ResourcesFooterFlex>
          <Button level='primary' type='accent' text={getMessage('goBack')} onClick={onClickDismiss} />
        </ResourcesFooterFlex>
      </>
    )
  }
}
