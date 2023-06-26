#pragma once

#include <string>
#include <unordered_map>
#include <variant>

// All parameters go into the enums here
// They are unique across the different types
namespace Dasher
{
	enum Parameter{ 
		BP_DRAW_MOUSE_LINE, BP_DRAW_MOUSE, BP_CURVE_MOUSE_LINE,
		BP_START_MOUSE,
		BP_START_SPACE, BP_CONTROL_MODE, 
		BP_MOUSEPOS_MODE,
		BP_PALETTE_CHANGE, BP_TURBO_MODE, BP_EXACT_DYNAMICS,
		BP_AUTOCALIBRATE, BP_REMAP_XTREME,
		BP_AUTO_SPEEDCONTROL,
		BP_LM_ADAPTIVE, BP_SOCKET_DEBUG, 
		BP_CIRCLE_START, BP_GLOBAL_KEYBOARD, BP_NONLINEAR_Y,
		BP_STOP_OUTSIDE, BP_BACKOFF_BUTTON,
		BP_TWOBUTTON_REVERSE, BP_2B_INVERT_DOUBLE, BP_SLOW_START,
		BP_COPY_ALL_ON_STOP, BP_SPEAK_ALL_ON_STOP, BP_SPEAK_WORDS,
		BP_GAME_HELP_DRAW_PATH, BP_TWO_PUSH_RELEASE_TIME,
		BP_SLOW_CONTROL_BOX,
		END_OF_BPS,

		LP_ORIENTATION, LP_MAX_BITRATE, LP_FRAMERATE,
		LP_LANGUAGE_MODEL_ID, LP_DASHER_FONTSIZE, LP_MESSAGE_FONTSIZE, LP_SHAPE_TYPE,
		LP_UNIFORM, LP_YSCALE, LP_MOUSEPOSDIST, LP_PY_PROB_SORT_THRES, LP_MESSAGE_TIME,
		LP_LM_MAX_ORDER, LP_LM_EXCLUSION,
		LP_LM_UPDATE_EXCLUSION, LP_LM_ALPHA, LP_LM_BETA,
		LP_LM_MIXTURE, LP_LINE_WIDTH, LP_GEOMETRY,
		LP_LM_WORD_ALPHA, LP_USER_LOG_LEVEL_MASK, 
		LP_ZOOMSTEPS, LP_B, LP_S, LP_BUTTON_SCAN_TIME, LP_R, LP_RIGHTZOOM,
		LP_NODE_BUDGET, LP_OUTLINE_WIDTH, LP_MIN_NODE_SIZE, LP_NONLINEAR_X,
		LP_AUTOSPEED_SENSITIVITY, LP_SOCKET_PORT, LP_SOCKET_INPUT_X_MIN, LP_SOCKET_INPUT_X_MAX,
		LP_SOCKET_INPUT_Y_MIN, LP_SOCKET_INPUT_Y_MAX,
		LP_CIRCLE_PERCENT, LP_TWO_BUTTON_OFFSET, LP_HOLD_TIME, LP_MULTIPRESS_TIME,
		LP_SLOW_START_TIME,
		LP_TWO_PUSH_OUTER, LP_TWO_PUSH_LONG, LP_TWO_PUSH_SHORT, LP_TWO_PUSH_TOLERANCE,
		LP_DYNAMIC_BUTTON_LAG, LP_STATIC1B_TIME, LP_STATIC1B_ZOOM,
		LP_DEMO_SPRING, LP_DEMO_NOISE_MEM, LP_DEMO_NOISE_MAG, LP_MAXZOOM, 
		LP_DYNAMIC_SPEED_INC, LP_DYNAMIC_SPEED_FREQ, LP_DYNAMIC_SPEED_DEC,
		LP_TAP_TIME, LP_MARGIN_WIDTH, LP_TARGET_OFFSET, LP_X_LIMIT_SPEED,
		LP_GAME_HELP_DIST, LP_GAME_HELP_TIME,
		END_OF_LPS,


		SP_ALPHABET_ID, SP_ALPHABET_1, SP_ALPHABET_2, SP_ALPHABET_3, SP_ALPHABET_4, 
		SP_COLOUR_ID, SP_CONTROL_BOX_ID, SP_DASHER_FONT, SP_GAME_TEXT_FILE,
		SP_SOCKET_INPUT_X_LABEL, SP_SOCKET_INPUT_Y_LABEL, SP_INPUT_FILTER, SP_INPUT_DEVICE,
		SP_BUTTON_0, SP_BUTTON_1, SP_BUTTON_2, SP_BUTTON_3, SP_BUTTON_4, SP_BUTTON_10, SP_JOYSTICK_DEVICE,
		END_OF_SPS
	};

	struct CParameterChange {
	    CParameterChange(Parameter parameter, bool value)
	        :iParameter(parameter), value(value) {}
	    CParameterChange(Parameter parameter, long value)
	        :iParameter(parameter), value(value) {}
	    CParameterChange(Parameter parameter, std::string value)
	        :iParameter(parameter),value(value){}
	    Parameter iParameter;
		std::variant<bool, long, std::string> value;
	};

  ///Namespace containing all static (i.e. fixed/constant) data about
  /// settings, that is _not_ dependent on the storage mechanism,
  /// the SettingsStore in use, or platform-specific details.
  /// (Except, some defaults are #ifdef'd according to platform).
  /// This data does NOT change at runtime.
  namespace Settings {
	enum class Persistence { PERSISTENT, EPHEMERAL };

  	// Types that are parameters can be
    enum ParameterType {
      ParamBool,
      ParamLong,
      ParamString,
      ParamInvalid
    };

    // Values
    struct Parameter_Value {
		std::string name;
		Settings::ParameterType type = Settings::ParamInvalid;
		Persistence persistence = Persistence::PERSISTENT;
		std::variant<bool, long, std::string> value;
        std::string human_readable;
	};

    extern const std::unordered_map<Parameter, const Parameter_Value> parameter_defaults;
    
    ///Get the type of a parameter by its key.
    /// \param iParameter one of the BP_*, LP_* or SP_* enum constants
    /// \return ParamBool, ParamLong or ParamString, respectively; or
    /// ParamInvalid if iParameter is not known (present in the parameter_defaults map
    ParameterType GetParameterType(Parameter iParameter);
    
    ///Gets the regName member of the struct for a parameter (of any of the 3 types).
    /// This is appropriate for use as a key for storing the setting value into e.g. a registry.
    /// Note - returns a string not a reference to one, because the table stores only a char*.
    /// \param iParameter one of the BP_*, LP_* or SP_* enum constants
    /// \return the regName member of the corresponding bp_table, lp_table,
    /// or sp_table struct.
    std::string GetParameterName(Parameter iParameter);
  }
}
