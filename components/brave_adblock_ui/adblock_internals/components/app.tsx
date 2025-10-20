// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { MemoryInfo } from './memory_info'
import { EngineRegexList, EngineDebugInfo } from './engine'
import { discardRegexs, saveRegexTexts } from './regex'

class AppState {
  default_engine = new EngineDebugInfo()
  additional_engine = new EngineDebugInfo()
  memory: { [key: string]: string } = {}
}

const EngineInfo = (props: { engine: EngineDebugInfo; caption: string }) => {
  return (
    <div>
      <h2>{props.caption}</h2>
      <div>
        Flatbuffer size:{' '}
        {(props.engine.flatbuffer_size / 1024 / 1024).toFixed(2)} MB
      </div>
      <div>Compiled regexes: {props.engine.compiled_regex_count}</div>
    </div>
  )
}

export class App extends React.Component<{}, AppState> {
  constructor(props: {}) {
    super(props)
    this.state = new AppState()
    this.getDebugInfo()
    setInterval(() => {
      this.getDebugInfo()
    }, 2000)
  }

  getDebugInfo() {
    return sendWithPromise('brave_adblock_internals.getDebugInfo').then(
      this.onGetDebugInfo.bind(this),
    )
  }

  onGetDebugInfo(state: AppState) {
    saveRegexTexts(state.default_engine.regex_data)
    saveRegexTexts(state.additional_engine.regex_data)
    this.setState(state)
  }

  discardAll() {
    discardRegexs(this.state.default_engine.regex_data)
    discardRegexs(this.state.additional_engine.regex_data)
  }

  render() {
    return (
      <div>
        <div>
          <EngineInfo
            engine={this.state.default_engine}
            caption='Default engine'
          />
          <EngineInfo
            engine={this.state.additional_engine}
            caption='Additional engine'
          />
          <MemoryInfo
            key='memory'
            caption='Browser process general memory'
            memory={this.state.memory}
          />
          <input
            type='button'
            value='Show Regexes'
            onClick={() => {
              window.location.href = './?regex_debug'
            }}
          />
          <input
            type='button'
            value='Discard All Regexes'
            onClick={() => {
              this.discardAll()
            }}
          />
        </div>

        {window.location.href.includes('regex_debug') && (
          <div>
            <EngineRegexList
              info={this.state.default_engine}
              caption='Default engine'
            />
            <EngineRegexList
              info={this.state.additional_engine}
              caption='Additional engine'
            />
          </div>
        )}
      </div>
    )
  }
}
