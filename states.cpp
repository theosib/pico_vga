
// Copyright Timothy Miller, 1999

#include "gterm.hpp"

// state machine transition tables
StateOption GTerm::normal_state[] = {
    {0007  , &GTerm::bell        , normal_state},
    {0010  , &GTerm::bs          , normal_state},
    {0011  , &GTerm::tab         , normal_state},
    {0012  , &GTerm::lf          , normal_state},
    {0013  , &GTerm::lf          , normal_state},
    //0014, &GTerm::ff          , normal_state},
    {0014  , &GTerm::lf          , normal_state},
    {0015  , &GTerm::cr          , normal_state},
    {0016  , &GTerm::charset_g1  , normal_state},
    {0017  , &GTerm::charset_g0  , normal_state},
    {0033  , 0                   , esc_state   },
    {-1    , &GTerm::normal_input, normal_state}
};

StateOption GTerm::esc_state[] = {
	{'[', &GTerm::clear_param       , bracket_state      },
	//{']', &GTerm::clear_param       , nonstd_state       },
    {'>', &GTerm::keypad_numeric    , normal_state       },
	{'=', &GTerm::keypad_application, normal_state       },
    {'7', &GTerm::save_cursor       , normal_state       },
	{'8', &GTerm::restore_cursor    , normal_state       },
    {'H', &GTerm::set_tab           , normal_state       },
    {'D', &GTerm::index_down        , normal_state       },
	{'M', &GTerm::index_up          , normal_state       },
    {'T', &GTerm::index_up          , normal_state       }, //FreeBSD cons25 ???
	{'E', &GTerm::next_line         , normal_state       },
    {'c', &GTerm::reset             , normal_state       },
    {'G', 0                         , gfx_state          },

	{'(', 0                         , cset_shiftin_state },
    {')', 0                         , cset_shiftout_state},
	{'#', 0                         , hash_state         },

    {0007  , &GTerm::bell        , esc_state},
    {0010  , &GTerm::bs          , esc_state},
    {0011  , &GTerm::tab         , esc_state},
    {0012  , &GTerm::lf          , esc_state},
    {0013  , &GTerm::lf          , esc_state},
    //{0014, &GTerm::ff          , esc_state},
    {0014  , &GTerm::lf          , esc_state},
    {0015  , &GTerm::cr          , esc_state},

    {-1, 0                         , normal_state}
};

StateOption GTerm::gfx_state[] = {
    {0033  , 0                    ,  normal_state},
    {-1    , &GTerm::gfx_input,     gfx_state}
};

StateOption GTerm::bracket_state[] = {
	{'?', &GTerm::set_q_mode     , q_bracket_state },
	{'"', &GTerm::set_quote_mode , bracket_state }, //???
	{'0', &GTerm::param_digit    , bracket_state },
	{'1', &GTerm::param_digit    , bracket_state },
	{'2', &GTerm::param_digit    , bracket_state },
	{'3', &GTerm::param_digit    , bracket_state },
	{'4', &GTerm::param_digit    , bracket_state },
	{'5', &GTerm::param_digit    , bracket_state },
	{'6', &GTerm::param_digit    , bracket_state },
	{'7', &GTerm::param_digit    , bracket_state },
	{'8', &GTerm::param_digit    , bracket_state },
	{'9', &GTerm::param_digit    , bracket_state },
	{';', &GTerm::next_param     , bracket_state },
	{'D', &GTerm::cursor_left    , normal_state  },
	{'B', &GTerm::cursor_down    , normal_state  },
	//{'e', &GTerm::cursor_down    , normal_state  },//just added
	{'C', &GTerm::cursor_right   , normal_state  },//just added
	//{'a', &GTerm::cursor_right   , normal_state  },
	{'A', &GTerm::cursor_up      , normal_state  },
	{'H', &GTerm::cursor_position, normal_state  },
	{'f', &GTerm::cursor_position, normal_state  },
	{'d', &GTerm::cursor_ypos    , normal_state  }, //???
	{'c', &GTerm::device_attrib  , normal_state  },
	{'P', &GTerm::delete_char    , normal_state  },
	{'h', &GTerm::set_mode       , normal_state  },
	{'l', &GTerm::clear_mode     , normal_state  },
	{'s', &GTerm::save_cursor    , normal_state  },
	{'u', &GTerm::restore_cursor , normal_state  },
	{'x', &GTerm::request_param  , normal_state  },
	{'r', &GTerm::set_margins    , normal_state  },
	{'M', &GTerm::delete_line    , normal_state  },
	{'n', &GTerm::status_report  , normal_state  },
	{'J', &GTerm::erase_display  , normal_state  },
	{'K', &GTerm::erase_line     , normal_state  },
	{'L', &GTerm::insert_line    , normal_state  },
	{'m', &GTerm::set_colors     , normal_state  },
	{'g', &GTerm::clear_tab      , normal_state  },
	{'@', &GTerm::insert_char    , normal_state  },
	{'X', &GTerm::erase_char     , normal_state  },
	{'p', 0                      , normal_state  }, // something to do with levels,???

    //cursor control char inside CSI
    {0007  , &GTerm::bell        , bracket_state},
    {0010  , &GTerm::bs          , bracket_state},
    {0011  , &GTerm::tab         , bracket_state},
    {0012  , &GTerm::lf          , bracket_state},
    {0013  , &GTerm::lf          , bracket_state},
    //{0014, &GTerm::ff          , bracket_state},
    {0014  , &GTerm::lf          , bracket_state},
    {0015  , &GTerm::cr          , bracket_state},

	{-1 , 0                      , normal_state  }
};

// Should put cursor control characters in these groups as well.
// Maybe later.

StateOption GTerm::q_bracket_state[] = {
	{'0', &GTerm::param_digit    , q_bracket_state },
	{'1', &GTerm::param_digit    , q_bracket_state },
	{'2', &GTerm::param_digit    , q_bracket_state },
	{'3', &GTerm::param_digit    , q_bracket_state },
	{'4', &GTerm::param_digit    , q_bracket_state },
	{'5', &GTerm::param_digit    , q_bracket_state },
	{'6', &GTerm::param_digit    , q_bracket_state },
	{'7', &GTerm::param_digit    , q_bracket_state },
	{'8', &GTerm::param_digit    , q_bracket_state },
	{'9', &GTerm::param_digit    , q_bracket_state },
	{';', &GTerm::next_param     , q_bracket_state },
	{'h', &GTerm::set_mode       , normal_state    },
	{'l', &GTerm::clear_mode     , normal_state    },

	{-1 , 0                      , normal_state}
};

StateOption GTerm::cset_shiftin_state[] = {
	{'A', &GTerm::cset_g0_a, normal_state},
	{'B', &GTerm::cset_g0_b, normal_state},
	{'0', &GTerm::cset_g0_0, normal_state},
	{'1', &GTerm::cset_g0_1, normal_state},
	{'2', &GTerm::cset_g0_2, normal_state},
	{-1 , 0, normal_state},
};

StateOption GTerm::cset_shiftout_state[] = {
	{'A', &GTerm::cset_g1_a, normal_state},
	{'B', &GTerm::cset_g1_b, normal_state},
	{'0', &GTerm::cset_g1_0, normal_state},
	{'1', &GTerm::cset_g1_1, normal_state},
	{'2', &GTerm::cset_g1_2, normal_state},
	{-1 , 0, normal_state},
};

//???
StateOption GTerm::hash_state[] = {
	{'3', 0                   , normal_state  },
	{'4', 0                   , normal_state  },
	{'5', 0                   , normal_state  },
	{'6', 0                   , normal_state  },
	{'8', &GTerm::screen_align, normal_state  },
	{-1 , 0                   , normal_state}
};

StateOption GTerm::nonstd_state[] = {
	{'0', &GTerm::param_digit    , nonstd_state },
	{'1', &GTerm::param_digit    , nonstd_state },
	{'2', &GTerm::param_digit    , nonstd_state },
	{'3', &GTerm::param_digit    , nonstd_state },
	{'4', &GTerm::param_digit    , nonstd_state },
	{'5', &GTerm::param_digit    , nonstd_state },
	{'6', &GTerm::param_digit    , nonstd_state },
	{'7', &GTerm::param_digit    , nonstd_state },
	{'8', &GTerm::param_digit    , nonstd_state },
	{'9', &GTerm::param_digit    , nonstd_state },
	{';', &GTerm::next_param     , normal_state },
	{-1, 0, normal_state}
};
