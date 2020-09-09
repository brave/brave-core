declare namespace IPFS {
  export interface ApplicationState {
    ipfsData: State | undefined
  }

  export interface State {
    connectedPeers: {
      peerCount: number
    }
    addressesConfig: AddressesConfig
    daemonStatus: DaemonStatus
  }

  export interface AddressesConfig {
    api: string
    gateway: string
    swarm: string[]
  }

  export interface DaemonStatus {
    installed: bool
    launched: bool
  }
}
