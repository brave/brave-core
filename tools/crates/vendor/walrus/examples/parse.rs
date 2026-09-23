// A small example which is primarily used to help benchmark parsing in walrus
// right now.

fn main() {
    env_logger::init();
    let a = std::env::args().nth(1).unwrap();
    walrus::Module::from_file(a).unwrap();
}
