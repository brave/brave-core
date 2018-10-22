/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Shared components
import { Button } from '../../../../../src/components'

interface Props {
  list: Array<string>
}

export default class StaticList extends React.PureComponent<Props, {}> {
  render () {
    const { list } = this.props
    return (
      <>
        <section>
          <div>{list.map((item, index) => <div key={index}>{item}</div>)}</div>
        </section>
        <footer>
          <Button text='Go Back' />
        </footer>
      </>
    )
  }
}
