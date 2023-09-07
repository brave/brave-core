declare namespace WebcompatReporter {
  export interface ApplicationState {
    reporterState: State | undefined
  }

  export interface DialogArgs {
    url: string
    adBlockSetting: string
    fpBlockSetting: string
    shieldsEnabled: string
  }

  export interface State {
    dialogArgs: DialogArgs
    submitted: boolean
  }
}
