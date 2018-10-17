declare namespace Sync {
  export interface ApplicationState {
    syncData: State | undefined
  }

  export interface SettingsFromBackEnd {
    sync_bookmarks: boolean
    sync_configured: boolean
    sync_history: boolean
    sync_settings: boolean
    sync_this_device: boolean
  }

  export interface DevicesFromBackEnd {
    name: string
    device_id: number
    last_active: number
  }
  export interface Devices {
    name: string
    id: number
    lastActive: number
  }
  export interface State {
    thisDeviceName: string
    devices: Devices[]
    isSyncConfigured: boolean
    shouldSyncThisDevice: boolean
    seedQRImageSource: string
    syncWords: string
    syncBookmarks: boolean
    syncSavedSiteSettings: boolean
    syncBrowsingHistory: boolean
  }
}
