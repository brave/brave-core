//! Implements this tree
//! ```
//! 1
//! ├── 2
//! └── 3
//!     ├── 4
//!     └── 5
//! ```

use ego_tree::{tree, NodeMut, Tree};

fn main() {
    // Manual construction of the tree
    let mut tree: Tree<i32> = Tree::new(1);
    let mut root: NodeMut<i32> = tree.root_mut();
    root.append(2);
    let mut child: NodeMut<i32> = root.append(3);
    child.append(4);
    child.append(5);
    println!("Manual:\n{tree}");

    // Construction of the tree through the tree! macro
    let macro_tree: Tree<i32> = tree!(1 => {2, 3 => {4, 5}});
    println!("Automated:\n{macro_tree}");

    // Prooving equality
    assert_eq!(tree, macro_tree);
}
