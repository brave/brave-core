# Correct by Construction Non-Empty List

This package exposes a type `NonEmpty<T>` with a data representation
that guarantees non-emptiness statically:

    struct NonEmpty<T>(T, Vec<T>)

The library is meant to have an interface similar to `std::vec::Vec`:

    use nonempty::NonEmpty;

    let mut l = NonEmpty::new(42);

    assert_eq!(l.first(), &42);

    l.push(36);
    l.push(58);

    let v: Vec<i32> = l.into();
    assert_eq!(v, vec![42, 36, 58]);
