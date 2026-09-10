#pragma once

// ASCII 'liquid density morph' GPS location pin used as the GPS section intro
// (DedSec / Watch Dogs style): the pin acquires expanding satellite rings and
// locks, blended by the morph engine with hacking-glyph glitches. Authored to
// match the Wi-Fi / Bluetooth intros.

static const int GPSI_COLS = 43;
static const int GPSI_ROWS = 22;

static const char* const gps_splash_frames[4][GPSI_ROWS] = {
  { // FRAME 0: idle pin
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                    .d$$b.                 )~",
    R"~(                 .d$$$$$$b.                )~",
    R"~(                 $$$'  '$$$                )~",
    R"~(                 $$$ () $$$                )~",
    R"~(                 '$$$..$$$'                )~",
    R"~(                   '$$$$$'                 )~",
    R"~(                     'Y$P'                 )~",
    R"~(                       YP                  )~",
    R"~(                       '                   )~",
    R"~(                                           )~",
    R"~(                 '-.___.-'                 )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 1: acquiring
    R"~(                                           )~",
    R"~(                 :  : :  :                 )~",
    R"~(              :             :              )~",
    R"~(            :                 :            )~",
    R"~(          :                     :          )~",
    R"~(         :          .d$$b.       :         )~",
    R"~(                 .d$$$$$$b.                )~",
    R"~(        :        $$$'  '$$$       :        )~",
    R"~(        :        $$$ () $$$       :        )~",
    R"~(        :        '$$$..$$$'       :        )~",
    R"~(                   '$$$$$'                 )~",
    R"~(         :           'Y$P'       :         )~",
    R"~(          :            YP       :          )~",
    R"~(            :          '      :            )~",
    R"~(               :            :              )~",
    R"~(                 '-.___.-'                 )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 2: locked
    R"~(            .                 .            )~",
    R"~(         .       :  : :  :       .         )~",
    R"~(              :             :              )~",
    R"~(      .     :                 :     .      )~",
    R"~(     .    :                     :    .     )~",
    R"~(         :          .d$$b.       :         )~",
    R"~(   .             .d$$$$$$b.            .   )~",
    R"~(        :        $$$'  '$$$       :        )~",
    R"~(   .    :        $$$ () $$$       :    .   )~",
    R"~(        :        '$$$..$$$'       :        )~",
    R"~(   .               '$$$$$'             .   )~",
    R"~(         :           'Y$P'       :         )~",
    R"~(     .    :            YP       :    .     )~",
    R"~(      .     :          '      :     .      )~",
    R"~(               :            :              )~",
    R"~(         .       '-.___.-'       .         )~",
    R"~(            .                 .            )~",
    R"~(               .   .   .   .               )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  },
  { // FRAME 3: glitch
    R"~(            %                 %            )~",
    R"~(         %       %  % %  %       %         )~",
    R"~(              %             %              )~",
    R"~(      %     %                 %     %      )~",
    R"~(     %    %                     %    %     )~",
    R"~(         %          .d%%b.       %         )~",
    R"~(   %             .d$%$%%$b.            %   )~",
    R"~(        %        %%%'  '%%$       %        )~",
    R"~(   %    %        %%$ () $$%       %    %   )~",
    R"~(        %        '$%$..%%%'       %        )~",
    R"~(   %               '%$%$$'             %   )~",
    R"~(         %           'Y%P'       %         )~",
    R"~(     %    %            YP       %    %     )~",
    R"~(      %     %          '      %     %      )~",
    R"~(               %            %              )~",
    R"~(         %       '-.___.-'       %         )~",
    R"~(            %                 %            )~",
    R"~(               %   %   %   %               )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~",
    R"~(                                           )~"
  }
};
