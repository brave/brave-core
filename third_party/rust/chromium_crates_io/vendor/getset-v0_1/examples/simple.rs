use std::sync::Arc;

use getset::{CloneGetters, CopyGetters, Getters, MutGetters, Setters, WithSetters};

#[derive(Getters, Setters, WithSetters, MutGetters, CopyGetters, CloneGetters, Default)]
pub struct Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get, set, get_mut, set_with)]
    private: T,

    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get_copy = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    public: T,

    /// Arc supported through CloneGetters
    #[getset(get_clone = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    arc: Arc<u16>,
}

fn main() {
    let mut foo = Foo::default();
    foo.set_private(1);
    (*foo.private_mut()) += 1;
    assert_eq!(*foo.private(), 2);
    foo = foo.with_private(3);
    assert_eq!(*foo.private(), 3);
    assert_eq!(*foo.arc(), 0);
}
