macro_rules! action_list {
    ( | $self:tt, $input:ident |>
        if $cond:ident
            ( $($if_actions:tt)* )
        else
            ( $($else_actions:tt)* )
    ) => {
        if $self.$cond() {
            action_list!(| $self, $input |> $($if_actions)*);
        } else {
            action_list!(| $self, $input |> $($else_actions)*);
        }
    };

    ( | $self:tt, $input:ident |> { $($code_block:tt)* } ) => ( $($code_block)* );

    ( | $self:tt, $input:ident |> $action:ident $($args:expr),*; $($rest:tt)* ) => {
        trace!(@actions $action $($args:expr)*);
        action!(| $self, $input |> $action $($args),*);
        action_list!(| $self, $input |> $($rest)*);
    };

     ( | $self:tt, $input:ident |> $action:ident ? $($args:expr),*; $($rest:tt)* ) => {
        trace!(@actions $action $($args:expr)*);
        action!(| $self, $input |> $action ? $($args),*);
        action_list!(| $self, $input |> $($rest)*);
    };

    // NOTE: state transition should always be in the end of the action list
    ( | $self:tt, $input:ident|> $($transition:tt)+ ) => {
        trace!(@actions $($transition)+);
        action!(@state_transition | $self, $input |> $($transition)+);
    };

    // NOTE: end of the action list
    ( | $self:tt, $input:ident |> ) => ();


    // State enter action list
    //--------------------------------------------------------------------
    ( @state_enter | $self:tt, $input:ident |> $($actions:tt)+ ) => {
        if $self.is_state_enter() {
            action_list!(|$self, $input|> $($actions)*);
            $self.set_is_state_enter(false);
        }
    };

    // NOTE: don't generate any code for the empty action list
    ( @state_enter | $self:tt, $input:ident |> ) => ();
}
