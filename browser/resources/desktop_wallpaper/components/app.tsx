// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import Button from '@brave/leo/react/button'
import DropDown from '@brave/leo/react/dropdown'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import Checkbox from '@brave/leo/react/checkbox'

import { useImagePreview } from '../hooks/useImagePreview'
import { useDesktopWallpaper } from '../hooks/useDesktopWallpaper'

import '@brave/leo/tokens/css/variables.css'
import './app.css'

const FIT_OPTIONS = new Map([
  ['fill', 'Fill'],
  ['fit', 'Fit'],
  ['stretch', 'Stretch'],
  ['center', 'Center'],
])

export function App() {
  const {
    status,
    fit,
    monitor,
    useAllMonitors,
    imageSource,
    displays,
    handleApply,
    handleCancel,
    handleFitOnChange,
    handleUseBackgroundOnChange,
    handleMonitorOnChange,
  } = useDesktopWallpaper('1')

  const { monitorWidth, monitorHeight, containerRef, getBackgroundSize } =
    useImagePreview(imageSource, monitor, displays, fit)

  return (
    <div className='dw-card'>
      <div className='dw-modal-layout'>
        {/* Loading */}
        {status.type === 'loading' && (
          <div className='dw-loading-section'>
            <ProgressRing mode='indeterminate' />
          </div>
        )}
        {/* Error */}
        {status.type === 'error' && (
          <div className='dw-generic-section'>
            <section>
              <Icon
                title={status.message}
                name='dangerous-filled'
                className='dw-icon dw-error-icon'
              />
              <p>{status.message}</p>
            </section>
            <footer>
              <Button
                kind='filled'
                size='small'
                onClick={handleCancel}
                className='dw-full-width-button'
              >
                Close
              </Button>
            </footer>
          </div>
        )}
        {/* Success */}
        {status.type === 'success' && (
          <div className='dw-generic-section'>
            <section>
              <Icon
                title='Success'
                name='check-circle-filled'
                className='dw-icon dw-success-icon'
              />
              <p>Wallpaper applied successfully</p>
            </section>
            <footer>
              <Button
                kind='filled'
                size='small'
                onClick={handleCancel}
                className='dw-full-width-button'
              >
                Done!
              </Button>
            </footer>
          </div>
        )}
        {/* Idle */}
        {status.type === 'idle' && (
          <div className='dw-form-section'>
            <div className='dw-text-section'>
              <h1 className='dw-modal-title'>Use as desktop wallpaper</h1>
            </div>
            <div className='preview-container'>
              <div
                className={`preview-box fit-${fit}`}
                style={{
                  backgroundImage: `url('${imageSource}')`,
                  aspectRatio: `${monitorWidth} / ${monitorHeight}`,
                  backgroundSize: getBackgroundSize(),
                  backgroundRepeat: 'no-repeat',
                  backgroundPosition: 'center',
                }}
                title={fit}
                ref={containerRef}
              />
            </div>
            {displays.length > 1 && (
              <>
                {!useAllMonitors && (
                  <DropDown
                    positionStrategy='fixed'
                    className='dw-dropdown'
                    placeholder='Pick one...'
                    onChange={handleMonitorOnChange}
                    value={monitor}
                  >
                    <div
                      slot='label'
                      className='dw-input-label'
                    >
                      Screen
                    </div>
                    {displays.map((display) => (
                      <leo-option
                        key={display.id}
                        value={display.id}
                      >
                        {display.label}
                      </leo-option>
                    ))}
                  </DropDown>
                )}
                <Checkbox
                  checked={useAllMonitors}
                  onChange={handleUseBackgroundOnChange}
                >
                  <span className='dw-input-label'>Apply to all screens</span>
                </Checkbox>
              </>
            )}
            <DropDown
              positionStrategy='fixed'
              className='dw-dropdown'
              placeholder='Choose fit'
              onChange={handleFitOnChange}
              value={FIT_OPTIONS.get(fit)}
            >
              <div
                slot='label'
                className='dw-input-label'
              >
                Wallpaper fit
              </div>
              {[...FIT_OPTIONS.keys()].map((key) => (
                <leo-option
                  key={key}
                  value={key}
                >
                  {FIT_OPTIONS.get(key)}
                </leo-option>
              ))}
            </DropDown>

            <div className='dw-side-by-side-buttons'>
              <Button
                kind='plain'
                size='small'
                onClick={handleCancel}
                className='dw-padded-button'
              >
                Cancel
              </Button>
              <Button
                kind='filled'
                size='small'
                onClick={handleApply}
                className='dw-padded-button'
              >
                Apply
              </Button>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}
