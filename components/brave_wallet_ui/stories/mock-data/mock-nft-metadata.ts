// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { NFTMetadataReturnType } from '../../constants/types'
import MoonCatIcon from '../../assets/png-icons/mooncat.png'
import MooncatProjectIcon from '../../assets/png-icons/mooncat-project-icon.png'

export const mockNFTMetadata: NFTMetadataReturnType[] = [
  {
    chainName: 'Ethereum',
    tokenType: 'ERC721',
    tokenID: '0x42a5',
    metadataUrl: 'ipfs://bafy1',
    imageURL: MoonCatIcon,
    imageMimeType: 'image/png',
    animationURL: undefined,
    animationMimeType: undefined,
    floorFiatPrice: '2123.42',
    floorCryptoPrice: '0.64922',
    contractInformation: {
      address: '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69',
      name: 'MoonCatsRescue',
      description: 'After a whirlwind adventure four years in the making, 25440 MoonCats have been rescued and are acclimating to their life on the blockchain.',
      website: 'https://mooncat.community',
      twitter: 'https://twitter.com/ponderware',
      facebook: '',
      logo: MooncatProjectIcon
    }
  },
  // AUDIO
  {
    chainName: 'Ethereum',
    tokenType: 'ERC1155',
    tokenID: '0x17',
    metadataUrl: 'ipfs://bafy1',
    imageURL: 'https://arweave.net/uLD7L4oAnKY8zoeE-zrB6gs-rKBkt4wse_Jt_NFQ4lM',
    imageMimeType: 'image/png',
    animationURL: 'https://arweave.net/g1MGUZO3blCHhYIjqFbswqmp6cmWgAbueVUcnm8FvfQ',
    animationMimeType: 'audio/wav',
    floorFiatPrice: '2123.42',
    floorCryptoPrice: '0.64922',
    contractInformation: {
      address: '0xd766f78936a619d8e412362bedf8ff1a49ce62ea',
      name: 'CHICKENHEAD [7/7]',
      description: 'REVOLUCIÓN\\n\\n#0111 // CHICANO FOREVER  \\n#0211 // RIDE WITH ME  \\n#0311 // FLY AWAY BIRDIE  \\n#0411 // THE LONELY BIRD  \\n#0511 // SEARCHING FOR SHELTER  \\n#0611 // TRIBE OF THE PEOPLE  \\n#0711 // EL REY  \\n#0811 // MAMASITA  \\n#0911 // CHICKENHEAD  \\n#1011 // GOBIERNO  \\n#1111 // KIDS IN CAGES',
      website: 'https://mooncat.community',
      twitter: 'https://twitter.com/ponderware',
      facebook: '',
      logo: ''
    }
  },
  // VIDEO
  {
    chainName: 'Ethereum',
    tokenType: 'ERC1155',
    tokenID: '0xBA7',
    metadataUrl: 'ipfs://bafy1',
    imageURL: 'https://ipfs.pixura.io/ipfs/QmdzCeMJ9Er4tWFJqUxYnMk7p6ob1DKs9zRpmw4E3KeUpG/GIF.gif',
    imageMimeType: 'image/gif',
    animationURL: 'https://ipfs.pixura.io/ipfs/QmSWsRwmisz9QwmxNn9d1jh5CB1u9tzGw235jEuCpw3d6L/_AnimatormakingAnimationAll3.mp4',
    animationMimeType: 'video/mp4',
    floorFiatPrice: '2123.42',
    floorCryptoPrice: '0.64922',
    contractInformation: {
      address: '0x2a459947f0ac25ec28c197f09c2d88058a83f3bb',
      name: 'Stickmen Toy',
      description: 'I am sure we all go through ups and downs when creating art.\nMaybe caring a little too much about what others think' +
        ' and not enough appreciation towards your own art. \nWhatever you make, it is still your creation. \nAnd it will always give you a like. :)\n\n*This is a full-length (2:37) short film that achieved a Staff Pick from Vimeo.',
      website: '',
      twitter: '',
      facebook: '',
      logo: ''
    }
  },
  // HTML
  {
    chainName: 'Ethereum',
    tokenType: 'ERC1155',
    tokenID: '0xBA7',
    metadataUrl: 'ipfs://bafy1',
    imageURL: 'https://www.arpeggi.io/image/song/476',
    imageMimeType: 'text/html',
    animationURL: 'https://www.arpeggi.io/player/476',
    animationMimeType: 'text/html',
    floorFiatPrice: '2123.42',
    floorCryptoPrice: '0.64922',
    contractInformation: {
      address: '0x476FA1B9Cb714f7A5c7723ac48596c422B2dabAE',
      name: 'Virtual Baroque',
      description: 'Virtual Baroque by Pedro Gomes. Minted on December 11, 2021. Fully on-chain. Composed with Arpeggi Studio (arpeggi.io/genesis-studio).',
      website: '',
      twitter: '',
      facebook: '',
      logo: ''
    }
  },
  // 3D Model
  {
    chainName: 'Ethereum',
    tokenType: 'ERC1155',
    tokenID: '0xBA7',
    metadataUrl: undefined,
    imageURL: undefined,
    imageMimeType: undefined,
    animationURL: 'https://openseauserdata.com/files/c3ba03ba3038e8c52ee9b2c58aece252.glb',
    animationMimeType: 'model/glb',
    floorFiatPrice: '2123.42',
    floorCryptoPrice: '0.64922',
    contractInformation: {
      address: '0x495f947276749ce646f68ac8c248420045cb7b5e',
      name: 'Japanese Cherry Blossom Tree',
      description: 'A Japanese Cherry Blossom Tree made in Blender. This 3D model is custom made with detailed sculpting and hand painted textures. It can be used as a game object or to add to your 3D NFT collection!',
      website: '',
      twitter: '',
      facebook: '',
      logo: ''
    }
  }
]
