//! A round-trip test on randomly generated strings.

use dev_util::Bucket;

#[test]
fn test() {
    const SAMPLE_SIZE: usize = 100;

    let random_strings = Bucket::interspersed();
    let mut iter = random_strings.iter();

    for _ in 0..SAMPLE_SIZE {
        let string = iter.next().unwrap();

        let simd_encoded = simd_cesu8::encode(string);
        let compare_encoded = cesu8::to_cesu8(string);

        assert_eq!(simd_encoded, compare_encoded);

        let simd_decoded = simd_cesu8::decode(&simd_encoded).unwrap();
        let compare_decoded = cesu8::from_cesu8(&compare_encoded).unwrap();

        assert_eq!(simd_decoded, compare_decoded);
    }
}
