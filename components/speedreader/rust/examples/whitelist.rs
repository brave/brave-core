extern crate adblock;
extern crate reqwest;
extern crate speedreader;
extern crate url;

use std::fs;
use std::io;

fn main() -> io::Result<()> {
    let mut correct = true;
    let mut full_whitelist = Vec::new();
    for entry in fs::read_dir("data/urls/")? {
        let entry = entry?;
        if entry.file_type()?.is_dir() {
            let path = entry.path();
            println!("Processing {:?}", path);
            let mut readable_path = path.clone();
            readable_path.push("readable.txt");
            let expected_readable = fs::read_to_string(&readable_path)?
                .split_ascii_whitespace()
                .map(|s: &str| s.to_owned())
                .collect::<Vec<String>>();

            let mut unreadable_path = path.clone();
            unreadable_path.push("unreadable.txt");
            let expected_unreadable = fs::read_to_string(&unreadable_path)?
                .split_ascii_whitespace()
                .map(|s: &str| s.to_owned())
                .collect::<Vec<String>>();

            let mut whitelist_path = path.clone();
            whitelist_path.push("whitelist.txt");
            let mut whitelist = fs::read_to_string(&whitelist_path)?
                .split('\n')
                .map(|s: &str| s.trim().to_owned())
                .filter(|f| detect_network_filter(f))
                .collect::<Vec<String>>();

            let adblock_matcher = adblock::engine::Engine::from_rules_debug(&whitelist);

            for url in expected_readable {
                let check = adblock_matcher.check_network_urls(&url, &url, "");
                if !check.matched {
                    println!("Error: expected {} to match provided filters", url);
                    correct = false;
                }
            }

            for url in expected_unreadable {
                let check = adblock_matcher.check_network_urls(&url, &url, "");
                if check.matched {
                    println!(
                        "Error: expected {} to not match provided filters, but matched {:?}",
                        url, check.filter
                    );
                    correct = false;
                }
            }

            if correct {
                full_whitelist.append(&mut whitelist);
            }
        }
    }

    let whitelist_filename = "whitelist.txt";
    println!(
        "Everything correct, writing used rules to {}",
        whitelist_filename
    );
    fs::write(whitelist_filename, full_whitelist.join("\n").as_bytes())?;
    Ok(())
}

/**
 * Given a single line (string), checks if this would likely be a cosmetic
 * filter, a network filter or something that is not supported. This check is
 * performed before calling a more specific parser to create an instance of
 * `NetworkFilter` or `CosmeticFilter`.
 */
fn detect_network_filter(filter: &str) -> bool {
    // Ignore comments
    if filter.len() == 1
        || filter.starts_with('!')
        || (filter.starts_with('#') && filter[1..].starts_with(char::is_whitespace))
        || filter.starts_with("[Adblock")
    {
        return false;
    }

    if filter.starts_with('|') || filter.starts_with("@@|") {
        return true;
    }

    // Ignore Adguard cosmetics
    // `$$`
    if filter.find("$$").is_some() {
        return false;
    }

    // Check if filter is cosmetics
    if let Some(sharp_index) = filter.find('#') {
        let after_sharp_index = sharp_index + 1;

        // Ignore Adguard cosmetics
        // `#$#` `#@$#`
        // `#%#` `#@%#`
        // `#?#`
        if filter[after_sharp_index..].starts_with(/* #@$# */ "@$#")
            || filter[after_sharp_index..].starts_with(/* #@%# */ "@%#")
            || filter[after_sharp_index..].starts_with(/* #%# */ "%#")
            || filter[after_sharp_index..].starts_with(/* #$# */ "$#")
            || filter[after_sharp_index..].starts_with(/* #?# */ "?#")
        {
            return false;
        } else if filter[after_sharp_index..].starts_with(/* ## */ '#')
            || filter[after_sharp_index..].starts_with(/* #@# */ "@#")
        {
            // Parse supported cosmetic filter
            // `##` `#@#`
            return false;
        }
    }

    // Everything else is a network filter
    true
}
