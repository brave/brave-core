// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { MemoryInfo } from './memory_info'
import { Engine, EngineDebugInfo } from './engine'
import { discardRegexs, saveRegexTexts } from './regex'

class AppState {
  default_engine = new EngineDebugInfo()
  additional_engine = new EngineDebugInfo()
  memory: { [key: string]: string } = {}
}

export class App extends React.Component<{}, AppState> {
  constructor (props: {}) {
    super(props)
    this.state = new AppState()
    this.getDebugInfo()
    setInterval(() => { this.getDebugInfo() }, 2000)
  }

  getDebugInfo () {
    return sendWithPromise('brave_adblock_internals.getDebugInfo').then(
      this.onGetDebugInfo.bind(this))
  }

  onGetDebugInfo (state: AppState) {
    saveRegexTexts(state.default_engine.regex_data)
    saveRegexTexts(state.additional_engine.regex_data)
    this.setState(state)
  }

  discardAll () {
    discardRegexs(this.state.default_engine.regex_data)
    discardRegexs(this.state.additional_engine.regex_data)
  }

  render () {
    return (
      <div>
        <MemoryInfo key="memory" caption="Browser process memory" memory={this.state.memory} />
        <input type="button" value="Discard All Regex" onClick={() => { this.discardAll() }} />
        <Engine key="default_engine" caption="Default engine" info={this.state.default_engine} />
        <Engine key="additional_engine" caption="Additional engine" info={this.state.additional_engine} />
      </div>
    )
  }
}
