use ego_tree::{tree, Tree};

#[test]
fn root() {
    let macro_tree = tree!('a');
    let manual_tree = Tree::new('a');
    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn single_child() {
    let macro_tree = tree!('a' => { 'b' });

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn single_child_comma() {
    let macro_tree = tree! {
        'a' => {
            'b',
        }
    };

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn leaves() {
    let macro_tree = tree!('a' => { 'b', 'c', 'd' });

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b');
    manual_tree.root_mut().append('c');
    manual_tree.root_mut().append('d');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn leaves_comma() {
    let macro_tree = tree! {
        'a' => {
            'b',
            'c',
            'd',
        }
    };

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b');
    manual_tree.root_mut().append('c');
    manual_tree.root_mut().append('d');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn nested_single_child() {
    let macro_tree = tree!('a' => { 'b' => { 'c' } });

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b').append('c');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn nested_single_child_comma() {
    let macro_tree = tree! {
        'a' => {
            'b' => {
                'c',
            },
        }
    };

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b').append('c');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn nested_leaves() {
    let macro_tree = tree!('a' => { 'b' => { 'c', 'd', 'e' } });

    let mut manual_tree = Tree::new('a');
    {
        let mut root = manual_tree.root_mut();
        let mut node = root.append('b');
        node.append('c');
        node.append('d');
        node.append('e');
    }

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn nested_leaves_comma() {
    let macro_tree = tree! {
        'a' => {
            'b' => {
                'c',
                'd',
                'e',
            },
        }
    };

    let mut manual_tree = Tree::new('a');
    {
        let mut root = manual_tree.root_mut();
        let mut node = root.append('b');
        node.append('c');
        node.append('d');
        node.append('e');
    }

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn nested_nested() {
    let macro_tree = tree!('a' => { 'b' => { 'c' => { 'd' } } });

    let mut manual_tree = Tree::new('a');
    manual_tree.root_mut().append('b').append('c').append('d');

    assert_eq!(manual_tree, macro_tree);
}

#[test]
fn mixed() {
    let macro_tree = tree! {
        'a' => {
            'b',
            'd' => { 'e', 'f' },
            'g' => { 'h' => { 'i' } },
            'j',
        }
    };

    let mut manual_tree = Tree::new('a');
    {
        let mut node = manual_tree.root_mut();
        node.append('b');
        {
            let mut d = node.append('d');
            d.append('e');
            d.append('f');
        }
        node.append('g').append('h').append('i');
        node.append('j');
    }

    assert_eq!(manual_tree, macro_tree);
}
