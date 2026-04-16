import * as React from 'react'
import '@brave/leo/tokens/css/variables.css'
import { App } from '../components/app'

const DarkThemeWrapper: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  React.useEffect(() => {
    document.documentElement.setAttribute('data-theme', 'dark')
    return () => {
      document.documentElement.removeAttribute('data-theme')
    }
  }, [])
  return <>{children}</>
}

export const Default = {
  render: () => (
      <div style={{ width: '400px', margin: '0 auto' }}>
        <App />
      </div>
  ),
}

export default {
  title: 'Desktop Wallpaper',
  component: App,
}
