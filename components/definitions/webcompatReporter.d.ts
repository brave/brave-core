declare namespace WebcompatReporter {
  export interface ApplicationState {
    reporterState: State | undefined
  }

  export interface State {
    siteUrl: string
    submitted: boolean
  }
}
