#[macro_use]
extern crate getset;

use crate::submodule::other::{Generic, Plain, ReferenceCounted, Where};
use std::rc::Rc;
use std::sync::Arc;

// For testing `pub(super)`
mod submodule {
    // For testing `pub(super::other)`
    pub mod other {
        use std::rc::Rc;
        use std::sync::Arc;

        #[derive(CloneGetters)]
        #[get_clone]
        pub struct Plain {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: Box<usize>,

            /// A doc comment.
            #[get_clone = "pub"]
            public_accessible: Box<usize>,
            // /// A doc comment.
            // #[get_clone = "pub(crate)"]
            // crate_accessible: Box<usize>,

            // /// A doc comment.
            // #[get_clone = "pub(super)"]
            // super_accessible: Box<usize>,

            // /// A doc comment.
            // #[get_clone = "pub(super::other)"]
            // scope_accessible: Box<usize>,

            // Prefixed getter.
            #[get_clone = "with_prefix"]
            private_prefixed: Box<usize>,

            // Prefixed getter.
            #[get_clone = "pub with_prefix"]
            public_prefixed: Box<usize>,
        }

        impl Default for Plain {
            fn default() -> Plain {
                Plain {
                    private_accessible: Box::new(17),
                    public_accessible: Box::new(18),
                    private_prefixed: Box::new(19),
                    public_prefixed: Box::new(20),
                }
            }
        }

        #[derive(CloneGetters)]
        #[get_clone]
        pub struct ReferenceCounted {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: Arc<usize>,

            /// A doc comment.
            #[get_clone = "pub"]
            public_accessible: Arc<usize>,
            // /// A doc comment.
            // #[get_clone = "pub(crate)"]
            // crate_accessible: Arc<usize>,

            // /// A doc comment.
            // #[get_clone = "pub(super)"]
            // super_accessible: Arc<usize>,

            // /// A doc comment.
            // #[get_clone = "pub(super::other)"]
            // scope_accessible: Arc<usize>,

            // Prefixed getter.
            #[get_clone = "with_prefix"]
            private_prefixed: Arc<usize>,

            // Prefixed getter.
            #[get_clone = "pub with_prefix"]
            public_prefixed: Arc<usize>,
        }

        impl Default for ReferenceCounted {
            fn default() -> ReferenceCounted {
                ReferenceCounted {
                    private_accessible: Arc::new(17),
                    public_accessible: Arc::new(18),
                    private_prefixed: Arc::new(19),
                    public_prefixed: Arc::new(20),
                }
            }
        }

        #[derive(CloneGetters, Default)]
        #[get_clone]
        pub struct Generic<T: Clone + Default> {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[get_clone = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[get_clone = "pub(crate)"]
            // crate_accessible: T,

            // /// A doc comment.
            // #[get_clone = "pub(super)"]
            // super_accessible: T,

            // /// A doc comment.
            // #[get_clone = "pub(super::other)"]
            // scope_accessible: T,
        }

        #[derive(CloneGetters, Getters, Default)]
        #[get_clone]
        pub struct Where<T>
        where
            T: Clone + Default,
        {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[get_clone = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[get_clone = "pub(crate)"]
            // crate_accessible: T,

            // /// A doc comment.
            // #[get_clone = "pub(super)"]
            // super_accessible: T,

            // /// A doc comment.
            // #[get_clone = "pub(super::other)"]
            // scope_accessible: T,
        }

        #[test]
        fn test_plain() {
            let val = Plain::default();
            val.private_accessible();
        }

        #[test]
        fn test_reference_counted() {
            let val = ReferenceCounted::default();
            val.private_accessible();
        }

        #[test]
        fn test_generic() {
            let val = Generic::<Box<usize>>::default();
            val.private_accessible();

            let val = Generic::<Rc<usize>>::default();
            val.private_accessible();

            let val = Generic::<Arc<usize>>::default();
            val.private_accessible();
        }

        #[test]
        fn test_where() {
            let val = Where::<Box<usize>>::default();
            val.private_accessible();

            let val = Where::<Rc<usize>>::default();
            val.private_accessible();

            let val = Where::<Arc<usize>>::default();
            val.private_accessible();
        }

        #[test]
        fn test_prefixed_plain() {
            let val = Plain::default();
            assert_eq!(19, *val.get_private_prefixed());
        }

        #[test]
        fn test_prefixed_reference_counted() {
            let val = ReferenceCounted::default();
            assert_eq!(19, *val.get_private_prefixed());
        }
    }
}

#[test]
fn test_plain() {
    let val = Plain::default();
    assert_eq!(18, *val.public_accessible());
}

#[test]
fn test_reference_counted() {
    let val = ReferenceCounted::default();
    assert_eq!(18, *val.public_accessible());
}

#[test]
fn test_generic() {
    let val = Generic::<Box<usize>>::default();
    assert_eq!(Box::default(), val.public_accessible());

    let val = Generic::<Rc<usize>>::default();
    assert_eq!(Rc::default(), val.public_accessible());

    let val = Generic::<Arc<usize>>::default();
    assert_eq!(Arc::default(), val.public_accessible());
}

#[test]
fn test_where() {
    let val = Where::<Box<usize>>::default();
    assert_eq!(Box::default(), val.public_accessible());

    let val = Where::<Rc<usize>>::default();
    assert_eq!(Rc::default(), val.public_accessible());

    let val = Where::<Arc<usize>>::default();
    assert_eq!(Arc::default(), val.public_accessible());
}

#[test]
fn test_prefixed_plain() {
    let val = Plain::default();
    assert_eq!(20, *val.get_public_prefixed());
}

#[test]
fn test_prefixed_reference_counted() {
    let val = ReferenceCounted::default();
    assert_eq!(20, *val.get_public_prefixed());
}
