// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import ZeroXIcon from '../assets/lp-icons/0x.svg'
import AaveIcon from '../assets/lp-icons/aave.svg'
import AldrinIcon from '../assets/lp-icons/aldrin.svg'
import ApeSwapIcon from '../assets/lp-icons/apeswap.svg'
import BraveIcon from '../../../../assets/svg-icons/brave-icon.svg'
import BalancerIcon from '../assets/lp-icons/balancer.svg'
import BalansolIcon from '../assets/lp-icons/balansol.svg'
import BancorIcon from '../assets/lp-icons/bancor.svg'
import BiSwapIcon from '../assets/lp-icons/biswap.svg'
import ComponentIcon from '../assets/lp-icons/component.jpg'
import CremaIcon from '../assets/lp-icons/crema.svg'
import CropperIcon from '../assets/lp-icons/cropper.svg'
import CryptoComIcon from '../assets/lp-icons/cryptocom.svg'
import CurveIcon from '../assets/lp-icons/curve.svg'
import CykuraIcon from '../assets/lp-icons/cykura.png'
import DFYNIcon from '../assets/lp-icons/dfyn.svg'
import DoDoIcon from '../assets/lp-icons/dodo.svg'
import DradexIcon from '../assets/lp-icons/dradex.svg'
import FireBirdOneSwapIcon from '../assets/lp-icons/firebirdoneswap.png'
import GooseFXIcon from '../assets/lp-icons/goosefx.svg'
import InvariantIcon from '../assets/lp-icons/invariant.svg'
import IronSwapIcon from '../assets/lp-icons/ironswap.svg'
import JupiterIcon from '../assets/lp-icons/jupiter.svg'
import KyberDMMIcon from '../assets/lp-icons/kyberdmm.svg'
import LidoIcon from '../assets/lp-icons/lido.svg'
import LifinityIcon from '../assets/lp-icons/lifinity.jpg'
import LiFiIcon from '../assets/lp-icons/lifi.svg'
import MakerPsmIcon from '../assets/lp-icons/makerpsm.svg'
import MarinadeIcon from '../assets/lp-icons/marinade.svg'
import MDexIcon from '../assets/lp-icons/mdex.svg'
import MercurialIcon from '../assets/lp-icons/mercurial.svg'
import MeshSwapIcon from '../assets/lp-icons/meshswap.svg'
import MStableIcon from '../assets/lp-icons/mstable.png'
import OrcaIcon from '../assets/lp-icons/orca.svg'
import QuickSwapIcon from '../assets/lp-icons/quickswap.svg'
import PancakeSwapIcon from '../assets/lp-icons/pancakeswap.svg'
import PenguinIcon from '../assets/lp-icons/penguin.png'
import SaberIcon from '../assets/lp-icons/saber.svg'
import SaddleIcon from '../assets/lp-icons/saddle.png'
import SarosIcon from '../assets/lp-icons/saros.svg'
import SerumIcon from '../assets/lp-icons/serum.svg'
import ShellIcon from '../assets/lp-icons/shell.jpg'
import ShibaSwapIcon from '../assets/lp-icons/shibaswap.svg'
import StepIcon from '../assets/lp-icons/step.svg'
import StepnIcon from '../assets/lp-icons/stepn.png'
import SushiSwapIcon from '../assets/lp-icons/sushiswap.svg'
import SynapseIcon from '../assets/lp-icons/synapse.png'
import SynthetixIcon from '../assets/lp-icons/synthetix.svg'
import UniswapIcon from '../assets/lp-icons/uniswap.svg'
import WaultSwapIcon from '../assets/lp-icons/waultswap.png'
import WOOFiIcon from '../assets/lp-icons/woofi.svg'

type LPMetadataType = {
  [name: string]: string
}

const LPMetadata: LPMetadataType = {
  '0x': ZeroXIcon,
  'Aave_V2': AaveIcon,
  'Aldrin': AldrinIcon,
  'ApeSwap': ApeSwapIcon,
  'Auto': BraveIcon,
  'Balancer': BalancerIcon,
  'Balancer_V2': BalancerIcon,
  'Balansol': BalansolIcon,
  'Bancor': BancorIcon,
  'BancorV3': BancorIcon,
  'BiSwap': BiSwapIcon,
  'Component': ComponentIcon,
  'Crema': CremaIcon,
  'Cropper': CropperIcon,
  'CryptoCom': CryptoComIcon,
  'Curve': CurveIcon,
  'Curve_V2': CurveIcon,
  'Cykura': CykuraIcon,
  'Dfyn': DFYNIcon,
  'DODO': DoDoIcon,
  'DODO_V2': DoDoIcon,
  'Dradex': DradexIcon,
  'FirebirdOneSwap': FireBirdOneSwapIcon,
  'GooseFX': GooseFXIcon,
  'Invariant': InvariantIcon,
  'IronSwap': IronSwapIcon,
  'Jupiter': JupiterIcon,
  'KyberDMM': KyberDMMIcon,
  'Lido': LidoIcon,
  'Li.Fi': LiFiIcon,
  'LI.FI Dex Aggregator': LiFiIcon,
  'Lifinity': LifinityIcon,
  // LiquidityProvider info unknown
  'LiquidityProvider': '',
  'MakerPsm': MakerPsmIcon,
  'Marinade': MarinadeIcon,
  'MDex': MDexIcon,
  'Mercurial': MercurialIcon,
  'MeshSwap': MeshSwapIcon,
  'mStable': MStableIcon,
  // MultiHop info unknown
  'MultiHop': '',
  'Orca': OrcaIcon,
  'QuickSwap': QuickSwapIcon,
  'PancakeSwap': PancakeSwapIcon,
  'PancakeSwap_V2': PancakeSwapIcon,
  'PancakeSwap_V3': PancakeSwapIcon,
  'Penguin': PenguinIcon,
  'Saber': SaberIcon,
  'Saddle': SaddleIcon,
  'Saros': SarosIcon,
  'Serum': SerumIcon,
  'Shell': ShellIcon,
  'ShibaSwap': ShibaSwapIcon,
  'Step': StepIcon,
  'Stepn': StepnIcon,
  'SushiSwap': SushiSwapIcon,
  'Synapse': SynapseIcon,
  'Synthetix': SynthetixIcon,
  'Uniswap': UniswapIcon,
  'Uniswap_V2': UniswapIcon,
  'Uniswap_V3': UniswapIcon,
  'WaultSwap': WaultSwapIcon,
  'WOOFi': WOOFiIcon
}

export default LPMetadata
