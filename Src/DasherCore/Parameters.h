#pragma once

#include <string>
#include <unordered_map>
#include <variant>

// All parameters go into the enums here
// They are unique across the different types
namespace Dasher
{
    typedef const std::string_view Parameter;
    namespace Parameters
    {
        constexpr Parameter BP_DRAW_MOUSE_LINE = "DrawMouseLine";
        constexpr Parameter BP_DRAW_MOUSE = "DrawMouse";
        constexpr Parameter BP_CURVE_MOUSE_LINE = "CurveMouseLine";
        constexpr Parameter BP_START_MOUSE = "StartOnLeft";
        constexpr Parameter BP_START_SPACE = "StartOnSpace";
        constexpr Parameter BP_CONTROL_MODE = "ControlMode";
        constexpr Parameter BP_MOUSEPOS_MODE = "StartOnMousePosition";
        constexpr Parameter BP_PALETTE_CHANGE = "PaletteChange";
        constexpr Parameter BP_TURBO_MODE = "TurboMode";
        constexpr Parameter BP_EXACT_DYNAMICS = "ExactDynamics";
        constexpr Parameter BP_AUTOCALIBRATE = "Autocalibrate";
        constexpr Parameter BP_REMAP_XTREME = "RemapXtreme";
        constexpr Parameter BP_AUTO_SPEEDCONTROL = "AutoSpeedControl";
        constexpr Parameter BP_LM_ADAPTIVE = "LMAdaptive";
        constexpr Parameter BP_SOCKET_DEBUG = "SocketInputDebug";
        constexpr Parameter BP_CIRCLE_START = "CircleStart";
        constexpr Parameter BP_GLOBAL_KEYBOARD = "GlobalKeyboard";
        constexpr Parameter BP_NONLINEAR_Y = "NonlinearY";
        constexpr Parameter BP_STOP_OUTSIDE = "PauseOutside";
        constexpr Parameter BP_BACKOFF_BUTTON = "BackoffButton";
        constexpr Parameter BP_TWOBUTTON_REVERSE = "TwoButtonReverse";
        constexpr Parameter BP_2B_INVERT_DOUBLE = "TwoButtonInvertDouble";
        constexpr Parameter BP_SLOW_START = "SlowStart";
        constexpr Parameter BP_COPY_ALL_ON_STOP = "CopyOnStop";
        constexpr Parameter BP_SPEAK_ALL_ON_STOP = "SpeakOnStop";
        constexpr Parameter BP_SPEAK_WORDS = "SpeakWords";
        constexpr Parameter BP_GAME_HELP_DRAW_PATH = "GameDrawPath";
        constexpr Parameter BP_TWO_PUSH_RELEASE_TIME = "TwoPushReleaseTime";
        constexpr Parameter BP_SLOW_CONTROL_BOX = "SlowControlBox";

        constexpr Parameter LP_ORIENTATION = "ScreenOrientation";
        constexpr Parameter LP_MAX_BITRATE = "MaxBitRateTimes100";
        constexpr Parameter LP_FRAMERATE = "FrameRate";
        constexpr Parameter LP_LANGUAGE_MODEL_ID = "LanguageModelID";
        constexpr Parameter LP_DASHER_FONTSIZE = "DasherFontSize";
        constexpr Parameter LP_MESSAGE_FONTSIZE = "MessageFontSize";
        constexpr Parameter LP_SHAPE_TYPE = "RenderStyle";
        constexpr Parameter LP_UNIFORM = "UniformTimes1000";
        constexpr Parameter LP_YSCALE = "YScaling";
        constexpr Parameter LP_MOUSEPOSDIST = "MousePositionBoxDistance";
        constexpr Parameter LP_PY_PROB_SORT_THRES = "PYProbabilitySortThreshold";
        constexpr Parameter LP_MESSAGE_TIME = "MessageTime";
        constexpr Parameter LP_LM_MAX_ORDER = "LMMaxOrder";
        constexpr Parameter LP_LM_EXCLUSION = "LMExclusion";
        constexpr Parameter LP_LM_UPDATE_EXCLUSION = "LMUpdateExclusion";
        constexpr Parameter LP_LM_ALPHA = "LMAlpha";
        constexpr Parameter LP_LM_BETA = "LMBeta";
        constexpr Parameter LP_LM_MIXTURE = "LMMixture";
        constexpr Parameter LP_LINE_WIDTH = "LineWidth";
        constexpr Parameter LP_GEOMETRY = "Geometry";
        constexpr Parameter LP_LM_WORD_ALPHA = "WordAlpha";
        constexpr Parameter LP_USER_LOG_LEVEL_MASK = "UserLogLevelMask";
        constexpr Parameter LP_ZOOMSTEPS = "Zoomsteps";
        constexpr Parameter LP_B = "ButtonMenuBoxes";
        constexpr Parameter LP_S = "ButtonMenuSafety";
        constexpr Parameter LP_BUTTON_SCAN_TIME = "ButtonMenuScanTime";
        constexpr Parameter LP_R = "ButtonModeNonuniformity";
        constexpr Parameter LP_RIGHTZOOM = "ButtonCompassModeRightZoom";
        constexpr Parameter LP_NODE_BUDGET = "NodeBudget";
        constexpr Parameter LP_OUTLINE_WIDTH = "OutlineWidth";
        constexpr Parameter LP_MIN_NODE_SIZE = "MinNodeSize";
        constexpr Parameter LP_NONLINEAR_X = "NonLinearX";
        constexpr Parameter LP_AUTOSPEED_SENSITIVITY = "AutospeedSensitivity";
        constexpr Parameter LP_SOCKET_PORT = "SocketPort";
        constexpr Parameter LP_SOCKET_INPUT_X_MIN = "SocketInputXMinTimes1000";
        constexpr Parameter LP_SOCKET_INPUT_X_MAX = "SocketInputXMaxTimes1000";
        constexpr Parameter LP_SOCKET_INPUT_Y_MIN = "SocketInputYMinTimes1000";
        constexpr Parameter LP_SOCKET_INPUT_Y_MAX = "SocketInputYMaxTimes1000";
        constexpr Parameter LP_CIRCLE_PERCENT = "CirclePercent";
        constexpr Parameter LP_TWO_BUTTON_OFFSET = "TwoButtonOffset";
        constexpr Parameter LP_HOLD_TIME = "HoldTime";
        constexpr Parameter LP_MULTIPRESS_TIME = "MultipressTime";
        constexpr Parameter LP_SLOW_START_TIME = "SlowStartTime";
        constexpr Parameter LP_TWO_PUSH_OUTER = "TwoPushOuter";
        constexpr Parameter LP_TWO_PUSH_LONG = "TwoPushLong";
        constexpr Parameter LP_TWO_PUSH_SHORT = "TwoPushShort";
        constexpr Parameter LP_TWO_PUSH_TOLERANCE = "TwoPushTolerance";
        constexpr Parameter LP_DYNAMIC_BUTTON_LAG = "DynamicButtonLag";
        constexpr Parameter LP_STATIC1B_TIME = "Static1BTime";
        constexpr Parameter LP_STATIC1B_ZOOM = "Static1BZoom";
        constexpr Parameter LP_DEMO_SPRING = "DemoSpring";
        constexpr Parameter LP_DEMO_NOISE_MEM = "DemoNoiseMem";
        constexpr Parameter LP_DEMO_NOISE_MAG = "DemoNoiseMag";
        constexpr Parameter LP_MAXZOOM = "ClickMaxZoom";
        constexpr Parameter LP_DYNAMIC_SPEED_INC = "DynamicSpeedInc";
        constexpr Parameter LP_DYNAMIC_SPEED_FREQ = "DynamicSpeedFreq";
        constexpr Parameter LP_DYNAMIC_SPEED_DEC = "DynamicSpeedDec";
        constexpr Parameter LP_TAP_TIME = "TapTime";
        constexpr Parameter LP_MARGIN_WIDTH = "MarginWidth";
        constexpr Parameter LP_TARGET_OFFSET = "TargetOffset";
        constexpr Parameter LP_X_LIMIT_SPEED = "XLimitSpeed";
        constexpr Parameter LP_GAME_HELP_DIST = "GameHelpDistance";
        constexpr Parameter LP_GAME_HELP_TIME = "GameHelpTime";

        constexpr Parameter SP_ALPHABET_ID = "AlphabetID";
        constexpr Parameter SP_ALPHABET_1 = "Alphabet1";
        constexpr Parameter SP_ALPHABET_2 = "Alphabet2";
        constexpr Parameter SP_ALPHABET_3 = "Alphabet3";
        constexpr Parameter SP_ALPHABET_4 = "Alphabet4";
        constexpr Parameter SP_COLOUR_ID = "ColourID";
        constexpr Parameter SP_CONTROL_BOX_ID = "ControlBoxID";
        constexpr Parameter SP_DASHER_FONT = "DasherFont";
        constexpr Parameter SP_GAME_TEXT_FILE = "GameTextFile";
        constexpr Parameter SP_SOCKET_INPUT_X_LABEL = "SocketInputXLabel";
        constexpr Parameter SP_SOCKET_INPUT_Y_LABEL = "SocketInputYLabel";
        constexpr Parameter SP_INPUT_FILTER = "InputFilter";
        constexpr Parameter SP_INPUT_DEVICE = "InputDevice";
        constexpr Parameter SP_BUTTON_0 = "Button0";
        constexpr Parameter SP_BUTTON_1 = "Button1";
        constexpr Parameter SP_BUTTON_2 = "Button2";
        constexpr Parameter SP_BUTTON_3 = "Button3";
        constexpr Parameter SP_BUTTON_4 = "Button4";
        constexpr Parameter SP_BUTTON_10 = "Button10";
        constexpr Parameter SP_JOYSTICK_DEVICE = "JoystickDevice";
    }

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
  namespace Parameters {
	enum class Persistence { PERSISTENT, EPHEMERAL };

  	// Types that are parameters can be
    enum ParameterType {
      PARAM_BOOL,
      PARAM_LONG,
      PARAM_STRING,
      PARAM_INVALID
    };

    // Values
    struct Parameter_Value {
		ParameterType type = PARAM_INVALID;
		Persistence persistence = Persistence::PERSISTENT;
		std::variant<bool, long, std::string> value;
        std::string human_readable;
	};

    typedef std::unordered_map<std::string_view, Parameter_Value> ParameterMap; //Mutable type, used by classes to store and alter parameters internally
    typedef const std::unordered_map<std::string_view, const Parameter_Value> DefaultsParameterMap; //Immutable Type, used for hardcoded defaults
    extern DefaultsParameterMap parameter_defaults;
    
    ///Get the type of a parameter by its key.
    /// \param iParameter one of the BP_*, LP_* or SP_* enum constants
    /// \return PARAM_BOOL, PARAM_LONG or PARAM_STRING, respectively; or
    /// PARAM_INVALID if iParameter is not known (present in the parameter_defaults map
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
