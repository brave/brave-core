use std::assert_eq;

use ego_tree::tree;

#[test]
fn sort() {
    let mut tree = tree!('a' => { 'd' => { 'e', 'f' }, 'c',  'b' });
    tree.root_mut().sort();
    assert_eq!(
        vec![&'b', &'c', &'d'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );
    assert_eq!(
        tree.to_string(),
        tree!('a' => { 'b', 'c',  'd' => { 'e', 'f' } }).to_string()
    );
}

#[test]
fn sort_by() {
    let mut tree = tree!('a' => { 'c', 'd', 'b' });
    tree.root_mut().sort_by(|a, b| b.value().cmp(a.value()));
    assert_eq!(
        vec![&'d', &'c', &'b'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );

    let mut tree = tree!('a' => { 'c','d', 'e', 'b' });
    tree.root_mut().sort_by(|a, b| b.value().cmp(a.value()));
    assert_eq!(
        vec![&'e', &'d', &'c', &'b'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );
}

#[test]
fn sort_by_key() {
    let mut tree = tree!("1a" => { "2b", "4c", "3d" });
    tree.root_mut()
        .sort_by_key(|a| a.value().split_at(1).0.parse::<i32>().unwrap());
    assert_eq!(
        vec!["2b", "3d", "4c"],
        tree.root()
            .children()
            .map(|n| *n.value())
            .collect::<Vec<_>>(),
    );
}

#[test]
fn sort_id() {
    let mut tree = tree!('a' => { 'd', 'c', 'b' });
    tree.root_mut().sort();
    assert_ne!(
        vec![&'d', &'c', &'b'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );
    tree.root_mut().sort_by_key(|n| n.id());
    assert_eq!(
        vec![&'d', &'c', &'b'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );
}

#[test]
fn sort_by_id() {
    let mut tree = tree!('a' => { 'd', 'b', 'c' });
    tree.root_mut().sort_by(|a, b| b.id().cmp(&a.id()));
    assert_eq!(
        vec![&'c', &'b', &'d'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>(),
    );
}
