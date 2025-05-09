use ego_tree::tree;

#[test]
fn prepend_subtree() {
    let mut tree = tree!('a' => { 'b', 'c' => { 'd', 'e' } });
    let node_id = tree.root().first_child().unwrap().id();
    let mut node = tree.get_mut(node_id).unwrap();
    assert_eq!(node.value(), &'b');

    let subtree = tree!('f' => { 'g', 'h' => { 'i', 'j' } });
    let mut root_subtree = node.prepend_subtree(subtree);
    assert_eq!(root_subtree.parent().unwrap().value(), &'b');
    assert_eq!(
        root_subtree.parent().unwrap().parent().unwrap().value(),
        &'a'
    );

    let new_tree =
        tree!('a' => { 'b' => { 'f' => { 'g', 'h' => { 'i', 'j' } } }, 'c' => { 'd', 'e' } });
    assert_eq!(format!("{:#?}", tree), format!("{:#?}", new_tree));
}

#[test]
fn append_subtree() {
    let mut tree = tree!('a' => { 'b', 'c' });
    let mut node = tree.root_mut();
    assert_eq!(node.value(), &'a');

    let subtree = tree!('d' => { 'e', 'f' });
    let mut root_subtree = node.append_subtree(subtree);
    assert_eq!(root_subtree.parent().unwrap().value(), &'a');

    let new_tree = tree!('a' => { 'b', 'c', 'd' => { 'e', 'f' } });
    assert_eq!(format!("{:#?}", tree), format!("{:#?}", new_tree));
}
