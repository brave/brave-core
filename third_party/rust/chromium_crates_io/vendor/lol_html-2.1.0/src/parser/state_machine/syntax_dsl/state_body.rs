macro_rules! state_body {
    ( | [ $self:tt, $ctx:tt, $input:ident, $ch:ident ] |> [$($arms:tt)+], [$($enter_actions:tt)*] ) => {
        action_list!(@state_enter |$self, $ctx, $input|> $($enter_actions)*);
        state_body!(@map_arms | [$self, $ctx, $input, $ch] |> [$($arms)+], [])
    };


    // Recursively expand each arm's pattern
    //--------------------------------------------------------------------
    ( @map_arms
        | $scope_vars:tt |>
        [ $pat:tt => ( $($actions:tt)* ) $($rest:tt)* ], [ $($expanded:tt)* ]
    ) => {
        arm_pattern!(|[ $scope_vars, [$($rest)*], [$($expanded)*] ]|> $pat => ( $($actions)* ))
    };

    ( @map_arms
        | $scope_vars:tt |>
        [], [$($expanded:tt)*]
    ) => {
        state_body!(@match_block |$scope_vars|> $($expanded)*);
    };


    // Callback for the expand_arm_pattern
    //--------------------------------------------------------------------
    ( @callback
        | [ $scope_vars:tt, [$($pending:tt)*], [$($expanded:tt)*] ] |>
        $($expanded_arm:tt)*
    ) => {
        state_body!(@map_arms | $scope_vars |> [$($pending)*], [$($expanded)* $($expanded_arm)*])
    };


    // Character match block
    //--------------------------------------------------------------------
    ( @match_block
        | [ $self:tt, $ctx:tt, $input:ident, $ch:ident ] |>
        $( $pat:pat_param $(|$pat_cont:pat)* $(if $pat_expr:expr)* => ( $($actions:tt)* ) )*
    ) => {
        // NOTE: guard against unreachable patterns
        // (e.g. such may occur if `eof => ...` arm comes before `eoc => ...` arm.)
        #[deny(unreachable_patterns)]
        match $ch {
            $(
                $pat $(| $pat_cont)* $(if $pat_expr)* => {
                    action_list!(|$self, $ctx, $input|> $($actions)*);
                }
            )*
        }
    };
}
