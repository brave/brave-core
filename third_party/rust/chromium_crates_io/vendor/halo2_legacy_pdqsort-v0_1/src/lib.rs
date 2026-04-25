pub mod sort;

#[cfg(test)]
mod rust_slice;

#[cfg(test)]
mod tests {
    use crate::sort;

    #[test]
    fn test_basic() {
        let mut a = [3i32, 2, 1];
        sort::quicksort(&mut a, i32::lt);
        assert_eq!(a, [1, 2, 3]);
    }
}
