#[test]
fn doctest_downstream_crate ()
{
    assert!(
        ::std::process::Command::new(env!("CARGO"))
            .current_dir("downstream_crate")
            .args(&["test", "--doc", "--features", "integration-tests"])
            .status()
            .unwrap()
            .success()
    );
}
