/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../common/locale'

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
          style={{ fontSize: '18px', marginTop: '20px' }}
        >
          {getLocale('customFiltersTitle')}
        </div>
        <div
          style={{ fontWeight: 'bold' }}
        >
          {getLocale('customFiltersInstructions')}
        </div>
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
