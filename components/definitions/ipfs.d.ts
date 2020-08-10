declare namespace IPFS {
  export interface ApplicationState {
    ipfsData: State | undefined
  }

  export interface State {
    connectedPeers: {
      peerCount: number
    }
  }
}
