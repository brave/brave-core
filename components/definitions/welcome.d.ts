declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface State {
    pageIndex: number
  }
}
