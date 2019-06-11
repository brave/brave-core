declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface State {
    // TODO explicitly type data when API is known
    defaultSearchProviders: Array<any>
  }

}
