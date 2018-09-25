declare namespace Sync {
  export interface ApplicationState {
    syncData: State | undefined
  }

  export interface State {
    isSyncEnabled: boolean
    mainDeviceName: string
    setImmediateSyncDevice: boolean
    devices: any[]
    syncBookmarks: boolean
    syncSavedSiteSettings: boolean
    syncBrowsingHistory: boolean
    // TBD
    TBDsettings: any
    TBDdevices: any
    TBDsyncWords: any
    TBDseed: any
    TBDmessage: any
  }
}
