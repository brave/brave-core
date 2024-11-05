declare namespace WebcompatReporter {
  export interface ApplicationState {
    reporterState: State | undefined
  }

  export interface DialogArgs {
    url: string
    isErrorPage: boolean
    adBlockSetting: string
    fpBlockSetting: string
    shieldsEnabled: string
    contactInfo: string
  }

  export interface State {
    dialogArgs: DialogArgs
    submitted: boolean
  }
}
