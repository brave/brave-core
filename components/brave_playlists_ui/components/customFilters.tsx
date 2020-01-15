/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  actions: any,
  rules: string
}

export class CustomFilters extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  onChangeCustomFilters = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.props.actions.updateCustomFilters(event.target.value)
  }

  render () {
    return (
      <div>
        <div
          i18n-content='customFiltersTitle'
          style={{ fontSize: '18px', marginTop: '20px' }}
        />
        <div
          i18n-content='customFiltersInstructions'
          style={{ fontWeight: 'bold' }}
        />
        <textarea
          style={{ marginTop: '10px' }}
          data-test-id={'customFiltersInput'}
          cols={100}
          rows={10}
          spellCheck={false}
          value={this.props.rules}
          onChange={this.onChangeCustomFilters}
        />
      </div>
    )
  }
}
