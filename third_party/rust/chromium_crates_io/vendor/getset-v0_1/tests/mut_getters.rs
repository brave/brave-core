#[macro_use]
extern crate getset;

use crate::submodule::other::{Generic, Plain, Where};

// For testing `pub(super)`
mod submodule {
    // For testing `pub(in super::other)`
    pub mod other {
        #[derive(MutGetters, Default)]
        #[get_mut]
        pub struct Plain {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: usize,

            /// A doc comment.
            #[get_mut = "pub"]
            public_accessible: usize,
            // /// A doc comment.
            // #[get_mut = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(in super::other)"]
            // scope_accessible: usize,

            // Prefixed getter.
            #[get_mut = "with_prefix"]
            private_prefixed: usize,

            // Prefixed getter.
            #[get_mut = "pub with_prefix"]
            public_prefixed: usize,
        }

        #[derive(MutGetters, Default)]
        #[get_mut]
        pub struct Generic<T: Copy + Clone + Default> {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[get_mut = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[get_mut = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(in super::other)"]
            // scope_accessible: usize,

            // Prefixed getter.
            #[get_mut = "with_prefix"]
            private_prefixed: T,

            // Prefixed getter.
            #[get_mut = "pub with_prefix"]
            public_prefixed: T,
        }

        #[derive(MutGetters, Default)]
        #[get_mut]
        pub struct Where<T>
        where
            T: Copy + Clone + Default,
        {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[get_mut = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[get_mut = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[get_mut = "pub(in super::other)"]
            // scope_accessible: usize,

            // Prefixed getter.
            #[get_mut = "with_prefix"]
            private_prefixed: T,

            // Prefixed getter.
            #[get_mut = "pub with_prefix"]
            public_prefixed: T,
        }

        #[test]
        fn test_plain() {
            let mut val = Plain::default();
            (*val.private_accessible_mut()) += 1;
            (*val.get_private_prefixed_mut()) += 1;
        }

        #[test]
        fn test_generic() {
            let mut val = Generic::<usize>::default();
            (*val.private_accessible_mut()) += 1;
            (*val.get_private_prefixed_mut()) += 1;
        }

        #[test]
        fn test_where() {
            let mut val = Where::<usize>::default();
            (*val.private_accessible_mut()) += 1;
            (*val.get_private_prefixed_mut()) += 1;
        }
    }
}

#[test]
fn test_plain() {
    let mut val = Plain::default();
    (*val.public_accessible_mut()) += 1;
    (*val.get_public_prefixed_mut()) += 1;
}

#[test]
fn test_generic() {
    let mut val = Generic::<usize>::default();
    (*val.public_accessible_mut()) += 1;
    (*val.get_public_prefixed_mut()) += 1;
}

#[test]
fn test_where() {
    let mut val = Where::<usize>::default();
    (*val.public_accessible_mut()) += 1;
    (*val.get_public_prefixed_mut()) += 1;
}
