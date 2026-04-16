import * as React from 'react'
import Button from '@brave/leo/react/button'
import DropDown from '@brave/leo/react/dropdown'
import ProgressRing from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import Checkbox from '@brave/leo/react/checkbox'

import '@brave/leo/tokens/css/variables.css'
import './app.css'
import { useCallback } from 'react'

const FIT_OPTIONS = new Map([
  ['fill', 'Fill'],
  ['fit', 'Fit'],
  ['stretch', 'Stretch'],
  ['center', 'Center'],
  ['tile', 'Tile'],
])

const MONITOR_OPTIONS = new Map([
  ["ID_LG_UltraWide_34WK95U", { label: "LG UltraWide 34WK95U", width: 3440, height: 1440 }],
  ["ID_Samsung_Odyssey_G9", { label: "Samsung Odyssey G9", width: 3840, height: 1080 }],
  ["ID_Lenovo_ThinkVision_P27q-20", { label: "Lenovo ThinkVision P27q-20", width: 1440, height: 2560 }],
])

const DEFAULT_MONITOR_KEY = MONITOR_OPTIONS.keys().next().value!

export function App() {
  const [fit, setFit] = React.useState('fill')
  const [monitor, setMonitor] = React.useState(DEFAULT_MONITOR_KEY)

  const [status, _] = React.useState<{ type: 'loading' | 'error' | 'success' | 'idle'; message?: string }>({ type: 'idle' })
  const [imgSize, setImgSize] = React.useState({ width: 0, height: 0 })
  const [scale, setScale] = React.useState(1)

  const imageUrl = 'https://placehold.co/600x400'
  const containerRef = React.useRef<HTMLDivElement>(null)
  const [useBackground, setUseBackground] = React.useState(true)

  const selectedMonitor = MONITOR_OPTIONS.get(monitor)
  const monitorWidth = selectedMonitor?.width ?? 1920
  const monitorHeight = selectedMonitor?.height ?? 1080

  const updateScale = useCallback(() => {
    if (containerRef.current) {
      const rect = containerRef.current.getBoundingClientRect()
      setScale(rect.width / monitorWidth)
    }
  }, [monitorWidth])

  React.useEffect(() => {
    const img = new Image()
    img.onload = () => setImgSize({ width: img.naturalWidth, height: img.naturalHeight })
    img.src = imageUrl
  }, [imageUrl])

  React.useEffect(() => {
    updateScale()
    window.addEventListener('resize', updateScale)
    return () => window.removeEventListener('resize', updateScale)
  }, [updateScale])

  const handleApply = () => {
    // Call C++ handler to set wallpaper
    //;(window as any).chrome?.send?.('set-desktop-wallpaper', [imageUrl, fit])
  }

  const handleCancel = () => {
    // Close the constrained dialog from JS
    //;(window as any).chrome?.send?.('dialogClose')
  }

  const handleFitOnChange = ({ value }: { value: string }) => {
    setFit(value)
  }

  const handleUseBackgroundOnChange = ({ checked }: { checked: boolean }) => {
    setUseBackground(checked)
  }

  const getBackgroundSize = () => {
    switch (fit) {
      case 'fill': return 'cover'
      case 'fit': return 'contain'
      case 'stretch': return '100% 100%'
      case 'center':
        return `${imgSize.width * scale}px ${imgSize.height * scale}px`
      case 'tile':
        return `${imgSize.width * scale}px ${imgSize.height * scale}px`
      default: return 'cover'
    }
  }

  return (
    <div className='dw-card'>
      <div className='dw-modal-layout'>
        {status.type === 'loading' && (
          <div className='dw-generic-section'>
            <ProgressRing mode='indeterminate' />
            <p>Applying wallpaper...</p>
          </div>
        )}
        {status.type === 'error' && (
          <div className='dw-generic-section'>
            <section>
              <Icon title={status.message} name='dangerous-filled' className='dw-icon dw-error-icon' />
              <p>{status.message}</p>
            </section>
            <footer>
              <Button kind='filled' size='small' className='dw-full-width-button'>
                Close
              </Button>
            </footer>
          </div>
        )}
        {status.type === 'success' && (
          <div className='dw-generic-section'>
            <section>
              <Icon title='Success' name='check-circle-filled' className='dw-icon dw-success-icon' />
              <p>Wallpaper applied successfully</p>
            </section>
            <footer>
              <Button kind='filled' size='small' className='dw-full-width-button'>
                Done!
              </Button>
            </footer>
          </div>
        )}
        {status.type === 'idle' && (
          <div className='dw-form-section'>
            <div className='dw-text-section'>
              <h1 className='dw-modal-title'>Use as desktop wallpaper</h1>
            </div>
            <div className='preview-container'>
              <div
                className={`preview-box fit-${fit}`}
                style={{
                  backgroundImage: `url('${imageUrl}')`,
                  aspectRatio: `${monitorWidth} / ${monitorHeight}`,
                  backgroundSize: getBackgroundSize()
                }}
                title={fit}
                ref={containerRef}
              />
            </div>
            {MONITOR_OPTIONS.size > 1 && (
              <>
                {!useBackground && (
                  <DropDown
                    positionStrategy='fixed'
                    className='dw-dropdown'
                    placeholder='Pick one...'
                    onChange={({ value }: { value: string }) => setMonitor(value)}
                    value={MONITOR_OPTIONS.get(monitor)?.label}
                  >
                    <div slot='label' className='dw-input-label'>Screen</div>
                    {[...MONITOR_OPTIONS.entries()].map(([key, value]) => (
                      <leo-option key={key} value={key}>
                        {value.label}
                      </leo-option>
                    ))}
                  </DropDown>
                )}
                <Checkbox checked={useBackground} onChange={handleUseBackgroundOnChange}>
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
              <div slot='label' className='dw-input-label'>Wallpaper fit</div>
              {[...FIT_OPTIONS.keys()].map((key) => (
                <leo-option key={key} value={key}>
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
