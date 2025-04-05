# `shardtree`

This is a Rust crate that provides an implementation of a fixed-depth Merkle
tree structure that is densely filled from the left. It supports:

- *Out-of-order insertion*: leaves and nodes may be inserted into the tree in
  arbitrary order. The structure will keep track of the right-most filled
  position as the frontier of the tree; any unfilled leaves to the left of this
  position are considered "missing", while any unfilled leaves to the right of
  this position are considered "empty".
- *Witnessing*: Individual leaves of the Merkle tree may be marked such that
  witnesses will be maintained for the marked leaves as additional nodes are
  inserted into the tree, but leaf and node data not specifically required to
  maintain these witnesses is not retained, for space efficiency.
- *Checkpointing*: the tree may be reset to a previously checkpointed state, up
  to a fixed number of checkpoints.

The tree is represented as an ordered collection of fixed-depth subtrees, or
"shards". The roots of the shards form the leaves in the "cap".

```
Level
  3           root         \
              / \           |
            /     \         |
  2       /         \        } cap
        / \         / \     |
       /   \       /   \    |
  1   A     B     C     D  /  \
     / \   / \   / \   / \     } shards
  0 /\ /\ /\ /\ /\ /\ /\ /\   /
```

This structure enables witnesses for marked leaves to be advanced up to recent
checkpoints or the latest state of the tree, without having to insert each
intermediate leaf individually. Instead, only the roots of all complete shards
between the one containing the marked leaf and the tree frontier need to be
inserted, along with the necessary nodes to build a path from the marked leaf to
the root of the shard containing it.

## [`Documentation`](https://docs.rs/shardtree)

## License

Licensed under either of

 * Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or
   http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall
be dual licensed as above, without any additional terms or conditions.
