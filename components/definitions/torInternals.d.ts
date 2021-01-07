declare namespace TorInternals {
  export interface ApplicationState {
    torInternalsData: State | undefined
  }

  export interface State {
    generalInfo: GeneralInfo
  }

  export interface GeneralInfo {
    torVersion: string,
    torPid: number,
    torProxyURI: string,
    isTorConnected: boolean
  }
}

