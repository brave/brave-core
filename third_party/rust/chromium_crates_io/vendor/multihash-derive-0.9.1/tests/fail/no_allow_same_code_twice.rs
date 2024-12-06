#[derive(Default)]
struct FooHasher {

}

impl multihash_derive::Hasher for FooHasher {
    fn update(&mut self, input: &[u8]) { }

    fn finalize(&mut self) -> &[u8] {
        todo!()
    }

    fn reset(&mut self) { }
}

#[derive(Clone, Debug, Eq, PartialEq, Copy, multihash_derive::MultihashDigest)]
#[mh(alloc_size = 32)]
pub enum Code {
    #[mh(code = 0x0, hasher = FooHasher)]
    Foo1,
    #[mh(code = 0x0, hasher = FooHasher)]
    Foo2,
}

fn main() {

}
