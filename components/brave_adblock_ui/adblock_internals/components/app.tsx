// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { MemoryInfo } from './memory_info'
import { EngineRegexList, EngineDebugInfo, SourceInfo } from './engine'
import { discardRegexs, saveRegexTexts } from './regex'

class AppState {
  default_engine = new EngineDebugInfo()
  additional_engine = new EngineDebugInfo()
  memory: { [key: string]: string } = {}
  debug_mode: boolean = false
}

const EngineSourceInfo = (props: { source_info: SourceInfo[] }) => {
  return (
    <details>
      <summary>+ Sources ({props.source_info.length})</summary>
      <table>
        <thead>
          <tr>
            <th>Title</th>
            <th>Homepage</th>
            <th>Network filters</th>
            <th>Cosmetic filters</th>
            <th>Parse errors</th>
          </tr>
        </thead>
        <tbody>
          {props.source_info.map((sourceInfo, index) => (
            <tr key={index}>
              <td>{sourceInfo.title}</td>
              <td>
                {sourceInfo.homepage && (
                  <a
                    href={sourceInfo.homepage}
                    target='_blank' rel='noopener noreferrer'
                  >
                    {sourceInfo.homepage}
                  </a>
                )}
              </td>
              <td>{sourceInfo.network_filter_count}</td>
              <td>{sourceInfo.cosmetic_filter_count}</td>
              <td>{sourceInfo.parse_error_count}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </details>
  )
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
      <EngineSourceInfo source_info={props.engine.source_info} />
    </div>
  )
}

export const App = () => {
  const [state, setState] = React.useState(new AppState())
  const [showRegexes, setShowRegexes] = React.useState(false)
  const getDebugInfo = () => {
    return sendWithPromise('brave_adblock_internals.getDebugInfo').then(
      (newState: AppState) => {
        saveRegexTexts(newState.default_engine.regex_data)
        saveRegexTexts(newState.additional_engine.regex_data)
        setState(newState)
      },
    )
  }

  const discardAll = () => {
    discardRegexs(state.default_engine.regex_data)
    discardRegexs(state.additional_engine.regex_data)
  }

  React.useEffect(() => {
    getDebugInfo()
    const interval = setInterval(() => {
      getDebugInfo()
    }, 2000)
    return () => {
      clearInterval(interval)
    }
  }, [])

  return (
    <>
      <div>
        <EngineInfo
          engine={state.default_engine}
          caption='Default engine'
        />
        <EngineInfo
          engine={state.additional_engine}
          caption='Additional engine'
        />
        <MemoryInfo
          key='memory'
          caption='Browser process general memory'
          memory={state.memory}
        />
        <div>
          <h2>Flags</h2>
          <input
            type='checkbox'
            checked={state.debug_mode}
            onChange={() => {
              chrome.send('brave_adblock_internals.setDebugMode', [
                !state.debug_mode,
              ])
              const warning = document.getElementById('debug-mode-warning')
              warning!.innerText = 'Restart browser to apply changes'
              getDebugInfo()
            }}
          />
          <label>Debug mode (<span id='debug-mode-warning'>WARNING: extra memory and CPU costs</span>)</label>
        </div>
        <div>
          <h2>Regex debug</h2>
          <input
            type='button'
            value={`${showRegexes ? 'Hide' : 'Show'} Regexes`}
            onClick={() => {
              setShowRegexes(!showRegexes)
            }}
          />
          <input
            type='button'
            value='Discard All Regexes'
            onClick={() => {
              discardAll()
            }}
          />
        </div>
      </div>

      {showRegexes && (
        <div>
          <EngineRegexList
            info={state.default_engine}
            caption='Default engine'
          />
          <EngineRegexList
            info={state.additional_engine}
            caption='Additional engine'
          />
        </div>
      )}
    </>
  )
}
