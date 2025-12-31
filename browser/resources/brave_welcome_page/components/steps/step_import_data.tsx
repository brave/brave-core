/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import Checkbox from '@brave/leo/react/checkbox'
import ProgressRing from '@brave/leo/react/progressRing'

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
  { id: 'bookmarks', label: 'Bookmarks', completedText: '68 bookmarks' },
  { id: 'history', label: 'History', completedText: 'Last 180 days' },
  { id: 'passwords', label: 'Saved passwords', completedText: '23 passwords' },
  { id: 'extensions', label: 'Extensions', completedText: '12 extensions' },
  { id: 'tabs', label: 'Open tabs and groups', completedText: '8 tabs' }
]

type ImportStatus = 'pending' | 'importing' | 'completed'

// Context to share state between Content and Footer
interface ImportDataContextType {
  isBrowserSelected: boolean
  setIsBrowserSelected: (value: boolean) => void
  hasSelectedImports: boolean
  setHasSelectedImports: (value: boolean) => void
  isImporting: boolean
  setIsImporting: (value: boolean) => void
  importComplete: boolean
  setImportComplete: (value: boolean) => void
  selectedBrowser: BrowserProfile | null
  setSelectedBrowser: (value: BrowserProfile | null) => void
  selectedImports: Record<string, boolean>
  setSelectedImports: (value: Record<string, boolean>) => void
  importStatuses: Record<string, ImportStatus>
  setImportStatuses: (value: Record<string, ImportStatus>) => void
  startImport: () => void
  resetImport: () => void
}

const ImportDataContext = React.createContext<ImportDataContextType>({
  isBrowserSelected: false,
  setIsBrowserSelected: () => {},
  hasSelectedImports: true,
  setHasSelectedImports: () => {},
  isImporting: false,
  setIsImporting: () => {},
  importComplete: false,
  setImportComplete: () => {},
  selectedBrowser: null,
  setSelectedBrowser: () => {},
  selectedImports: {},
  setSelectedImports: () => {},
  importStatuses: {},
  setImportStatuses: () => {},
  startImport: () => {},
  resetImport: () => {}
})

// Provider component to wrap both Content and Footer
export function ImportDataProvider({ children }: { children: React.ReactNode }) {
  const [isBrowserSelected, setIsBrowserSelected] = React.useState(false)
  const [hasSelectedImports, setHasSelectedImports] = React.useState(true)
  const [isImporting, setIsImporting] = React.useState(false)
  const [importComplete, setImportComplete] = React.useState(false)
  const [selectedBrowser, setSelectedBrowser] = React.useState<BrowserProfile | null>(null)
  const [selectedImports, setSelectedImports] = React.useState<Record<string, boolean>>(
    Object.fromEntries(importOptions.map(opt => [opt.id, true]))
  )
  const [importStatuses, setImportStatuses] = React.useState<Record<string, ImportStatus>>(
    Object.fromEntries(importOptions.map(opt => [opt.id, 'pending']))
  )

  const startImport = React.useCallback(() => {
    setIsImporting(true)
    
    // Get list of selected imports to process
    const selectedIds = importOptions
      .filter(opt => selectedImports[opt.id])
      .map(opt => opt.id)
    
    // Set all selected items to importing initially
    setImportStatuses(prev => {
      const newStatuses = { ...prev }
      selectedIds.forEach(id => {
        newStatuses[id] = 'importing'
      })
      return newStatuses
    })

    // Simulate completing each item one by one
    selectedIds.forEach((id, index) => {
      setTimeout(() => {
        setImportStatuses(prev => ({
          ...prev,
          [id]: 'completed'
        }))
        
        // Check if this is the last item
        if (index === selectedIds.length - 1) {
          setTimeout(() => {
            setIsImporting(false)
            setImportComplete(true)
          }, 300)
        }
      }, (index + 1) * 800) // Stagger completions by 800ms each
    })
  }, [selectedImports])

  const resetImport = React.useCallback(() => {
    setIsImporting(false)
    setImportComplete(false)
    setIsBrowserSelected(false)
    setSelectedBrowser(null)
    setSelectedImports(Object.fromEntries(importOptions.map(opt => [opt.id, true])))
    setImportStatuses(Object.fromEntries(importOptions.map(opt => [opt.id, 'pending'])))
  }, [])

  return (
    <ImportDataContext.Provider value={{
      isBrowserSelected,
      setIsBrowserSelected,
      hasSelectedImports,
      setHasSelectedImports,
      isImporting,
      setIsImporting,
      importComplete,
      setImportComplete,
      selectedBrowser,
      setSelectedBrowser,
      selectedImports,
      setSelectedImports,
      importStatuses,
      setImportStatuses,
      startImport,
      resetImport
    }}>
      {children}
    </ImportDataContext.Provider>
  )
}

type TransitionState = 'idle' | 'dropdown-exiting' | 'dropdown-entering' | 'selected-exiting' | 'selected-entering' | 'morphing-to-transfer'

export function StepImportDataContent({}: StepContentProps) {
  const {
    setIsBrowserSelected,
    setHasSelectedImports,
    isImporting,
    importComplete,
    selectedBrowser,
    setSelectedBrowser,
    selectedImports,
    setSelectedImports,
    importStatuses
  } = React.useContext(ImportDataContext)

  const [transitionState, setTransitionState] = React.useState<TransitionState>('idle')
  const [displayedBrowser, setDisplayedBrowser] = React.useState<BrowserProfile | null>(selectedBrowser)
  const [isAtScrollBottom, setIsAtScrollBottom] = React.useState(false)
  const prevIsImporting = React.useRef(isImporting)
  const dropdownListRef = React.useRef<HTMLDivElement>(null)

  // Handle scroll detection for hiding gradient at bottom
  const handleDropdownScroll = React.useCallback((e: React.UIEvent<HTMLDivElement>) => {
    const target = e.target as HTMLDivElement
    const isAtBottom = target.scrollHeight - target.scrollTop - target.clientHeight < 1
    setIsAtScrollBottom(isAtBottom)
  }, [])

  // Handle transition when import starts - trigger morph animation
  React.useEffect(() => {
    if (isImporting && !prevIsImporting.current && selectedBrowser) {
      // Trigger the morphing animation class briefly
      setTransitionState('morphing-to-transfer')
      setTimeout(() => setTransitionState('idle'), 50)
    }
    prevIsImporting.current = isImporting
  }, [isImporting, selectedBrowser])

  const handleBrowserSelect = (browser: BrowserProfile) => {
    // Start exit animation for dropdown
    setTransitionState('dropdown-exiting')
    setDisplayedBrowser(browser)
    
    // After exit animation, switch to selected state with enter animation
    setTimeout(() => {
      setSelectedBrowser(browser)
      setIsBrowserSelected(true)
      setTransitionState('selected-entering')
      
      // Reset to idle after enter animation completes
      setTimeout(() => setTransitionState('idle'), 350)
    }, 200)
  }

  const handleClearSelection = () => {
    if (isImporting || importComplete) return // Don't allow changing during/after import
    
    // Start exit animation for selected header
    setTransitionState('selected-exiting')
    
    // After exit animation, switch to dropdown state with enter animation
    setTimeout(() => {
      setSelectedBrowser(null)
      setIsBrowserSelected(false)
      setDisplayedBrowser(null)
      setTransitionState('dropdown-entering')
      
      // Reset to idle after enter animation completes
      setTimeout(() => setTransitionState('idle'), 300)
    }, 200)
  }

  const handleImportToggle = (id: string, checked: boolean) => {
    if (isImporting || importComplete) return // Don't allow changing during/after import
    const newImports = { ...selectedImports, [id]: checked }
    setSelectedImports(newImports)
    setHasSelectedImports(Object.values(newImports).some(v => v))
  }

  // Show importing state
  const showImportingState = isImporting || importComplete
  
  // Determine which view to show based on transition state and selected browser
  const isExitingDropdown = transitionState === 'dropdown-exiting'
  const isEnteringDropdown = transitionState === 'dropdown-entering'
  const isExitingSelected = transitionState === 'selected-exiting'
  const isEnteringSelected = transitionState === 'selected-entering'
  const isMorphingToTransfer = transitionState === 'morphing-to-transfer'
  
  // Show dropdown when no browser selected OR during dropdown transitions
  const showDropdown = (!selectedBrowser && transitionState !== 'selected-entering') || isExitingDropdown || isEnteringDropdown
  // Show selected/morphing header when browser is selected
  const showSelected = selectedBrowser && transitionState !== 'dropdown-entering'

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
          {/* Morphing header - transitions between selected and transfer states */}
          {showSelected && (selectedBrowser || displayedBrowser) ? (
            <div 
              className={`browser-morphing-header ${showImportingState ? 'transfer-mode' : 'selected-mode'} ${isExitingSelected ? 'exiting' : ''} ${isEnteringSelected ? 'entering' : ''} ${isMorphingToTransfer ? 'morphing-to-transfer' : ''} ${importComplete ? 'import-complete' : ''}`}
              onClick={!showImportingState ? handleClearSelection : undefined}
            >
              {/* Browser icon - shared element that morphs position */}
              <div className="browser-item-icon">
                <Icon name={(selectedBrowser || displayedBrowser)!.icon} />
              </div>
              
              {/* Selected state elements - fade out during morph */}
              <div className="selected-elements">
                <h3 className="browser-selected-name">{(selectedBrowser || displayedBrowser)!.name}</h3>
                {(selectedBrowser || displayedBrowser)!.profile && (
                  <Label mode="loud" color="neutral">{(selectedBrowser || displayedBrowser)!.profile}</Label>
                )}
                <Icon name="carat-down" />
              </div>
              
              {/* Transfer state elements - fade in during morph */}
              <div className="transfer-elements">
                <div className="transfer-arrows">
                  <Icon name="carat-right" className="arrow-1" />
                  <Icon name="carat-right" className="arrow-2" />
                  <Icon name="carat-right" className="arrow-3" />
                </div>
                <div className="brave-icon-with-spinner">
                  <Icon name="social-brave-release-favicon-fullheight-color" />
                  {isImporting && (
                    <div className="transfer-spinner">
                      <ProgressRing />
                    </div>
                  )}
                  {importComplete && (
                    <div className="transfer-complete">
                      <Icon name="check-circle-filled" />
                    </div>
                  )}
                </div>
              </div>
            </div>
          ) : (
            <div className="browser-selector-wrapper">
              {/* Browser dropdown list */}
              {showDropdown && (
                <div className={`browser-dropdown ${isExitingDropdown ? 'exiting' : ''} ${isEnteringDropdown ? 'entering' : ''} ${isAtScrollBottom ? 'scrolled-to-bottom' : ''}`}>
                  <div className="browser-dropdown-header">
                    <div className="browser-icons-grid">
                      <Icon name="chromerelease-color" />
                      <Icon name="safari-color" />
                      <Icon name="firefox-color" />
                      <Icon name="edge-color" />
                    </div>
                    <h3>Select your previous browser</h3>
                  </div>
                  <div 
                    className="browser-dropdown-list"
                    ref={dropdownListRef}
                    onScroll={handleDropdownScroll}
                  >
                    {browserProfiles.map((browser, index) => (
                      <div
                        key={index}
                        className="browser-item"
                        onClick={() => handleBrowserSelect(browser)}
                        style={{ animationDelay: `${index * 30}ms` }}
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
            </div>
          )}

          {/* Import options / progress */}
          {selectedBrowser && (
            <div className="import-options">
              <h4>{showImportingState ? 'Importing your content' : 'What do you want to import?'}</h4>
              <div className="import-options-list">
                {importOptions
                  .filter(option => showImportingState ? selectedImports[option.id] : true)
                  .map((option) => (
                  <div
                    key={option.id}
                    className={`import-option-item ${showImportingState ? 'importing-state' : ''}`}
                    onClick={() => !showImportingState && handleImportToggle(option.id, !selectedImports[option.id])}
                  >
                    {showImportingState ? (
                      <>
                        <div className="import-status-icon">
                          {importStatuses[option.id] === 'completed' ? (
                            <Icon name="check-circle-filled" className="status-complete" />
                          ) : (
                            <ProgressRing />
                          )}
                        </div>
                        <span className="import-item-label">{option.label}</span>
                        <span className="import-item-status">
                          {importStatuses[option.id] === 'completed' ? option.completedText : 'Importing'}
                        </span>
                      </>
                    ) : (
                      <div onClick={(e) => e.stopPropagation()}>
                        <Checkbox
                          checked={selectedImports[option.id]}
                          onChange={(detail) => handleImportToggle(option.id, detail.checked)}
                        >
                          {option.label}
                        </Checkbox>
                      </div>
                    )}
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
  const { isBrowserSelected, hasSelectedImports, isImporting, importComplete, startImport, resetImport } = React.useContext(ImportDataContext)

  const canImport = isBrowserSelected && hasSelectedImports
  const showImportingState = isImporting || importComplete

  const handleButtonClick = () => {
    if (importComplete) {
      onNext()
    } else if (!isImporting) {
      startImport()
    }
  }

  const getButtonText = () => {
    if (importComplete) return 'Continue'
    if (isImporting) return 'Importing...'
    return 'Import'
  }

  return (
    <div className="footer">
      <div className="footer-left">
        <Button 
          kind="plain-faint" 
          size="large" 
          onClick={onBack}
          isDisabled={isImporting}
        >
          Back
        </Button>
      </div>
      <div className="footer-right">
        {showImportingState ? (
          <Button kind="plain-faint" size="large" onClick={resetImport} isDisabled={isImporting}>Reset</Button>
        ) : (
          <Button kind="plain-faint" size="large" onClick={onSkip}>Skip</Button>
        )}
        <Button
          kind="filled"
          size="large"
          className='main-button'
          onClick={handleButtonClick}
          isDisabled={!canImport && !importComplete}
          isLoading={isImporting}
        >
          {getButtonText()}
        </Button>
      </div>
    </div>
  )
}

