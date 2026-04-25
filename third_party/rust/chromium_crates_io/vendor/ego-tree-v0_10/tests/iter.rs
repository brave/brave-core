use ego_tree::tree;

#[test]
fn into_iter() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec!['a', 'b', 'c', 'd'],
        tree.into_iter().collect::<Vec<_>>()
    );
}

#[test]
fn values() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec![&'a', &'b', &'c', &'d'],
        tree.values().collect::<Vec<_>>()
    );
}

#[test]
fn values_mut() {
    let mut tree = tree!('a' => { 'b', 'c', 'd' });

    for c in tree.values_mut() {
        *c = c.to_ascii_uppercase();
    }

    assert_eq!(
        vec![&'A', &'B', &'C', &'D'],
        tree.values().collect::<Vec<_>>()
    );
}

#[test]
fn nodes() {
    let mut tree = tree!('a' => { 'b' => { 'c' }, 'd' });
    tree.orphan('e').append('f');
    tree.root_mut().append('g');
    assert_eq!(
        vec![&'a', &'b', &'c', &'d', &'e', &'f', &'g'],
        tree.nodes().map(|n| n.value()).collect::<Vec<_>>()
    );
}

#[test]
fn ancestors() {
    let tree = tree!('a' => { 'b' => { 'c' => { 'd' } } });
    let d = tree
        .root()
        .last_child()
        .unwrap()
        .last_child()
        .unwrap()
        .last_child()
        .unwrap();
    assert_eq!(
        vec![&'c', &'b', &'a'],
        d.ancestors().map(|n| n.value()).collect::<Vec<_>>()
    );
}

#[test]
fn ancestors_fused() {
    let tree = tree!('a' => { 'b' => { 'c' => { 'd' } } });
    let d = tree
        .root()
        .last_child()
        .unwrap()
        .last_child()
        .unwrap()
        .last_child()
        .unwrap();

    let mut ancestors = d.ancestors();
    assert_eq!(ancestors.by_ref().count(), 3);
    assert_eq!(ancestors.next(), None);
}

#[test]
fn prev_siblings() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec![&'c', &'b'],
        tree.root()
            .last_child()
            .unwrap()
            .prev_siblings()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn next_siblings() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec![&'c', &'d'],
        tree.root()
            .first_child()
            .unwrap()
            .next_siblings()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn children() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec![&'b', &'c', &'d'],
        tree.root()
            .children()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn children_fused() {
    let tree = tree!('a' => { 'b', 'c', 'd' });

    let mut children = tree.root().children();

    assert_eq!(children.by_ref().count(), 3);
    assert_eq!(children.next(), None);
}

#[test]
fn children_rev() {
    let tree = tree!('a' => { 'b', 'c', 'd' });
    assert_eq!(
        vec![&'d', &'c', &'b'],
        tree.root()
            .children()
            .rev()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn first_children() {
    let tree = tree!('a' => { 'b' => { 'd', 'e' }, 'c' });
    assert_eq!(
        vec![&'b', &'d'],
        tree.root()
            .first_children()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn last_children() {
    let tree = tree!('a' => { 'b', 'c' => { 'd', 'e' } });
    assert_eq!(
        vec![&'c', &'e'],
        tree.root()
            .last_children()
            .map(|n| n.value())
            .collect::<Vec<_>>()
    );
}

#[test]
fn traverse() {
    use ego_tree::iter::Edge;

    #[derive(Debug, PartialEq, Eq)]
    enum Value<'a> {
        Open(&'a char),
        Close(&'a char),
    }

    let tree = tree!('a' => { 'b' => { 'd', 'e' }, 'c' });

    let traversal = tree
        .root()
        .traverse()
        .map(|edge| match edge {
            Edge::Open(node) => Value::Open(node.value()),
            Edge::Close(node) => Value::Close(node.value()),
        })
        .collect::<Vec<_>>();

    assert_eq!(
        &[
            Value::Open(&'a'),
            Value::Open(&'b'),
            Value::Open(&'d'),
            Value::Close(&'d'),
            Value::Open(&'e'),
            Value::Close(&'e'),
            Value::Close(&'b'),
            Value::Open(&'c'),
            Value::Close(&'c'),
            Value::Close(&'a'),
        ],
        &traversal[..]
    );
}

#[test]
fn traverse_fused() {
    let tree = tree!('a' => { 'b' => { 'd', 'e' }, 'c' });

    let mut traversal = tree.root().traverse();

    assert_eq!(traversal.by_ref().count(), 2 * 5);
    assert_eq!(traversal.next(), None);
}

#[test]
fn descendants() {
    let tree = tree!('a' => { 'b' => { 'd', 'e' }, 'c' });

    let descendants = tree
        .root()
        .descendants()
        .map(|n| n.value())
        .collect::<Vec<_>>();

    assert_eq!(&[&'a', &'b', &'d', &'e', &'c',], &descendants[..]);
}

#[test]
fn descendants_fused() {
    let tree = tree!('a' => { 'b' => { 'd', 'e' }, 'c' });

    let mut descendants = tree.root().descendants();

    assert_eq!(descendants.by_ref().count(), 5);
    assert_eq!(descendants.next(), None);
}
