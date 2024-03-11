# Gadgets

The Orchard circuit makes use of the following gadgets from the `halo2_gadgets` crate:

- [Elliptic curve](https://zcash.github.io/halo2/design/gadgets/ecc.html):
  - `FixedPoint`
  - `FixedPointBaseField`
  - `FixedPointShort`
  - `NonIdentityPoint`
  - `Point`
- Poseidon:
  - `Hash<ConstantLength>`
- [Sinsemilla](https://zcash.github.io/halo2/design/gadgets/sinsemilla.html):
  - `CommitDomain`
  - `Message`
  - `MessagePiece`
  - [`MerklePath`](https://zcash.github.io/halo2/design/gadgets/sinsemilla/merkle-crh.html)

It instantiates the instruction sets required for these gadgets with the following chips:

- `halo2_gadgets::ecc::chip::EccChip`
- `halo2_gadgets::poseidon::Pow5Chip`
- [`halo2_gadgets::sinsemilla::chip::SinsemillaChip`](https://zcash.github.io/halo2/design/gadgets/sinsemilla.html#plonk--halo-2-constraints)
- [`halo2_gadgets::sinsemilla::merkle::chip::MerkleChip`](https://zcash.github.io/halo2/design/gadgets/sinsemilla/merkle-crh.html#circuit-components)
- `halo2_gadgets::utilities::UtilitiesInstructions`
- [`halo2_gadgets::utilities::lookup_range_check::LookupRangeCheckConfig`](https://zcash.github.io/halo2/design/gadgets/decomposition.html#lookup-decomposition)

It also makes use of the following utility functions for standardising constraints:
- `halo2_gadgets::utilities::{bitrange_subset, bool_check}`
