// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

interface Props {
  caption: string
  memory: { [key: string]: string }
}

export class MemoryInfo extends React.Component<Props, {}> {
  render () {
    const items = Object.keys(this.props.memory).map(key => {
      const v = this.props.memory[key]
      return (<div key={key}>{key} : {v}</div>)
    })

    return (<div>
      <h2>{this.props.caption}</h2>
      {items}
    </div>)
  }
}
