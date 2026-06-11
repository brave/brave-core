use getset::{CopyGetters, Getters, MutGetters, Setters, WithSetters};

#[test]
fn test_unary_tuple() {
    #[derive(Setters, Getters, MutGetters, WithSetters)]
    struct UnaryTuple(#[getset(set, get, get_mut, set_with)] i32);

    let mut tup = UnaryTuple(42);
    assert_eq!(tup.get(), &42);
    assert_eq!(tup.get_mut(), &mut 42);
    tup.set(43);
    assert_eq!(tup.get(), &43);
    tup = tup.set_with(44);
    assert_eq!(tup.get(), &44);

    #[derive(CopyGetters)]
    struct CopyUnaryTuple(#[getset(get_copy)] i32);

    let tup = CopyUnaryTuple(42);
    assert_eq!(tup.get(), 42);
}

#[test]
fn test_unary_tuple_with_attrs() {
    #[derive(Setters, Getters, MutGetters, WithSetters)]
    #[getset(set, get, get_mut, set_with)]
    struct UnaryTuple(i32);

    let mut tup = UnaryTuple(42);
    assert_eq!(tup.get(), &42);
    assert_eq!(tup.get_mut(), &mut 42);
    tup.set(43);
    assert_eq!(tup.get(), &43);
    tup = tup.set_with(44);
    assert_eq!(tup.get(), &44);

    #[derive(CopyGetters)]
    #[getset(get_copy)]
    struct CopyUnaryTuple(i32);

    let tup = CopyUnaryTuple(42);
    assert_eq!(tup.get(), 42);
}
