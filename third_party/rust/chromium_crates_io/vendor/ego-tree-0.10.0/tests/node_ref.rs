use ego_tree::tree;

#[test]
fn value() {
    let tree = tree!('a');
    assert_eq!(&'a', tree.root().value());
}

#[test]
fn parent() {
    let tree = tree!('a' => { 'b' });
    let b = tree.root().first_child().unwrap();
    assert_eq!(tree.root(), b.parent().unwrap());
}

#[test]
fn prev_sibling() {
    let tree = tree!('a' => { 'b', 'c' });
    let c = tree.root().last_child().unwrap();
    assert_eq!(tree.root().first_child(), c.prev_sibling());
}

#[test]
fn next_sibling() {
    let tree = tree!('a' => { 'b', 'c' });
    let b = tree.root().first_child().unwrap();
    assert_eq!(tree.root().last_child(), b.next_sibling());
}

#[test]
fn first_child() {
    let tree = tree!('a' => { 'b', 'c' });
    assert_eq!(&'b', tree.root().first_child().unwrap().value());
}

#[test]
fn last_child() {
    let tree = tree!('a' => { 'b', 'c' });
    assert_eq!(&'c', tree.root().last_child().unwrap().value());
}

#[test]
fn has_siblings() {
    let tree = tree!('a' => { 'b', 'c' });
    assert!(!tree.root().has_siblings());
    assert!(tree.root().first_child().unwrap().has_siblings());
}

#[test]
fn has_children() {
    let tree = tree!('a' => { 'b', 'c' });
    assert!(tree.root().has_children());
    assert!(!tree.root().first_child().unwrap().has_children());
}

#[test]
fn clone() {
    let tree = tree!('a');
    let one = tree.root();
    let two = one;
    assert_eq!(one, two);
}

#[test]
fn eq() {
    let tree = tree!('a');
    assert_eq!(tree.root(), tree.root());
}

#[test]
#[should_panic]
fn neq() {
    let tree = tree!('a' => { 'b', 'c' });
    assert_eq!(tree.root(), tree.root().first_child().unwrap());
}

#[test]
#[should_panic]
fn neq_tree() {
    let one = tree!('a');
    let two = one.clone();
    assert_eq!(one.root(), two.root());
}
