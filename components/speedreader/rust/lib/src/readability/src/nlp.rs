use regex::{Regex, RegexSet};
use std::cmp::max;
use std::collections::HashSet;

// A list of common abbreviations
// Taken from https://www.enchantedlearning.com/abbreviations/
static EN_ABBREVIATIONS_LIST: [&str; 80] = [
    "abbr.", "Acad.", "alt.", "A.D.", "A.M.", "apt.", "Assn.", "Aug.", "Ave.", "B.A.", "B.S.",
    "B.C.", "Blvd.", "Capt.", "ctr.", "cent.", "Col.", "Cpl.", "Corp.", "Ct.", "D.C.", "Dr.",
    "Dr.", "ed.", "etc.", "Feb.", "ft.", "Ft.", "gal.", "Gen.", "Gov.", "hwy.", "e.g.", "i.e.",
    "in.", "inc.", "Jan.", "Jr.", "Lk.", "Ln.", "lib.", "lat.", "lib.", "Lt.", "Ltd.", "long.",
    "M.D.", "Mr.", "Msgr.", "mo.", "mt.", "mus.", "Nov.", "no.", "Oct.", "oz.", "p.", "pt.", "pl.",
    "pop.", "P.M.", "Prof.", "qt.", "Rd.", "R.N.", "Sept.", "Sgt.", "Sr.", "Sta.", "St.", "ste.",
    "Sun.", "Ter.", "Tpk.", "Univ.", "U.S.", "U.S.A.", "vol.", "wt.", "EE.UU.",
];

lazy_static! {
    // MAX_ABBREVIATION_LEN is the maximum size we consider for an abbreviation.
    pub static ref MAX_ABBREVIATION_LEN: usize = EN_ABBREVIATIONS_LIST
        .iter()
        .fold(0, |m, x| return max(m, x.chars().count()));
    static ref EN_ABBREVIATIONS: HashSet<&'static str> =
        EN_ABBREVIATIONS_LIST.iter().cloned().collect();
    static ref POSSIBLE_SENTENCE_BOUNDARIES: Regex = Regex::new(r"[\S^(.?!)]+[.?!] ").unwrap();
    static ref ABBREVIATION_HEURISTICS: RegexSet = RegexSet::new(&[
        r"^\p{Uppercase}\p{Lowercase}\.$",
        r"^\p{Uppercase}\.(?:\p{Uppercase}\.)?$",
    ]).unwrap();
}

/// Determines if a slice is an abbreviation by checking a list of common abbreviations and some
/// simple heuristics.
#[inline]
pub fn is_abbreviation(s: &str) -> bool {
    let len = s.chars().count();
    if len > *MAX_ABBREVIATION_LEN {
        false
    } else {
        EN_ABBREVIATIONS.contains(s) || ABBREVIATION_HEURISTICS.is_match(s)
    }
}

/// Looks for the first sentence boundary. Three cases must be met:
///     (1) The token at the boundary must not list of common abbreviations.
///     (2) The token must should not pass heuristics that match abbreviations
///         of the form `A.`, `F.A.`, or `Ms.`.
///     (3) The next token must be capitalized.
pub fn first_sentence_boundary(s: &str) -> Option<usize> {
    for m in POSSIBLE_SENTENCE_BOUNDARIES.find_iter(&s) {
        let is_next_token_caps = s[m.end()..]
            .trim_start()
            .chars()
            .next()
            .map(|c| c.is_uppercase())
            .unwrap_or_else(|| true);
        if !is_next_token_caps {
            continue;
        }
        let byte_offset = m.end() - 1;
        if m.as_str().ends_with(". ") {
            if !is_abbreviation(&m.as_str().trim_end()) {
                return Some(byte_offset);
            }
        } else {
            return Some(byte_offset);
        }
    }
    None
}

#[macro_use]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn max_abbreviation_len() {
        // Sanity check. Need to change this if extending list size.
        assert_eq!(*MAX_ABBREVIATION_LEN, 6);
    }

    #[test]
    fn test_is_abbreviation() {
        // In the set
        assert!(is_abbreviation("Sr."));
        assert!(is_abbreviation("Nov."));
        assert!(is_abbreviation("P.M."));
        assert!(is_abbreviation("e.g."));

        // Not in set, matches first heuristic
        assert!(is_abbreviation("Ms."));

        // Not in set, matches second heuristic
        assert!(is_abbreviation("A."));
        assert!(is_abbreviation("F.A."));

        // Things that shouldn't match
        assert!(!is_abbreviation("Baseball."));
        assert!(!is_abbreviation("Soccer."));
        assert!(!is_abbreviation("and."));
        assert!(!is_abbreviation("TH.EN."));
        assert!(!is_abbreviation("America."));
        assert!(!is_abbreviation("it."));
    }

    macro_rules! test_first_sentence_boundary {
        ($name:ident, $input:expr, $expected:expr) => {
            #[test]
            fn $name() {
                let mut s = $input.to_string();
                let byte_offset = first_sentence_boundary(&s).unwrap();
                s.truncate(byte_offset);
                assert_eq!($expected, s);
            }
        };
    }

    test_first_sentence_boundary!(
        many_abbreviations,
        "Dr. Stevens is the best surgeon in the U.S.A. and all of North America. Next sentence here.",
        "Dr. Stevens is the best surgeon in the U.S.A. and all of North America."
    );

    test_first_sentence_boundary!(
        other_punctuation,
        "Dr. Stevens is the best surgeon in the U.S.A.! Next sentence here.",
        "Dr. Stevens is the best surgeon in the U.S.A.!"
    );

    test_first_sentence_boundary!(
        espanol,
        "EE.UU. saluda \"cooperación\" de talibanes en nueva evacuación tras su retirada de Afganistán. Eliminar este oración",
        "EE.UU. saluda \"cooperación\" de talibanes en nueva evacuación tras su retirada de Afganistán."
    );

    test_first_sentence_boundary!(
        yahoo,
        "Yahoo! is an American web services provider. Yahoo was established in January 1994.",
        "Yahoo! is an American web services provider."
    );

    test_first_sentence_boundary!(
        end_of_string_edge_case,
        "This string ends with a space. ",
        "This string ends with a space."
    );
}
