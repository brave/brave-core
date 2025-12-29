/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import Checkbox from '@brave/leo/react/checkbox'

import { StepContentProps, StepFooterProps } from '../types'

interface BrowserProfile {
  name: string
  icon: string
  profile?: string
}

const browserProfiles: BrowserProfile[] = [
  { name: 'Google Chrome', icon: 'chromerelease-color', profile: 'Agustín Ruiz' },
  { name: 'Google Chrome', icon: 'chromerelease-color', profile: 'Work' },
  { name: 'Microsoft Edge', icon: 'edge-color' },
  { name: 'Vivaldi', icon: 'vivaldi-color' },
  { name: 'DuckDuckGo', icon: 'duckduckgo-color' },
  { name: 'Firefox', icon: 'firefox-color' },
  { name: 'Edge', icon: 'edge-color' },
  { name: 'Ecosia', icon: 'ecosia-color' },
  { name: 'Safari', icon: 'safari-color' }
]

const importOptions = [
  { id: 'bookmarks', label: 'Bookmarks' },
  { id: 'history', label: 'History' },
  { id: 'passwords', label: 'Saved passwords' },
  { id: 'extensions', label: 'Extensions' },
  { id: 'tabs', label: 'Open tabs and groups' }
]

// Context to share state between Content and Footer
interface ImportDataContextType {
  isBrowserSelected: boolean
  setIsBrowserSelected: (value: boolean) => void
  hasSelectedImports: boolean
  setHasSelectedImports: (value: boolean) => void
}

const ImportDataContext = React.createContext<ImportDataContextType>({
  isBrowserSelected: false,
  setIsBrowserSelected: () => {},
  hasSelectedImports: true,
  setHasSelectedImports: () => {}
})

// Provider component to wrap both Content and Footer
export function ImportDataProvider({ children }: { children: React.ReactNode }) {
  const [isBrowserSelected, setIsBrowserSelected] = React.useState(false)
  const [hasSelectedImports, setHasSelectedImports] = React.useState(true)
  return (
    <ImportDataContext.Provider value={{
      isBrowserSelected,
      setIsBrowserSelected,
      hasSelectedImports,
      setHasSelectedImports
    }}>
      {children}
    </ImportDataContext.Provider>
  )
}

export function StepImportDataContent({}: StepContentProps) {
  const { setIsBrowserSelected, setHasSelectedImports } = React.useContext(ImportDataContext)
  const [selectedBrowser, setSelectedBrowser] = React.useState<BrowserProfile | null>(null)
  const [selectedImports, setSelectedImports] = React.useState<Record<string, boolean>>(
    Object.fromEntries(importOptions.map(opt => [opt.id, true]))
  )

  const handleBrowserSelect = (browser: BrowserProfile) => {
    setSelectedBrowser(browser)
    setIsBrowserSelected(true)
  }

  const handleClearSelection = () => {
    setSelectedBrowser(null)
    setIsBrowserSelected(false)
  }

  const handleImportToggle = (id: string, checked: boolean) => {
    const newImports = { ...selectedImports, [id]: checked }
    setSelectedImports(newImports)
    setHasSelectedImports(Object.values(newImports).some(v => v))
  }

  return (
    <div className="content">
      <div className="left-content">
        <div className="left-text-content">
          <h1>Bring along your old browser content</h1>
          <p>Import everything from your old browser and make the transition seamless.</p>
          <p>You can also do this later in Brave's settings.</p>
        </div>
      </div>
      <div className="right-content">
        <div className="import-container">
          {/* Browser selector header */}
          {selectedBrowser ? (
            <div className="browser-selected-header" onClick={handleClearSelection}>
              <div className="browser-item-icon">
                <Icon name={selectedBrowser.icon} />
              </div>
              <h3 className="browser-selected-name">{selectedBrowser.name}</h3>
              {selectedBrowser.profile && (
                <Label mode="loud" color="neutral">{selectedBrowser.profile}</Label>
              )}
              <Icon name="carat-down" />
            </div>
          ) : (
            <div className="browser-dropdown">
              <div className="browser-dropdown-header">
                <div className="browser-icons-grid">
                  <Icon name="chromerelease-color" />
                  <Icon name="safari-color" />
                  <Icon name="firefox-color" />
                  <Icon name="edge-color" />
                </div>
                <h3>Select your previous browser</h3>
              </div>
              <div className="browser-dropdown-list">
                {browserProfiles.map((browser, index) => (
                  <div
                    key={index}
                    className="browser-item"
                    onClick={() => handleBrowserSelect(browser)}
                  >
                    <div className="browser-item-icon">
                      <Icon name={browser.icon} />
                    </div>
                    <span className="browser-item-name">{browser.name}</span>
                    {browser.profile && (
                      <Label mode="loud" color="neutral">{browser.profile}</Label>
                    )}
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Import options */}
          {selectedBrowser && (
            <div className="import-options">
              <h4>What do you want to import?</h4>
              <div className="import-options-list">
                {importOptions.map((option) => (
                  <div
                    key={option.id}
                    className="import-option-item"
                    onClick={() => handleImportToggle(option.id, !selectedImports[option.id])}
                  >
                    <Checkbox
                      checked={selectedImports[option.id]}
                      onChange={(detail) => handleImportToggle(option.id, detail.checked)}
                    >
                      {option.label}
                    </Checkbox>
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  )
}

export function StepImportDataFooter({ onNext, onBack, onSkip }: StepFooterProps) {
  const { isBrowserSelected, hasSelectedImports } = React.useContext(ImportDataContext)

  const canImport = isBrowserSelected && hasSelectedImports

  return (
    <div className="footer">
      <div className="footer-left">
        <Button kind="plain-faint" size="large" onClick={onBack}>Back</Button>
      </div>
      <div className="footer-right">
        <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        <Button
          kind="filled"
          size="large"
          className='main-button'
          onClick={onNext}
          isDisabled={!canImport}
        >
          Import
        </Button>
      </div>
    </div>
  )
}

