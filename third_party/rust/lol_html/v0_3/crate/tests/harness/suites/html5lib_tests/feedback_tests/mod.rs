mod expected_tokens;

use super::super::{for_each_test_file, get_test_file_reader};
use super::{default_initial_states, Bailout, TestCase};
use hashbrown::HashMap;
use serde_json::from_reader;
use std::fs::File;
use std::io::{BufRead, BufReader};

fn parse_inputs(file: BufReader<File>) -> Vec<String> {
    let mut inputs = Vec::default();
    let mut in_data = 0;

    for line in file.lines().map(Result::unwrap) {
        if line == "#data" {
            in_data = 1;
        } else if line.starts_with('#') {
            in_data = 0;
        } else if in_data > 0 {
            if in_data > 1 {
                let s: &mut String = inputs.last_mut().unwrap();
                s.push('\n');
                s.push_str(&line);
            } else {
                inputs.push(line);
            }
            in_data += 1;
        }
    }

    inputs
}

#[derive(Deserialize, Default)]
pub struct ExpectedBailouts(HashMap<String, Bailout>);

fn load_expected_bailouts() -> ExpectedBailouts {
    from_reader::<_, ExpectedBailouts>(get_test_file_reader("/expected_bailouts.json")).unwrap()
}

pub fn get_test_cases() -> Vec<TestCase> {
    let mut tests = Vec::default();
    let expected_bailouts = load_expected_bailouts();

    let mut add_tests = |file| {
        tests.extend(parse_inputs(file).into_iter().map(|input| {
            let expected_bailout = expected_bailouts.0.get(&input).cloned();

            TestCase {
                description: input
                    .chars()
                    .flat_map(char::escape_default)
                    .collect::<String>()
                    + " (with feedback)",
                expected_tokens: expected_tokens::get(&input),
                input: input.into(),
                initial_states: default_initial_states(),
                double_escaped: false,
                last_start_tag: String::new(),
                expected_bailout,
            }
        }));
    };

    for_each_test_file("html5lib-tests/tree-construction/*.dat", &mut add_tests);
    for_each_test_file("regression/*.dat", &mut add_tests);

    tests
}
