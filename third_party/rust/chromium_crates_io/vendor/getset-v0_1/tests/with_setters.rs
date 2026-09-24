#[macro_use]
extern crate getset;

use crate::submodule::other::{Generic, Plain, Where};

// For testing `pub(super)`
mod submodule {
    // For testing `pub(in super::other)`
    pub mod other {
        #[derive(WithSetters, Default)]
        #[set_with]
        pub struct Plain {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: usize,

            /// A doc comment.
            #[set_with = "pub"]
            public_accessible: usize,

            /// This field is used for testing chaining.
            #[set_with = "pub"]
            second_public_accessible: bool,
            // /// A doc comment.
            // #[set_with = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(in super::other)"]
            // scope_accessible: usize,
        }

        #[derive(WithSetters, Default)]
        #[set_with]
        pub struct Generic<T: Copy + Clone + Default> {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[set_with = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[set_with = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(in super::other)"]
            // scope_accessible: usize,
        }

        #[derive(WithSetters, Default)]
        #[set_with]
        pub struct Where<T>
        where
            T: Copy + Clone + Default,
        {
            /// A doc comment.
            /// Multiple lines, even.
            private_accessible: T,

            /// A doc comment.
            #[set_with = "pub"]
            public_accessible: T,
            // /// A doc comment.
            // #[set_with = "pub(crate)"]
            // crate_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(super)"]
            // super_accessible: usize,

            // /// A doc comment.
            // #[set_with = "pub(in super::other)"]
            // scope_accessible: usize,
        }

        #[test]
        fn test_plain() {
            let val: Plain = Plain::default();
            let _: Plain = val.with_private_accessible(1);
        }

        #[test]
        fn test_generic() {
            let val: Generic<i32> = Generic::default();
            let _: Generic<i32> = val.with_private_accessible(1);
        }

        #[test]
        fn test_where() {
            let val: Where<i32> = Where::default();
            let _: Where<i32> = val.with_private_accessible(1);
        }
    }
}

#[test]
fn test_plain() {
    let val: Plain = Plain::default();
    let _: Plain = val.with_public_accessible(1);
}

#[test]
fn test_generic() {
    let val: Generic<i32> = Generic::default();
    let _: Generic<i32> = val.with_public_accessible(1);
}

#[test]
fn test_where() {
    let val: Where<i32> = Where::default();
    let _: Where<i32> = val.with_public_accessible(1);
}

#[test]
fn test_chaining() {
    let val: Plain = Plain::default();
    let _: Plain = val
        .with_public_accessible(1)
        .with_second_public_accessible(true);
}
