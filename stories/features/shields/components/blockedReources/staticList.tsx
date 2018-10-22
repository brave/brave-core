/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { ResourcesListText, ResourcesListScroll, ResourcesFooter } from '../../../../../src/features/shields'

// Shared components
import { Button } from '../../../../../src/components'

interface Props {
  list: any[]
  onClickDismiss: () => void
}

export default class StaticList extends React.PureComponent<Props, {}> {
  render () {
    const { list, onClickDismiss } = this.props
    return (
      <>
        <ResourcesListScroll>
          {list.map((item, index) => <ResourcesListText key={index}>{item}</ResourcesListText>)}
        </ResourcesListScroll>
        <ResourcesFooter>
          <Button level='primary' type='accent' text='Go Back' onClick={onClickDismiss} />
        </ResourcesFooter>
      </>
    )
  }
}
