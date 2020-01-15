/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  actions: any
  resource: AdBlock.FilterList
}

export class AdBlockItem extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  onChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.enableFilterList(this.props.resource.uuid, event.target.checked)
  }

  render () {
    return (
      <div>
        <label>
          <input
            id={this.props.resource.uuid}
            type='checkbox'
            checked={this.props.resource.enabled}
            onChange={this.onChange}
          />
          {this.props.resource.title}
        </label>
      </div>
    )
  }
}
