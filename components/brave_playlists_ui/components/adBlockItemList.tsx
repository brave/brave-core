/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { AdBlockItem } from './adBlockItem'

interface Props {
  actions: any
  resources: AdBlock.FilterList[]
}

export class AdBlockItemList extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    const regionalLists = this.props.resources.map((resource) => (
      <AdBlockItem
        actions={this.props.actions}
        key={resource.uuid}
        resource={resource}
      />)
    )
    return (
      <div>
        <div
          i18n-content='additionalFiltersTitle'
          style={{ fontSize: '18px', marginTop: '20px' }}
        />
        <div
          i18n-content='additionalFiltersWarning'
          style={{ fontWeight: 'bold' }}
        />
        <div style={{ marginTop: '10px' }} >
          {regionalLists}
        </div>
      </div>
    )
  }
}
