define_state_group!(plaintext_states_group = {

    plaintext_state {
        eoc => ( emit_text?; )
        eof => ( emit_text?; emit_eof?; )
        _   => ()
    }

});
