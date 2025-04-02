use ego_tree::{tree, Tree};

#[test]
fn new() {
    let tree = Tree::new('a');
    let root = tree.root();
    assert_eq!(&'a', root.value());
    assert_eq!(None, root.parent());
    assert_eq!(None, root.prev_sibling());
    assert_eq!(None, root.next_sibling());
    assert_eq!(None, root.first_child());
    assert_eq!(None, root.last_child());
}

#[test]
fn root() {
    let tree = Tree::new('a');
    assert_eq!(&'a', tree.root().value());
}

#[test]
fn root_mut() {
    let mut tree = Tree::new('a');
    assert_eq!(&'a', tree.root_mut().value());
}

#[test]
fn orphan() {
    let mut tree = Tree::new('a');
    let mut orphan = tree.orphan('b');
    assert_eq!(&'b', orphan.value());
    assert!(orphan.parent().is_none());
}

#[test]
fn get() {
    let tree = Tree::new('a');
    let id = tree.root().id();
    assert_eq!(Some(tree.root()), tree.get(id));
}

#[test]
fn get_mut() {
    let mut tree = Tree::new('a');
    let id = tree.root().id();
    assert_eq!(Some('a'), tree.get_mut(id).map(|mut n| *n.value()));
}

#[test]
fn clone() {
    let one = Tree::new('a');
    let two = one.clone();
    assert_eq!(one, two);
}

#[test]
fn eq() {
    let one = Tree::new('a');
    let two = Tree::new('a');
    assert_eq!(one, two);
}

#[test]
#[should_panic]
fn neq() {
    let one = Tree::new('a');
    let two = Tree::new('b');
    assert_eq!(one, two);
}

#[test]
fn insert_id_after() {
    let mut tree = tree! {
        "root" => {
            "a" => {
                "child 1",
            },
            "b" => {
                "child 2",
            },
        }
    };

    let a = tree.root().first_child().unwrap().id();
    let b = tree.root().last_child().unwrap().id();

    assert_eq!(2, tree.root().children().count());
    assert_eq!(1, tree.get(a).unwrap().children().count());
    assert_eq!(1, tree.get(b).unwrap().children().count());

    let child_1 = tree.get(a).unwrap().first_child().unwrap().id();
    tree.get_mut(b).unwrap().insert_id_after(child_1);

    assert_eq!(
        0,
        tree.get(a).unwrap().children().count(),
        "child 1 should be moved from a"
    );
    assert_eq!(
        1,
        tree.get(b).unwrap().children().count(),
        "b should be unchanged"
    );
    assert_eq!(
        child_1,
        tree.root().last_child().unwrap().id(),
        "child 1 should be last child of root"
    );
    assert_eq!(3, tree.root().children().count());
}

#[test]
fn insert_id_before() {
    let mut tree = tree! {
        "root" => {
            "a" => {
                "child 1",
            },
            "b" => {
                "child 2",
            },
        }
    };

    let a = tree.root().first_child().unwrap().id();
    let b = tree.root().last_child().unwrap().id();

    assert_eq!(2, tree.root().children().count());
    assert_eq!(1, tree.get(a).unwrap().children().count());
    assert_eq!(1, tree.get(b).unwrap().children().count());

    let child_1 = tree.get(a).unwrap().first_child().unwrap().id();
    tree.get_mut(b).unwrap().insert_id_before(child_1);

    assert_eq!(
        0,
        tree.get(a).unwrap().children().count(),
        "child 1 should be moved from a"
    );
    assert_eq!(
        1,
        tree.get(b).unwrap().children().count(),
        "b should be unchanged"
    );
    assert_eq!(
        b,
        tree.root().last_child().unwrap().id(),
        "b should be last child of root"
    );
    assert_eq!(
        child_1,
        tree.get(b).unwrap().prev_sibling().unwrap().id(),
        "child 1 should be between a and b"
    );
    assert_eq!(3, tree.root().children().count());
}

#[test]
fn test_map_values() {
    let str_tree = tree! {
        "root" => {
            "a" => {
                "child 1",
            },
            "b" => {
                "child 2",
            },
        }
    };

    let identity_mapped_tree = str_tree.clone().map(|value| value);

    // If we pass the identity function to `.map_values()`,
    // then we expect the tree to effectively remain untouched:
    assert_eq!(str_tree, identity_mapped_tree);

    let string_tree = str_tree.clone().map(|value| value.to_owned());

    // A `&str` will produce the same output for `.to_string()` as its equivalent `String`,
    // so the output of `.to_string()` should match for corresponding trees as well:
    assert_eq!(str_tree.to_string(), string_tree.to_string());
}

#[test]
fn test_map_value_refs() {
    let str_tree = tree! {
        "root" => {
            "a" => {
                "child 1",
            },
            "b" => {
                "child 2",
            },
        }
    };

    let identity_mapped_tree = str_tree.map_ref(|&value| value);

    // If we pass the identity function to `.map_values()`,
    // then we expect the tree to effectively remain untouched:
    assert_eq!(str_tree, identity_mapped_tree);

    let string_tree = str_tree.map_ref(|&value| value.to_owned());

    // A `&str` will produce the same output for `.to_string()` as its equivalent `String`,
    // so the output of `.to_string()` should match for corresponding trees as well:
    assert_eq!(str_tree.to_string(), string_tree.to_string());
}

#[test]
fn test_display() {
    let tree = tree! {
        "root" => {
            "a" => {
                "child 1",
            },
            "b" => {
                "child 2",
            },
        }
    };

    let repr = format!("{tree}");
    let expected = "root\n├── a\n│   └── child 1\n└── b\n    └── child 2\n";

    assert_eq!(repr, expected);

    let tree = tree! {
        "root" => {
            "a", "b" => {
                "x", "y"
            }, "c"
        }
    };

    let repr = format!("{tree}");
    let expected = "root\n├── a\n├── b\n│   ├── x\n│   └── y\n└── c\n";

    assert_eq!(repr, expected);
}
