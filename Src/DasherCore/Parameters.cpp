#include "Parameters.h"

#include "DasherTypes.h"

namespace Dasher{
  namespace Settings {

	const std::unordered_map<Parameter, const Parameter_Value> parameter_defaults = {
        {BP_DRAW_MOUSE_LINE       , Parameter_Value{"DrawMouseLine"        , PARAM_BOOL, Persistence::PERSISTENT, true , "Draw Mouse Line"}},
		{BP_DRAW_MOUSE            , Parameter_Value{"DrawMouse"            , PARAM_BOOL, Persistence::PERSISTENT, true , "Draw Mouse Position"}},
		{BP_CURVE_MOUSE_LINE      , Parameter_Value{"CurveMouseLine"       , PARAM_BOOL, Persistence::PERSISTENT, false, "Curve mouse line according to screen nonlinearity"}},
		{BP_START_MOUSE           , Parameter_Value{"StartOnLeft"          , PARAM_BOOL, Persistence::PERSISTENT, true , "StartOnLeft"}},
		{BP_START_SPACE           , Parameter_Value{"StartOnSpace"         , PARAM_BOOL, Persistence::PERSISTENT, false, "StartOnSpace"}},
		{BP_MOUSEPOS_MODE         , Parameter_Value{"StartOnMousePosition" , PARAM_BOOL, Persistence::PERSISTENT, false, "StartOnMousePosition"}},
		{BP_PALETTE_CHANGE        , Parameter_Value{"PaletteChange"        , PARAM_BOOL, Persistence::PERSISTENT, false , "Switch from color palette in setting automatically to the one provided in the alphabet"}},
		{BP_TURBO_MODE            , Parameter_Value{"TurboMode"            , PARAM_BOOL, Persistence::PERSISTENT, true , "Boost speed when holding key1 or right mouse button"}},
		{BP_SMOOTH_PRESS_MODE     , Parameter_Value{"SmoothPressMode"      , PARAM_BOOL, Persistence::PERSISTENT, true , "Use Press-Input in the Smoothing-Input-Filter"}},
		{BP_SMOOTH_DRAW_MOUSE     , Parameter_Value{"SmoothDrawMouse"      , PARAM_BOOL, Persistence::PERSISTENT, false, "Draw an additional mouse cursor for the actual mouse position."}},
		{BP_SMOOTH_DRAW_MOUSE_LINE, Parameter_Value{"SmoothDrawMouseLine"  , PARAM_BOOL, Persistence::PERSISTENT, false, "Draw an additional mouse cursor line for the actual mouse position."}},
		{BP_SMOOTH_ONLY_FORWARD   , Parameter_Value{"SmoothOnlyForward"    , PARAM_BOOL, Persistence::PERSISTENT, true , "In the Smoothing-Input-Filter, only apply smoothing if the cursor is moving forward"}},
		{BP_EXACT_DYNAMICS        , Parameter_Value{"ExactDynamics"        , PARAM_BOOL, Persistence::PERSISTENT, false, "Use exact computation of per-frame movement (slower)"}},
		{BP_AUTOCALIBRATE         , Parameter_Value{"Autocalibrate"        , PARAM_BOOL, Persistence::PERSISTENT, false, "Automatically learn TargetOffset e.g. gazetracking"}},
		{BP_REMAP_XTREME          , Parameter_Value{"RemapXtreme"          , PARAM_BOOL, Persistence::PERSISTENT, false, "Pointer at extreme Y translates more and zooms less"}},
		{BP_AUTO_SPEEDCONTROL     , Parameter_Value{"AutoSpeedControl"     , PARAM_BOOL, Persistence::PERSISTENT, true , "AutoSpeedControl"}},
		{BP_LM_ADAPTIVE           , Parameter_Value{"LMAdaptive"           , PARAM_BOOL, Persistence::PERSISTENT, true , "Whether language model should learn as you enter text"}},
		{BP_SOCKET_DEBUG          , Parameter_Value{"SocketInputDebug"     , PARAM_BOOL, Persistence::PERSISTENT, false, "Print information about socket input processing to console"}},
		{BP_CIRCLE_START          , Parameter_Value{"CircleStart"          , PARAM_BOOL, Persistence::PERSISTENT, false, "Start on circle mode"}},
		{BP_GLOBAL_KEYBOARD       , Parameter_Value{"GlobalKeyboard"       , PARAM_BOOL, Persistence::PERSISTENT, false, "Whether to assume global control of the keyboard"}},
		{BP_NONLINEAR_Y           , Parameter_Value{"NonlinearY"           , PARAM_BOOL, Persistence::PERSISTENT, true , "Apply nonlinearities to Y axis (i.e. compress top &amp; bottom)"}},
		{BP_STOP_OUTSIDE          , Parameter_Value{"PauseOutside"         , PARAM_BOOL, Persistence::PERSISTENT, false, "Whether to stop when pointer leaves canvas area"}},
#ifdef TARGET_OS_IPHONE                              
		{BP_BACKOFF_BUTTON       , Parameter_Value{"BackoffButton"        , PARAM_BOOL, Persistence::PERSISTENT, false, "Whether to enable the extra backoff button in dynamic mode"}},
#else                                         
		{BP_BACKOFF_BUTTON        , Parameter_Value{"BackoffButton"        , PARAM_BOOL, Persistence::PERSISTENT, true , "Whether to enable the extra backoff button in dynamic mode"}},
#endif                                          
		{BP_TWOBUTTON_REVERSE     , Parameter_Value{"TwoButtonReverse"     , PARAM_BOOL, Persistence::PERSISTENT, false, "Reverse the up/down buttons in two button mode"}},
		{BP_2B_INVERT_DOUBLE      , Parameter_Value{"TwoButtonInvertDouble", PARAM_BOOL, Persistence::PERSISTENT, false, "Double-press acts as opposite button in two-button mode"}},
		{BP_SLOW_START            , Parameter_Value{"SlowStart"            , PARAM_BOOL, Persistence::PERSISTENT, false, "Start at low speed and increase"}},
		{BP_COPY_ALL_ON_STOP      , Parameter_Value{"CopyOnStop"           , PARAM_BOOL, Persistence::PERSISTENT, false, "Copy all text to clipboard whenever we stop"}},
		{BP_SPEAK_ALL_ON_STOP     , Parameter_Value{"SpeakOnStop"          , PARAM_BOOL, Persistence::PERSISTENT, false, "Speak all text whenever we stop"}},
		{BP_SPEAK_WORDS           , Parameter_Value{"SpeakWords"           , PARAM_BOOL, Persistence::PERSISTENT, false, "Speak words as they are written"}},
		{BP_GAME_HELP_DRAW_PATH   , Parameter_Value{"GameDrawPath"         , PARAM_BOOL, Persistence::PERSISTENT, true , "When we give help, show the shortest path to the target sentence"}},
		{BP_TWO_PUSH_RELEASE_TIME , Parameter_Value{"TwoPushReleaseTime"   , PARAM_BOOL, Persistence::PERSISTENT, false, "Use push and release times of single press rather than push times of two presses"}},
		{BP_SLOW_CONTROL_BOX      , Parameter_Value{"SlowControlBox"       , PARAM_BOOL, Persistence::PERSISTENT, true , "Slow down when going through control box" }},
		{BP_SIMULATE_TRANSPARENCY , Parameter_Value{"SimulateTransparency" , PARAM_BOOL, Persistence::PERSISTENT, false, "Enable the internal color mixing and thus the need to support alpha blending in the renderer." }},
									 
		{LP_ORIENTATION           , Parameter_Value{ "ScreenOrientation"         , PARAM_LONG, Persistence::PERSISTENT, -2l  , "Screen Orientation"}},
		{LP_MAX_BITRATE           , Parameter_Value{ "MaxBitRateTimes100"        , PARAM_LONG, Persistence::PERSISTENT, 80l  , "Max Bit Rate Times 100"}},
		{LP_FRAMERATE             , Parameter_Value{ "FrameRate"                 , PARAM_LONG, Persistence::EPHEMERAL , 3200l , "Decaying average of last known frame rates, *100"}},
		{LP_LANGUAGE_MODEL_ID     , Parameter_Value{ "LanguageModelID"           , PARAM_LONG, Persistence::PERSISTENT, 0l    , "LanguageModelID"}},
		{LP_DASHER_FONTSIZE       , Parameter_Value{ "DasherFontSize"            , PARAM_LONG, Persistence::PERSISTENT, 22l    , "Font size reached at crosshair (in points)"}},
		{LP_MESSAGE_FONTSIZE      , Parameter_Value{ "MessageFontSize"           , PARAM_LONG, Persistence::PERSISTENT, 14l   , "Size of font for messages (in points)"}},
		{LP_SHAPE_TYPE            , Parameter_Value{ "RenderStyle"               , PARAM_LONG, Persistence::PERSISTENT, static_cast<long>(Options::OVERLAPPING_RECTANGLE), "Shapes to render in (see Options::Rendering_Shape_Types)"}},
		{LP_UNIFORM               , Parameter_Value{ "UniformTimes1000"          , PARAM_LONG, Persistence::PERSISTENT, 50l   , "UniformTimes1000"}},
		{LP_YSCALE                , Parameter_Value{ "YScaling"                  , PARAM_LONG, Persistence::PERSISTENT, 0l    , "YScaling"}},
		{LP_MOUSEPOSDIST          , Parameter_Value{ "MousePositionBoxDistance"  , PARAM_LONG, Persistence::PERSISTENT, 50l   , "MousePositionBoxDistance"}},
		{LP_PY_PROB_SORT_THRES    , Parameter_Value{ "PYProbabilitySortThreshold", PARAM_LONG, Persistence::PERSISTENT, 85l   , "Sort converted syms in descending probability order up to this percentage"}},
		{LP_MESSAGE_TIME          , Parameter_Value{ "MessageTime"               , PARAM_LONG, Persistence::PERSISTENT, 2500l , "Time for which non-modal messages are displayed, in ms"}},
		{LP_LM_MAX_ORDER          , Parameter_Value{ "LMMaxOrder"                , PARAM_LONG, Persistence::PERSISTENT, 5l    , "LMMaxOrder"}},
		{LP_LM_EXCLUSION          , Parameter_Value{ "LMExclusion"               , PARAM_LONG, Persistence::PERSISTENT, 0l    , "LMExclusion" }},
		{LP_LM_UPDATE_EXCLUSION   , Parameter_Value{ "LMUpdateExclusion"         , PARAM_LONG, Persistence::PERSISTENT, 1l    , "LMUpdateExclusion"}},
		{LP_LM_ALPHA              , Parameter_Value{ "LMAlpha"                   , PARAM_LONG, Persistence::PERSISTENT, 49l   , "LMAlpha"}},
		{LP_LM_BETA               , Parameter_Value{ "LMBeta"                    , PARAM_LONG, Persistence::PERSISTENT, 77l   , "LMBeta"}},
		{LP_LM_MIXTURE            , Parameter_Value{ "LMMixture"                 , PARAM_LONG, Persistence::PERSISTENT, 50l   , "LMMixture"}},
		{LP_LINE_WIDTH            , Parameter_Value{ "LineWidth"                 , PARAM_LONG, Persistence::PERSISTENT, 1l    , "Width to draw crosshair and mouse line"}},
		{LP_GEOMETRY              , Parameter_Value{ "Geometry"                  , PARAM_LONG, Persistence::PERSISTENT, static_cast<long>(Options::ScreenGeometry::old_style)    , "Screen geometry (mostly for tall thin screens) - see Options::ScreenGeometry"}},
		{LP_LM_WORD_ALPHA         , Parameter_Value{ "WordAlpha"                 , PARAM_LONG, Persistence::PERSISTENT, 50l   , "Alpha value for word-based model"}},
		{LP_USER_LOG_LEVEL_MASK   , Parameter_Value{ "UserLogLevelMask"          , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Controls level of user logging, 0 = none, 1 = short, 2 = detailed, 3 = both"}},
		{LP_ZOOMSTEPS             , Parameter_Value{ "Zoomsteps"                 , PARAM_LONG, Persistence::PERSISTENT, 32l   , "Integerised ratio of zoom size for click/button mode, denom 64."}},
		{LP_B                     , Parameter_Value{ "ButtonMenuBoxes"           , PARAM_LONG, Persistence::PERSISTENT, 4l    , "Number of boxes for button menu mode"}},
		{LP_S                     , Parameter_Value{ "ButtonMenuSafety"          , PARAM_LONG, Persistence::PERSISTENT, 25l   , "Safety parameter for button mode, in percent."}},
#ifdef TARGET_OS_IPHONE           	 
		{LP_BUTTON_SCAN_TIME     , P arameter_Value{ "ButtonMenuScanTime"        , PARAM_LONG, Persistence::PERSISTENT, 600l  , "Scanning time in menu mode (0 = don't scan), in ms"}},
#else                              
		{LP_BUTTON_SCAN_TIME      , Parameter_Value{ "ButtonMenuScanTime"        , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Scanning time in menu mode (0 = don't scan), in ms"}},
#endif                             
		{LP_R                     , Parameter_Value{ "ButtonModeNonuniformity"   , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Button mode box non-uniformity"}},
		{LP_RIGHTZOOM             , Parameter_Value{ "ButtonCompassModeRightZoom", PARAM_LONG, Persistence::PERSISTENT, 5120l , "Zoomfactor (*1024) for compass mode"}},
#ifdef TARGET_OS_IPHONE           	 
		{LP_NODE_BUDGET          , P arameter_Value{ "NodeBudget"                , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Target (min) number of node objects to maintain"}},
#else                              
		{LP_NODE_BUDGET           , Parameter_Value{ "NodeBudget"                , PARAM_LONG, Persistence::PERSISTENT, 3000l , "Target (min) number of node objects to maintain"}},
#endif                             
		{LP_OUTLINE_WIDTH         , Parameter_Value{ "OutlineWidth"              , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Absolute value is line width to draw boxes (fill iff >=0)" }},
		{LP_TEXT_PADDING          , Parameter_Value{ "TextPadding"               , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Pixel distance to inset the letters into the boxes" }},
		{LP_MIN_NODE_SIZE         , Parameter_Value{ "MinNodeSize"               , PARAM_LONG, Persistence::PERSISTENT, 50l   , "Minimum size of node (in dasher coords) to draw" }}, 
		{LP_NONLINEAR_X           , Parameter_Value{ "NonLinearX"                , PARAM_LONG, Persistence::PERSISTENT, 5l    , "Nonlinear compression of X-axis (0 = none, higher = more extreme)"}},
		{LP_AUTOSPEED_SENSITIVITY , Parameter_Value{ "AutospeedSensitivity"      , PARAM_LONG, Persistence::PERSISTENT, 100l  , "Sensitivity of automatic speed control (percent)"}},
		{LP_SOCKET_PORT           , Parameter_Value{ "SocketPort"                , PARAM_LONG, Persistence::PERSISTENT, 20320l, "UDP/TCP socket to use for network socket input"}},
		{LP_SOCKET_INPUT_X_MIN    , Parameter_Value{ "SocketInputXMinTimes1000"  , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Bottom of range of X values expected from network input"}},
		{LP_SOCKET_INPUT_X_MAX    , Parameter_Value{ "SocketInputXMaxTimes1000"  , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Top of range of X values expected from network input"}},
		{LP_SOCKET_INPUT_Y_MIN    , Parameter_Value{ "SocketInputYMinTimes1000"  , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Bottom of range of Y values expected from network input"}},
		{LP_SOCKET_INPUT_Y_MAX    , Parameter_Value{ "SocketInputYMaxTimes1000"  , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Top of range of Y values expected from network input"}},
		{LP_CIRCLE_PERCENT        , Parameter_Value{ "CirclePercent"             , PARAM_LONG, Persistence::PERSISTENT, 10l   , "Percentage of nominal vertical range to use for radius of start circle"}},
		{LP_TWO_BUTTON_OFFSET     , Parameter_Value{ "TwoButtonOffset"           , PARAM_LONG, Persistence::PERSISTENT, 1638l , "Offset for two button dynamic mode"}},
		{LP_HOLD_TIME             , Parameter_Value{ "HoldTime"                  , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Time for which buttons must be held to count as long presses, in ms"}},
		{LP_MULTIPRESS_TIME       , Parameter_Value{ "MultipressTime"            , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Time in which multiple presses must occur, in ms"}},
		{LP_SLOW_START_TIME       , Parameter_Value{ "SlowStartTime"             , PARAM_LONG, Persistence::PERSISTENT, 1000l , "Time over which slow start occurs"}},
		{LP_SMOOTH_TAU            , Parameter_Value{ "SmoothTau"                 , PARAM_LONG, Persistence::PERSISTENT, 250l ,  "Factor Tau, that is used in smoothing input mode for exponential smoothing. Greater Zero, more smoothing the greater."}},
		{LP_TWO_PUSH_OUTER        , Parameter_Value{ "TwoPushOuter"              , PARAM_LONG, Persistence::PERSISTENT, 1792l , "Offset for one button dynamic mode outer marker"}},
		{LP_TWO_PUSH_LONG         , Parameter_Value{ "TwoPushLong"               , PARAM_LONG, Persistence::PERSISTENT, 512l  , "Distance between down markers (long gap)"}},
		{LP_TWO_PUSH_SHORT        , Parameter_Value{ "TwoPushShort"              , PARAM_LONG, Persistence::PERSISTENT, 80l   , "Distance between up markers, as percentage of long gap"}},
		{LP_TWO_PUSH_TOLERANCE    , Parameter_Value{ "TwoPushTolerance"          , PARAM_LONG, Persistence::PERSISTENT, 100l  , "Tolerance of two-push-mode pushes, in ms"}},
		{LP_DYNAMIC_BUTTON_LAG    , Parameter_Value{ "DynamicButtonLag"          , PARAM_LONG, Persistence::PERSISTENT, 50l   , "Lag of pushes in dynamic button mode (ms)"}},
		{LP_STATIC1B_TIME         , Parameter_Value{ "Static1BTime"              , PARAM_LONG, Persistence::PERSISTENT, 2000l , "Time for static-1B mode to scan from top to bottom (ms)"}},
		{LP_STATIC1B_ZOOM         , Parameter_Value{ "Static1BZoom"              , PARAM_LONG, Persistence::PERSISTENT, 8l    , "Zoom factor for static-1B mode"}},
		{LP_DEMO_SPRING           , Parameter_Value{ "DemoSpring"                , PARAM_LONG, Persistence::PERSISTENT, 100l  , "Springyness in Demo-mode"}},
		{LP_DEMO_NOISE_MEM        , Parameter_Value{ "DemoNoiseMem"              , PARAM_LONG, Persistence::PERSISTENT, 100l  , "Memory parameter for noise in Demo-mode"}},
		{LP_DEMO_NOISE_MAG        , Parameter_Value{ "DemoNoiseMag"              , PARAM_LONG, Persistence::PERSISTENT, 325l  , "Magnitude of noise in Demo-mode"}},
		{LP_MAXZOOM               , Parameter_Value{ "ClickMaxZoom"              , PARAM_LONG, Persistence::PERSISTENT, 200l  , "Maximum zoom possible in click mode (times 10)"}},
		{LP_DYNAMIC_SPEED_INC     , Parameter_Value{ "DynamicSpeedInc"           , PARAM_LONG, Persistence::PERSISTENT, 3l    , "%age by which dynamic mode auto speed control increases speed"}},
		{LP_DYNAMIC_SPEED_FREQ    , Parameter_Value{ "DynamicSpeedFreq"          , PARAM_LONG, Persistence::PERSISTENT, 10l   , "Seconds after which dynamic mode auto speed control increases speed"}},
		{LP_DYNAMIC_SPEED_DEC     , Parameter_Value{ "DynamicSpeedDec"           , PARAM_LONG, Persistence::PERSISTENT, 8l    , "%age by which dynamic mode auto speed control decreases speed on reverse"}},
		{LP_TAP_TIME              , Parameter_Value{ "TapTime"                   , PARAM_LONG, Persistence::PERSISTENT, 200l  , "Max length of a stylus 'tap' rather than hold (ms)"}},
#ifdef TARGET_OS_IPHONE           	 
		{LP_MARGIN_WIDTH         , P arameter_Value{ "MarginWidth"               , PARAM_LONG, Persistence::PERSISTENT, 500l  , "Width of RHS margin (in Dasher co-ords)"}},
#else                              
		{LP_MARGIN_WIDTH          , Parameter_Value{ "MarginWidth"               , PARAM_LONG, Persistence::PERSISTENT, 300l  , "Width of RHS margin (in Dasher co-ords)"}},
#endif                             
		{LP_TARGET_OFFSET         , Parameter_Value{ "TargetOffset"              , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Vertical distance between mouse pointer and target (400=screen height)"}},
		{LP_X_LIMIT_SPEED         , Parameter_Value{ "XLimitSpeed"               , PARAM_LONG, Persistence::PERSISTENT, 800l  , "X Co-ordinate at which maximum speed is reached (&lt;2048=xhair)"}},
		{LP_GAME_HELP_DIST        , Parameter_Value{ "GameHelpDistance"          , PARAM_LONG, Persistence::PERSISTENT, 1920l , "Distance of sentence from center to decide user needs help"}},
		{LP_GAME_HELP_TIME        , Parameter_Value{ "GameHelpTime"              , PARAM_LONG, Persistence::PERSISTENT, 0l    , "Time for which user must need help before help drawn"}},
								
								
		{SP_ALPHABET_ID          , Parameter_Value{ "AlphabetID"       , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "AlphabetID"}},
		{SP_ALPHABET_1           , Parameter_Value{ "Alphabet1"        , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Alphabet History 1"}},
		{SP_ALPHABET_2           , Parameter_Value{ "Alphabet2"        , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Alphabet History 2"}},
		{SP_ALPHABET_3           , Parameter_Value{ "Alphabet3"        , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Alphabet History 3"}},
		{SP_ALPHABET_4           , Parameter_Value{ "Alphabet4"        , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Alphabet History 4"}},
		{ SP_COLOUR_ID           , Parameter_Value{ "ColourID"         , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "ColourID" }},
		{SP_DASHER_FONT          , Parameter_Value{ "DasherFont"       , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "DasherFont"}},
		{SP_GAME_TEXT_FILE       , Parameter_Value{ "GameTextFile"     , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "User-specified file with strings to practice writing"}},
		{SP_SOCKET_INPUT_X_LABEL , Parameter_Value{ "SocketInputXLabel", PARAM_STRING, Persistence::PERSISTENT, std::string("x")             , "Label preceding X values for network input"}},
		{SP_SOCKET_INPUT_Y_LABEL , Parameter_Value{ "SocketInputYLabel", PARAM_STRING, Persistence::PERSISTENT, std::string("y")             , "Label preceding Y values for network input"}},
#ifdef TARGET_OS_IPHONE           
		{SP_INPUT_FILTER        , Parameter_Value{ "InputFilter"      , PARAM_STRING, Persistence::PERSISTENT, std::string("Stylus Control"), "Input filter used to provide the current control mode"}},
#else                             
		{SP_INPUT_FILTER         , Parameter_Value{ "InputFilter"      , PARAM_STRING, Persistence::PERSISTENT, std::string("Normal Control"), "Input filter used to provide the current control mode"}},
#endif                            
		{SP_INPUT_DEVICE         , Parameter_Value{ "InputDevice"      , PARAM_STRING, Persistence::PERSISTENT, std::string("Mouse Input")   , "Driver for the input device"}},
		{SP_BUTTON_0             , Parameter_Value{ "Button0"          , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 0"}},
		{SP_BUTTON_1             , Parameter_Value{ "Button1"          , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 1"}},
		{SP_BUTTON_2             , Parameter_Value{ "Button2"          , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 2"}},
		{SP_BUTTON_3             , Parameter_Value{ "Button3"          , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 3"}},
		{SP_BUTTON_4             , Parameter_Value{ "Button4"          , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 4"}},
		{SP_BUTTON_10            , Parameter_Value{ "Button10"         , PARAM_STRING, Persistence::PERSISTENT, std::string("")              , "Assignment to button 10"}},
		{SP_JOYSTICK_DEVICE      , Parameter_Value{ "JoystickDevice"   , PARAM_STRING, Persistence::PERSISTENT, std::string("/dev/input/js0"), "Joystick device"}}
	};

	ParameterType GetParameterType(Parameter parameter) {
		if (parameter_defaults.find(parameter) != parameter_defaults.end())
		{
			return parameter_defaults.at(parameter).type;
		}
		return PARAM_INVALID;
	}

	std::pair<Parameter, ParameterType> GetParameter(const std::string& parameterName) {

		for(auto& [key, value] : parameter_defaults)
		{
			if(value.name == parameterName) return {key, value.type};
		}
		return {PM_INVALID, PARAM_INVALID};
	}

	std::string GetParameterName(Parameter parameter) {
		if (parameter_defaults.find(parameter) != parameter_defaults.end())
		{
			return parameter_defaults.at(parameter).name;
		}
		return "";
	}

} //end namespace Settings
} //end namespace Dasher
