use super::attribute_matcher::AttributeMatcher;
use super::program::{AddressRange, ExecutionBranch, Instruction, Program};
use super::{
    Ast, AstNode, AttributeComparisonExpr, Expr, OnAttributesExpr, OnTagNameExpr, Predicate,
    SelectorState,
};
use crate::base::{Bytes, HasReplacementsError};
use crate::html::LocalName;
use encoding_rs::Encoding;
use selectors::attr::{AttrSelectorOperator, ParsedCaseSensitivity};
use std::fmt::Debug;
use std::hash::Hash;
use std::iter;

/// An expression using only the tag name of an element.
pub type CompiledLocalNameExpr = Box<dyn Fn(&SelectorState<'_>, &LocalName<'_>) -> bool + Send>;
/// An expression using the attributes of an element.
pub type CompiledAttributeExpr =
    Box<dyn Fn(&SelectorState<'_>, &AttributeMatcher<'_>) -> bool + Send>;

#[derive(Default)]
struct ExprSet {
    pub local_name_exprs: Vec<CompiledLocalNameExpr>,
    pub attribute_exprs: Vec<CompiledAttributeExpr>,
}

pub(crate) struct AttrExprOperands {
    pub name: Bytes<'static>,
    pub value: Bytes<'static>,
    pub case_sensitivity: ParsedCaseSensitivity,
}

impl Expr<OnTagNameExpr> {
    #[inline]
    pub fn compile_expr<F: Fn(&SelectorState<'_>, &LocalName<'_>) -> bool + Send + 'static>(
        &self,
        f: F,
    ) -> CompiledLocalNameExpr {
        if self.negation {
            Box::new(move |s, a| !f(s, a))
        } else {
            Box::new(f)
        }
    }
}

trait Compilable {
    fn compile(
        &self,
        encoding: &'static Encoding,
        exprs: &mut ExprSet,
        enable_nth_of_type: &mut bool,
    );
}

impl Compilable for Expr<OnTagNameExpr> {
    fn compile(
        &self,
        encoding: &'static Encoding,
        exprs: &mut ExprSet,
        enable_nth_of_type: &mut bool,
    ) {
        let expr = match &self.simple_expr {
            OnTagNameExpr::ExplicitAny => self.compile_expr(|_, _| true),
            OnTagNameExpr::Unmatchable => self.compile_expr(|_, _| false),
            OnTagNameExpr::LocalName(local_name) => {
                match LocalName::from_str_without_replacements(local_name, encoding)
                    .map(LocalName::into_owned)
                {
                    Ok(local_name) => self.compile_expr(move |_, actual| *actual == local_name),
                    // NOTE: selector value can't be converted to the given encoding, so
                    // it won't ever match.
                    Err(_) => self.compile_expr(|_, _| false),
                }
            }
            &OnTagNameExpr::NthChild(nth) => {
                self.compile_expr(move |state, _| state.cumulative.is_nth(nth))
            }
            &OnTagNameExpr::NthOfType(nth) => {
                *enable_nth_of_type = true;
                self.compile_expr(move |state, _| {
                    state
                        .typed
                        .expect("Counter for type required at this point")
                        .is_nth(nth)
                })
            }
        };

        exprs.local_name_exprs.push(expr);
    }
}

impl Expr<OnAttributesExpr> {
    #[inline]
    pub fn compile_expr<
        F: Fn(&SelectorState<'_>, &AttributeMatcher<'_>) -> bool + Send + 'static,
    >(
        &self,
        f: F,
    ) -> CompiledAttributeExpr {
        if self.negation {
            Box::new(move |s, a| !f(s, a))
        } else {
            Box::new(f)
        }
    }
}

#[inline]
fn compile_literal(
    encoding: &'static Encoding,
    lit: &str,
) -> Result<Bytes<'static>, HasReplacementsError> {
    Bytes::from_str_without_replacements(lit, encoding).map(Bytes::into_owned)
}

#[inline]
fn compile_literal_lowercase(
    encoding: &'static Encoding,
    lit: &str,
) -> Result<Bytes<'static>, HasReplacementsError> {
    compile_literal(encoding, &lit.to_ascii_lowercase())
}

#[inline]
fn compile_operands(
    encoding: &'static Encoding,
    name: &str,
    value: &str,
) -> Result<(Bytes<'static>, Bytes<'static>), HasReplacementsError> {
    Ok((
        compile_literal_lowercase(encoding, name)?,
        compile_literal(encoding, value)?,
    ))
}

impl Compilable for Expr<OnAttributesExpr> {
    fn compile(&self, encoding: &'static Encoding, exprs: &mut ExprSet, _: &mut bool) {
        let expr_result =
            match &self.simple_expr {
                OnAttributesExpr::Id(id) => compile_literal(encoding, id)
                    .map(|id| self.compile_expr(move |_, m| m.has_id(&id))),

                OnAttributesExpr::Class(class) => compile_literal(encoding, class)
                    .map(|class| self.compile_expr(move |_, m| m.has_class(&class))),

                OnAttributesExpr::AttributeExists(name) => compile_literal(encoding, name)
                    .map(|name| self.compile_expr(move |_, m| m.has_attribute(&name))),

                &OnAttributesExpr::AttributeComparisonExpr(AttributeComparisonExpr {
                    ref name,
                    ref value,
                    case_sensitivity,
                    operator,
                }) => compile_operands(encoding, name, value).map(move |(name, value)| {
                    let operands = AttrExprOperands {
                        name,
                        value,
                        case_sensitivity,
                    };
                    match operator {
                        AttrSelectorOperator::Equal => {
                            self.compile_expr(move |_, m| m.attr_eq(&operands))
                        }
                        AttrSelectorOperator::Includes => self
                            .compile_expr(move |_, m| m.matches_splitted_by_whitespace(&operands)),
                        AttrSelectorOperator::DashMatch => {
                            self.compile_expr(move |_, m| m.has_dash_matching_attr(&operands))
                        }
                        AttrSelectorOperator::Prefix => {
                            self.compile_expr(move |_, m| m.has_attr_with_prefix(&operands))
                        }
                        AttrSelectorOperator::Suffix => {
                            self.compile_expr(move |_, m| m.has_attr_with_suffix(&operands))
                        }
                        AttrSelectorOperator::Substring => {
                            self.compile_expr(move |_, m| m.has_attr_with_substring(&operands))
                        }
                    }
                }),
            };

        exprs
            .attribute_exprs
            .push(expr_result.unwrap_or_else(|_| self.compile_expr(|_, _| false)));
    }
}

pub(crate) struct Compiler<P>
where
    P: PartialEq + Eq + Copy + Debug + Hash,
{
    encoding: &'static Encoding,
    instructions: Box<[Option<Instruction<P>>]>,
    free_space_start: usize,
}

impl<P: 'static> Compiler<P>
where
    P: PartialEq + Eq + Copy + Debug + Hash,
{
    #[must_use]
    pub fn new(encoding: &'static Encoding) -> Self {
        Self {
            encoding,
            instructions: Default::default(),
            free_space_start: 0,
        }
    }

    fn compile_predicate(
        &self,
        Predicate {
            on_tag_name_exprs,
            on_attr_exprs,
        }: &Predicate,
        branch: ExecutionBranch<P>,
        enable_nth_of_type: &mut bool,
    ) -> Instruction<P> {
        let mut exprs = ExprSet::default();

        on_tag_name_exprs
            .iter()
            .for_each(|c| c.compile(self.encoding, &mut exprs, enable_nth_of_type));
        on_attr_exprs
            .iter()
            .for_each(|c| c.compile(self.encoding, &mut exprs, enable_nth_of_type));

        let ExprSet {
            local_name_exprs,
            attribute_exprs,
        } = exprs;

        debug_assert!(
            !local_name_exprs.is_empty() || !attribute_exprs.is_empty(),
            "Predicate should contain expressions"
        );

        Instruction {
            associated_branch: branch,
            local_name_exprs: local_name_exprs.into(),
            attribute_exprs: attribute_exprs.into(),
        }
    }

    /// Reserves space for a set of nodes, returning the range for the nodes to be placed
    #[inline]
    fn reserve(&mut self, nodes: &[AstNode<P>]) -> AddressRange {
        let addr_range = self.free_space_start..self.free_space_start + nodes.len();

        self.free_space_start = addr_range.end;

        debug_assert!(self.free_space_start <= self.instructions.len());

        addr_range
    }

    #[inline]
    fn compile_descendants(
        &mut self,
        nodes: Vec<AstNode<P>>,
        enable_nth_of_type: &mut bool,
    ) -> Option<AddressRange> {
        if nodes.is_empty() {
            None
        } else {
            Some(self.compile_nodes(nodes, enable_nth_of_type))
        }
    }

    fn compile_nodes(
        &mut self,
        nodes: Vec<AstNode<P>>,
        enable_nth_of_type: &mut bool,
    ) -> AddressRange {
        // NOTE: we need sibling nodes to be in a contiguous region, so
        // we can reference them by range instead of vector of addresses.
        let addr_range = self.reserve(&nodes);

        for (node, position) in nodes.into_iter().zip(addr_range.clone()) {
            let branch = ExecutionBranch {
                matched_payload: node.payload,
                jumps: self.compile_descendants(node.children, enable_nth_of_type),
                hereditary_jumps: self.compile_descendants(node.descendants, enable_nth_of_type),
            };

            self.instructions[position] =
                Some(self.compile_predicate(&node.predicate, branch, enable_nth_of_type));
        }

        addr_range
    }

    #[must_use]
    pub fn compile(mut self, ast: Ast<P>) -> Program<P> {
        let mut enable_nth_of_type = false;
        self.instructions = iter::repeat_with(|| None)
            .take(ast.cumulative_node_count)
            .collect();

        let entry_points = self.compile_nodes(ast.root, &mut enable_nth_of_type);

        Program {
            instructions: self
                .instructions
                .into_vec()
                .into_iter()
                .map(|o| o.unwrap())
                .collect(),
            entry_points,
            enable_nth_of_type,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::html::Namespace;
    use crate::rewritable_units::Token;
    use crate::selectors_vm::{tests::test_with_token, TryExecResult};
    use crate::test_utils::ASCII_COMPATIBLE_ENCODINGS;
    use encoding_rs::UTF_8;
    use hashbrown::HashSet;

    macro_rules! assert_instr_res {
        ($res:expr, $should_match:expr, $selector:expr, $input:expr, $encoding:expr) => {{
            let expected_payload = if *$should_match {
                Some(vec![0].into_iter().collect::<HashSet<_>>())
            } else {
                None
            };

            assert_eq!(
                $res.map(|b| b.matched_payload.to_owned()),
                expected_payload,
                "Instruction didn't produce expected matching result\n\
                 selector: {:#?}\n\
                 input: {:#?}\n\
                 encoding: {:?}\n\
                 ",
                $selector,
                $input,
                $encoding.name()
            );
        }};
    }

    fn compile(
        selectors: &[&str],
        encoding: &'static Encoding,
        expected_entry_point_count: usize,
    ) -> Program<usize> {
        let mut ast = Ast::default();

        for (idx, selector) in selectors.iter().enumerate() {
            ast.add_selector(&selector.parse().unwrap(), idx);
        }

        let program = Compiler::new(encoding).compile(ast);

        assert_eq!(
            program.entry_points.end - program.entry_points.start,
            expected_entry_point_count
        );

        program
    }

    fn with_negated<'i>(
        selector: &str,
        test_cases: &[(&'i str, bool)],
    ) -> Vec<(String, Vec<(&'i str, bool)>)> {
        vec![
            (selector.to_string(), test_cases.to_owned()),
            (
                format!(":not({selector})"),
                test_cases
                    .iter()
                    .map(|(input, should_match)| (*input, !should_match))
                    .collect(),
            ),
        ]
    }

    fn with_start_tag(
        html: &str,
        encoding: &'static Encoding,
        mut action: impl FnMut(LocalName<'_>, AttributeMatcher<'_>),
    ) {
        test_with_token(html, encoding, |t| match t {
            Token::StartTag(t) => {
                let (input, attrs) = t.raw_attributes();
                let tag_name = t.name();
                let attr_matcher = AttributeMatcher::new(input, attrs, Namespace::Html);
                let local_name =
                    LocalName::from_str_without_replacements(&tag_name, encoding).unwrap();

                action(local_name, attr_matcher);
            }
            _ => panic!("Start tag expected"),
        });
    }

    fn for_each_test_case<T>(
        test_cases: &[(&str, T)],
        encoding: &'static Encoding,
        action: impl Fn(&str, &T, &SelectorState<'_>, LocalName<'_>, AttributeMatcher<'_>),
    ) {
        for (input, matching_data) in test_cases {
            with_start_tag(input, encoding, |local_name, attr_matcher| {
                let counter = Default::default();
                let state = SelectorState {
                    cumulative: &counter,
                    typed: None,
                };
                action(input, matching_data, &state, local_name, attr_matcher);
            });
        }
    }

    fn assert_attr_expr_matches(
        selector: &str,
        encoding: &'static Encoding,
        test_cases: &[(&str, bool)],
    ) {
        let program = compile(&[selector], encoding, 1);
        let instr = &program.instructions[program.entry_points.start];

        for_each_test_case(
            test_cases,
            encoding,
            |input, should_match, state, local_name, attr_matcher| {
                assert!(
                    matches!(
                        instr.try_exec_without_attrs(state, &local_name),
                        TryExecResult::AttributesRequired
                    ),
                    "Instruction should not execute without attributes"
                );

                let multi_step_res = instr.complete_exec_with_attrs(state, &attr_matcher);
                let res = instr.exec(state, &local_name, &attr_matcher);

                assert_eq!(multi_step_res, res);
                assert_instr_res!(res, should_match, selector, input, encoding);
            },
        );
    }

    fn assert_non_attr_expr_matches_and_negation_reverses_match(
        selector: &str,
        encoding: &'static Encoding,
        test_cases: &[(&str, bool)],
    ) {
        for (selector, test_cases) in with_negated(selector, test_cases) {
            let program = compile(&[&selector], encoding, 1);
            let instr = &program.instructions[program.entry_points.start];

            for_each_test_case(
                &test_cases,
                encoding,
                |input, should_match, state, local_name, attr_matcher| {
                    #[allow(clippy::match_wild_err_arm)]
                    let multi_step_res = match instr.try_exec_without_attrs(state, &local_name) {
                        TryExecResult::Branch(b) => Some(b),
                        TryExecResult::Fail => None,
                        TryExecResult::AttributesRequired => {
                            panic!("Should match without attribute request")
                        }
                    };

                    let res = instr.exec(state, &local_name, &attr_matcher);

                    assert_eq!(multi_step_res, res);

                    assert_instr_res!(res, should_match, selector, input, encoding);
                },
            );
        }
    }

    fn assert_attr_expr_matches_and_negation_reverses_match(
        selector: &str,
        encoding: &'static Encoding,
        test_cases: &[(&str, bool)],
    ) {
        for (selector, test_cases) in &with_negated(selector, test_cases) {
            assert_attr_expr_matches(selector, encoding, test_cases);
        }
    }

    macro_rules! exec_generic_instr {
        ($instr:expr, $state:expr, $local_name:expr, $attr_matcher:expr) => {{
            let res = $instr.exec($state, &$local_name, &$attr_matcher);

            let multi_step_res = match $instr.try_exec_without_attrs($state, &$local_name) {
                TryExecResult::Branch(b) => Some(b),
                TryExecResult::Fail => None,
                TryExecResult::AttributesRequired => {
                    $instr.complete_exec_with_attrs(&*$state, &$attr_matcher)
                }
            };

            assert_eq!(res, multi_step_res);

            res
        }};
    }

    fn assert_generic_expr_matches(
        selector: &str,
        encoding: &'static Encoding,
        test_cases: &[(&str, bool)],
    ) {
        let program = compile(&[selector], encoding, 1);
        let instr = &program.instructions[program.entry_points.start];

        for_each_test_case(
            test_cases,
            encoding,
            |input, should_match, state, local_name, attr_matcher| {
                let res = exec_generic_instr!(instr, state, local_name, attr_matcher);

                assert_instr_res!(res, should_match, selector, input, encoding);
            },
        );
    }

    macro_rules! exec_instr_range {
        ($range:expr, $program:expr, $state:expr, $local_name:expr, $attr_matcher:expr) => {{
            let mut matched_payload = HashSet::default();
            let mut jumps = Vec::default();
            let mut hereditary_jumps = Vec::default();

            for addr in $range.clone() {
                let res = exec_generic_instr!(
                    $program.instructions[addr],
                    $state,
                    $local_name,
                    $attr_matcher
                );

                if let Some(res) = res {
                    for &p in res.matched_payload.iter() {
                        matched_payload.insert(p);
                    }

                    if let Some(ref j) = res.jumps {
                        jumps.push(j.to_owned());
                    }

                    if let Some(ref j) = res.hereditary_jumps {
                        hereditary_jumps.push(j.to_owned());
                    }
                }
            }

            (matched_payload, jumps, hereditary_jumps)
        }};
    }

    macro_rules! assert_payload {
        ($actual:expr, $expected:expr, $selectors:expr, $input:expr) => {
            assert_eq!(
                $actual,
                $expected.iter().cloned().collect::<HashSet<_>>(),
                "Instructions didn't produce expected payload\n\
                 selectors: {:#?}\n\
                 input: {:#?}\n\
                 ",
                $selectors,
                $input
            );
        };
    }

    fn assert_entry_points_match(
        selectors: &[&str],
        expected_entry_point_count: usize,
        test_cases: &[(&str, Vec<usize>)],
    ) {
        let program = compile(selectors, UTF_8, expected_entry_point_count);

        // NOTE: encoding of the individual components is tested by other tests,
        // so we use only UTF-8 here.
        for_each_test_case(
            test_cases,
            UTF_8,
            |input, expected_payload, state, local_name, attr_matcher| {
                let (matched_payload, _, _) = exec_instr_range!(
                    program.entry_points,
                    program,
                    state,
                    local_name,
                    attr_matcher
                );

                assert_payload!(matched_payload, expected_payload, selectors, input);
            },
        );
    }

    #[test]
    fn compiled_non_attr_expression() {
        for encoding in &ASCII_COMPATIBLE_ENCODINGS {
            assert_non_attr_expr_matches_and_negation_reverses_match(
                "*",
                encoding,
                &[
                    ("<div>", true),
                    ("<span>", true),
                    ("<anything-else>", true),
                    ("<FуБар>", true),
                ],
            );

            assert_non_attr_expr_matches_and_negation_reverses_match(
                r#"[foo*=""]"#,
                encoding,
                &[
                    ("<div>", false),
                    ("<span>", false),
                    ("<anything-else>", false),
                ],
            );

            assert_non_attr_expr_matches_and_negation_reverses_match(
                "div",
                encoding,
                &[
                    ("<div>", true),
                    ("<Div>", true),
                    ("<divnotdiv>", false),
                    ("<span>", false),
                    ("<anything-else>", false),
                ],
            );

            // NOTE: case-insensitivity for local names that can't be represented by a hash.
            assert_non_attr_expr_matches_and_negation_reverses_match(
                "fooƔ",
                encoding,
                &[("<fooƔ", true), ("<FooƔ>", true)],
            );

            assert_non_attr_expr_matches_and_negation_reverses_match(
                "span",
                encoding,
                &[
                    ("<div>", false),
                    ("<span>", true),
                    ("<spanϰ>", false),
                    ("<anything-else>", false),
                ],
            );

            assert_non_attr_expr_matches_and_negation_reverses_match(
                "anything-else",
                encoding,
                &[
                    ("<div>", false),
                    ("<span>", false),
                    ("<anything-else>", true),
                ],
            );
        }
    }

    #[test]
    fn compiled_attr_expression() {
        for encoding in &ASCII_COMPATIBLE_ENCODINGS {
            assert_attr_expr_matches_and_negation_reverses_match(
                "#foo⾕",
                encoding,
                &[
                    ("<div bar=baz qux id='foo⾕'>", true),
                    ("<div iD='foo⾕'>", true),
                    ("<div bar=baz qux id='foo1'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                ".c2",
                encoding,
                &[
                    ("<div bar=baz class='c1 c2 c3 c4' qux>", true),
                    ("<div CLASS='c1 c2 c3 c4'>", true),
                    ("<div class='c1 c23 c4'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                "[foo⽅]",
                encoding,
                &[
                    ("<div foo1 foo2 foo⽅>", true),
                    ("<div FOo⽅=123>", true),
                    ("<div id='baz'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo="barα"]"#,
                encoding,
                &[
                    ("<div fOo='barα'>", true),
                    ("<div foo=barα>", true),
                    ("<div foo='BaRα'>", false),
                    ("<div foo='42'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo="barα" i]"#,
                encoding,
                &[
                    ("<div fOo='barα'>", true),
                    ("<div foo=barα>", true),
                    ("<div foo='BaRα'>", true),
                    ("<div foo='42'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo~="μbar3"]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbar2 μbar3\tbar4'>", true),
                    ("<div foo='μbar3'>", true),
                    ("<div foo='bar1 bar2 μBAR3'>", false),
                    ("<div foo='42'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo~="μbar3" i]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbar2 μbar3\tbar4'>", true),
                    ("<div foo='μbar3'>", true),
                    ("<div foo='bar1 bar2 μBAR3'>", true),
                    ("<div foo='42'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            // NOTE: "lang" attribute always case-insesitive for HTML elements.
            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[lang|="en" s]"#,
                encoding,
                &[
                    ("<div lang='en-GB'>", true),
                    ("<div lang='en-US'>", true),
                    ("<div lang='en'>", true),
                    ("<div lang='En'>", false),
                    ("<div lang='En-GB'>", false),
                    ("<div lang='fr'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[lang|="en"]"#,
                encoding,
                &[
                    ("<div lang='en-GB'>", true),
                    ("<div lang='en-US'>", true),
                    ("<div lang='en'>", true),
                    ("<div lang='En'>", true),
                    ("<div lang='En-GB'>", true),
                    ("<div lang='fr'>", false),
                    ("<div bar=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo^="barБ"]"#,
                encoding,
                &[
                    ("<div fOo='barБ1\nbar2 bar3\tbar4'>", true),
                    ("<div foo='barБ'>", true),
                    ("<div foo='BaRБ'>", false),
                    ("<div foo='bazbarБ'>", false),
                    ("<div foo='42'>", false),
                    ("<div barБ=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo^="bar㕦" i]"#,
                encoding,
                &[
                    ("<div fOo='bar㕦1\nbar2 bar3\tbar4'>", true),
                    ("<div foo='bar㕦'>", true),
                    ("<div foo='BaR㕦'>", true),
                    ("<div foo='bazbar㕦'>", false),
                    ("<div foo='42'>", false),
                    ("<div bar㕦=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo*="barφ"]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbarφ2 bar3\tbar4'>", true),
                    ("<div foo='barφ'>", true),
                    ("<div foo='Barφ'>", false),
                    ("<div foo='42BaRφ42'>", false),
                    ("<div foo='bazbatbarφ'>", true),
                    ("<div foo='42'>", false),
                    ("<div barφ=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo*="barφ" i]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbarφ2 bar3\tbar4'>", true),
                    ("<div Foo='barφ'>", true),
                    ("<div foo='42BaRφ42'>", true),
                    ("<div foo='bazbatbarφ'>", true),
                    ("<div foo='42'>", false),
                    ("<div barφ=baz qux>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo$="barЫ"]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbar2 bar3\tbarЫ'>", true),
                    ("<div foo='barЫ'>", true),
                    ("<div foo='bazbarЫ'>", true),
                    ("<div foo='BaRЫ'>", false),
                    ("<div foo='42'>", false),
                    ("<div barЫ=baz qux>", false),
                    ("<div foo='bar'>", false),
                ],
            );

            assert_attr_expr_matches_and_negation_reverses_match(
                r#"[foo$="barЫ" i]"#,
                encoding,
                &[
                    ("<div fOo='bar1\nbar2 bar3\tbarЫ'>", true),
                    ("<div foo='barЫ'>", true),
                    ("<div foo='bazbarЫ'>", true),
                    ("<div foo='BaRЫ'>", true),
                    ("<div foo='42'>", false),
                    ("<div barЫ=baz qux>", false),
                ],
            );

            assert_attr_expr_matches(
                r#"#foo1.c1.c2[foo3][foo2$="barЫ"]"#,
                encoding,
                &[
                    (
                        "<div id='foo1' class='c4 c2 c3 c1' foo3 foo2=heybarЫ>",
                        true,
                    ),
                    (
                        "<div ID='foo1' class='c4 c2 c3 c1' foo3=test foo2=barЫ>",
                        true,
                    ),
                    ("<div id='foo1' class='c4 c2 c3 c1' foo3>", false),
                    (
                        "<div id='foo1' class='c4 c2 c3 c5' foo3 foo2=heybarЫ>",
                        false,
                    ),
                    (
                        "<div id='foo12' class='c4 c2 c3 c5' foo3 foo2=heybarЫ>",
                        false,
                    ),
                ],
            );
        }
    }

    #[test]
    fn generic_expressions() {
        for encoding in &ASCII_COMPATIBLE_ENCODINGS {
            assert_generic_expr_matches(
                r#"div#foo1.c1.c2[foo3੦][foo2$="bar"]"#,
                encoding,
                &[
                    (
                        "<div id='foo1' class='c4 c2 c3 c1' foo3੦ foo2=heybar>",
                        true,
                    ),
                    (
                        "<span id='foo1' class='c4 c2 c3 c1' foo3੦ foo2=heybar>",
                        false,
                    ),
                    (
                        "<div ID='foo1' class='c4 c2 c3 c1' foo3੦=test foo2=bar>",
                        true,
                    ),
                    ("<div id='foo1' class='c4 c2 c3 c1' foo3੦>", false),
                    (
                        "<div id='foo1' class='c4 c2 c3 c5' foo3੦ foo2=heybar>",
                        false,
                    ),
                    (
                        "<div id='foo12' class='c4 c2 c3 c5' foo3੦ foo2=heybar>",
                        false,
                    ),
                ],
            );

            assert_generic_expr_matches(
                r"some-thing[lang|=en]",
                encoding,
                &[
                    ("<some-thing lang='en-GB'", true),
                    ("<some-thing lang='en-US'", true),
                    ("<some-thing lang='fr'>", false),
                    ("<some-thing lang>", false),
                    ("<span lang='en-GB'", false),
                ],
            );
        }
    }

    #[test]
    fn multiple_entry_points() {
        assert_entry_points_match(
            &["div", "div.c1.c2", "#foo", ".c1#foo"],
            4,
            &[
                ("<div>", vec![0]),
                ("<div class='c3 c2  c1'>", vec![0, 1]),
                ("<div class='c1 c2' id=foo>", vec![0, 1, 2, 3]),
                ("<div class='c1' id=foo>", vec![0, 2, 3]),
                ("<span class='c1 c2'>", vec![]),
            ],
        );

        assert_entry_points_match(
            &["span, [foo$=bar]"],
            2,
            &[
                ("<span>", vec![0]),
                ("<div fOo=testbar>", vec![0]),
                ("<span foo=bar>", vec![0]),
            ],
        );
    }

    #[test]
    fn jumps() {
        let selectors = [
            "div > .c1",
            "div > .c2",
            "div #d1",
            "div #d2",
            "[foo=bar] #id1 > #id2",
        ];

        let program = compile(&selectors, UTF_8, 2);

        macro_rules! exec {
            ($html:expr, $add_range:expr, $expected_payload:expr) => {{
                let mut jumps = Vec::default();
                let mut hereditary_jumps = Vec::default();
                let counter = Default::default();
                let state = SelectorState {
                    cumulative: &counter,
                    typed: None,
                };

                with_start_tag($html, UTF_8, |local_name, attr_matcher| {
                    let res =
                        exec_instr_range!($add_range, program, &state, local_name, attr_matcher);

                    assert_payload!(res.0, $expected_payload, selectors, $html);

                    jumps = res.1;
                    hereditary_jumps = res.2;
                });

                (jumps, hereditary_jumps)
            }};
        }

        {
            let (jumps, hereditary_jumps) = exec!("<div>", program.entry_points, []);

            assert_eq!(jumps.len(), 1);
            assert_eq!(hereditary_jumps.len(), 1);

            {
                let (jumps, hereditary_jumps) = exec!("<span class='c1 c2'>", jumps[0], [0, 1]);

                assert_eq!(jumps.len(), 0);
                assert_eq!(hereditary_jumps.len(), 0);
            }

            {
                let (jumps, hereditary_jumps) = exec!("<span class='c2'>", jumps[0], [1]);

                assert_eq!(jumps.len(), 0);
                assert_eq!(hereditary_jumps.len(), 0);
            }

            {
                let (jumps, hereditary_jumps) = exec!("<h1 id=d2>", hereditary_jumps[0], [3]);

                assert_eq!(jumps.len(), 0);
                assert_eq!(hereditary_jumps.len(), 0);
            }
        }

        {
            let (jumps, hereditary_jumps) = exec!("<div foo=bar>", program.entry_points, []);

            assert_eq!(jumps.len(), 1);
            assert_eq!(hereditary_jumps.len(), 2);
        }

        {
            let (jumps, hereditary_jumps) = exec!("<span foo=bar>", program.entry_points, []);

            assert_eq!(jumps.len(), 0);
            assert_eq!(hereditary_jumps.len(), 1);

            {
                let (jumps, hereditary_jumps) = exec!("<table id=id1>", hereditary_jumps[0], []);

                assert_eq!(jumps.len(), 1);
                assert_eq!(hereditary_jumps.len(), 0);

                {
                    let (jumps, hereditary_jumps) = exec!("<span id=id2>", jumps[0], [4]);

                    assert_eq!(jumps.len(), 0);
                    assert_eq!(hereditary_jumps.len(), 0);
                }
            }
        }
    }
}
