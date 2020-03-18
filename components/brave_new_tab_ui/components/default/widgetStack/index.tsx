/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget'

class Stack extends React.PureComponent<{}, {}> {

  render () {
    return (
      <>
        {this.props.children}
      </>
    )
  }
}

export const WidgetStack = createWidget(Stack)
