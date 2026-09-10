#pragma once

// ASCII 'liquid density morph' Wi-Fi signal node used as the Wi-Fi section
// intro (DedSec / Watch Dogs style): a node grows expanding signal arcs over
// four frames, blended by the morph engine with hacking-glyph glitches.
// Generated (cropped to a centred 43-col box) from the reference web engine.

static const int WIFI_COLS = 43;
static const int WIFI_ROWS = 22;

static const char* const wifi_splash_frames[4][WIFI_ROWS] = {
  { // FRAME 0: base node
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                    ...                    )~",
    R"~(                  .d$$$b.                  )~",
    R"~(                  $$$$$$$                  )~",
    R"~(                  'Y$$$P'                  )~",
    R"~(                    '''                    )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 1: first arc
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                  ._   _.                  )~",
    R"~(               .d$$$$$$$$$b.               )~",
    R"~(             .d$$$$$$$$$$$$$b.             )~",
    R"~(             $$$$P'     'Y$$$$             )~",
    R"~(             `''           ''`             )~",
    R"~(                    ...                    )~",
    R"~(                  .d$$$b.                  )~",
    R"~(                  $$$$$$$                  )~",
    R"~(                  'Y$$$P'                  )~",
    R"~(                    '''                    )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 2: second arc
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(             _,.,---------.,,_             )~",
    R"~(         _,d$$$$$$$$$$$$$$$$$$$b,_         )~",
    R"~(       .d$$$$$$$$$$$$$$$$$$$$$$$$$b.       )~",
    R"~(       $$$$P''               ''Y$$$$       )~",
    R"~(       `^'                       '^`       )~",
    R"~(                  ._   _.                  )~",
    R"~(               .d$$$$$$$$$b.               )~",
    R"~(             .d$$$$$$$$$$$$$b.             )~",
    R"~(             $$$$P'     'Y$$$$             )~",
    R"~(             `''           ''`             )~",
    R"~(                    ...                    )~",
    R"~(                  .d$$$b.                  )~",
    R"~(                  $$$$$$$                  )~",
    R"~(                  'Y$$$P'                  )~",
    R"~(                    '''                    )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 3: full glitch
    R"~(                                           )~",
    R"~(        _,.,-------------------.,,_        )~",
    R"~(    _,d%%%%%%%%%%%%%%%%%%%%%%%%%%%%%b,_    )~",
    R"~(  .d%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%b.  )~",
    R"~(  %%%%P''                         ''Y%%%%  )~",
    R"~(             _,.,---------.,,_             )~",
    R"~(         _,d%%%%%%%%%%%%%%%%%%%b,_         )~",
    R"~(       .d%%%%%%%%%%%%%%%%%%%%%%%%%b.       )~",
    R"~(       %%%%P''               ''Y%%%%       )~",
    R"~(                  ._   _.                  )~",
    R"~(               .d%%%%%%%%%b.               )~",
    R"~(             .d%%%%%%%%%%%%%b.             )~",
    R"~(             %%%%P'     'Y%%%%             )~",
    R"~(                                           )~",
    R"~(                    ...                    )~",
    R"~(                  .d%%%b.                  )~",
    R"~(                  %%%%%%%                  )~",
    R"~(                  'Y%%%P'                  )~",
    R"~(                    '''                    )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  }
};
