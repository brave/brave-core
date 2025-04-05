# Ego Tree

[![crates.io](https://img.shields.io/crates/v/ego-tree?color=dark-green)][crate]
[![downloads](https://img.shields.io/crates/d/ego-tree)][crate]
[![test](https://github.com/rust-scraper/ego-tree/actions/workflows/test.yml/badge.svg)][tests]

`ego-tree` is a Rust crate that provides a Vec-backed ID-tree implementation. It offers a flexible and efficient way to create and manipulate tree structures in Rust, with a focus on performance and ease of use.

`ego-tree` is on [Crates.io][crate] and [GitHub][github].

## Design Philosophy

The design of `ego-tree` is centered around the following principles:

1. **Efficiency**: The tree structure is backed by a Vec, allowing for fast, cache-friendly operations and efficient memory usage.

2. **Flexibility**: Nodes can have any number of children, allowing for the representation of various tree structures.

3. **Stability**: Node references remain valid even after modifying the tree structure, thanks to the use of stable NodeId indices.

4. **Safety**: The API is designed to prevent common errors, such as creating cycles or detaching the root node.

5. **Ergonomics**: The crate provides both low-level operations and high-level conveniences like the `tree!` macro for easy tree construction.

## Key Design Choices

### Vec-Backed Structure

Unlike traditional pointer-based trees, `ego-tree` uses a Vec to store all nodes. This design choice offers several advantages:

- Improved cache locality, potentially leading to better performance
- Simplified memory management
- Easier serialization and deserialization
- Constant-time access to any node by its ID

### Node IDs

Nodes are identified by `NodeId`s, which are wrappers around indices into the underlying Vec. This approach allows for:

- Stable references to nodes, even as the tree structure changes
- Efficient node lookup (O(1) time complexity)
- Compact representation of relationships between nodes

### Immutable and Mutable Node References

The crate provides both `NodeRef` (immutable) and `NodeMut` (mutable) types for working with nodes. This separation allows for:

- Clear distinction between read-only and modifying operations
- Prevention of multiple mutable references to the same node, enforcing Rust's borrowing rules
- Efficient implementation of various tree traversal iterators

### Orphan Nodes

Nodes can be detached from the tree but not removed entirely. This design choice:

- Simplifies certain tree manipulation algorithms
- Allows for temporary detachment and reattachment of subtrees
- Maintains the validity of NodeIds, even for detached nodes

### Rich Iterator Support

The crate provides a variety of iterator types for traversing the tree in different ways. This design:

- Allows for efficient and idiomatic tree traversal
- Supports various algorithms and use cases without sacrificing performance
- Leverages Rust's powerful iterator ecosystem

## Use Cases

`ego-tree` is well-suited for applications that require:

- Efficient representation and manipulation of hierarchical data structures
- Frequent traversal and modification of tree structures
- Stable references to tree nodes across operations
- Serialization and deserialization of tree structures

Some potential use cases include:

- DOM-like structures for document processing
- File system representations
- Organizational hierarchies
- Game scene graphs
- Abstract syntax trees for compilers or interpreters

## Getting Started

Add this to your `Cargo.toml`:

```toml
[dependencies]
ego-tree = "0.6.2"
```

Basic usage:

```rust
use ego_tree::Tree;

let mut tree = Tree::new(1);
let mut root = tree.root_mut();
root.append(2);
let mut child = root.append(3);
child.append(4);
child.append(5);
```

For more detailed usage examples and API documentation, please refer to the [documentation](https://docs.rs/ego-tree).

## License

This project is licensed under the ISC License.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Credits

`ego-tree` is created and maintained by the team of [rust-scraper](https://github.com/rust-scraper).

[crate]: https://crates.io/crates/ego-tree
[github]: https://github.com/rust-scraper/ego-tree
[tests]: https://github.com/rust-scraper/ego-tree/actions/workflows/test.yml
