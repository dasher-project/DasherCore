cmake_minimum_required(VERSION 3.10)

project("DasherCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(HAVE_ROUND _CRT_SECURE_NO_WARNINGS)
set(CMAKE_SUPPRESS_REGENERATION true)

option(DASHER_HAVE_OWN_FILEUTILS "Set to true if you provide your own FileUtils implementation" OFF)
if(${DASHER_HAVE_OWN_FILEUTILS})
	add_compile_definitions(HAVE_OWN_FILEUTILS)
endif()

###############################
# Building pugixml Library
###############################

set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Thirdparty/pugixml" EXCLUDE_FROM_ALL)

###############################
# Building Core Dasher Library
###############################

file(GLOB_RECURSE HEADER_FILES
    ${CMAKE_CURRENT_LIST_DIR}/Src/*.h
)

set(SOURCE_FILES
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Actions.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/PressFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Parameters.cpp 
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SettingsStore.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/XmlSettingsStore.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/AbstractXMLParser.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Alphabet/AlphIO.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Alphabet/AlphInfo.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Alphabet/AlphabetMap.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/AlternatingDirectMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/AlphabetManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/AutoSpeedControl.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/BasicLog.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/FileLogger.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/FileUtils.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ButtonMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ButtonMultiPress.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/CircleStartHandler.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ClickFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ColorIO.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ColorPalette.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/CompassMode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ConversionManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ConvertingAlphMgr.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherButtons.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherInterfaceBase.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DashIntfSettings.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DashIntfScreenMsgs.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherModule.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherNode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherView.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DasherViewSquare.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DefaultFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DemoFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DynamicButtons.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/DynamicFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/FileWordGenerator.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/FrameRate.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/GameModule.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/MandarinAlphMgr.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Messages.cpp 
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ModuleManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/NodeCreationManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ExpansionPolicy.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/OneButtonDynamicFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/OneButtonFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/OneDimensionalFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/PressFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SmoothingFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/RoutingAlphMgr.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SCENode.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/ScreenGameModule.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SimpleTimer.cpp
	# ${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SocketInput.cpp
	# ${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/SocketInputBase.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/StylusFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/TimeSpan.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Trainer.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/TwoBoxStartHandler.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/TwoButtonDynamicFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/TwoPushDynamicFilter.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserButton.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserLocation.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserLog.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserLogBase.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserLogParam.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/UserLogTrial.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/WordGeneratorBase.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/XMLUtil.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/CTWLanguageModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/DictLanguageModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/HashTable.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/PPMLanguageModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/PPMPYLanguageModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/RoutingPPMLanguageModel.cpp
	${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/WordLanguageModel.cpp
)

ADD_library(DasherCore STATIC
	${HEADER_FILES}
	${SOURCE_FILES}
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/Src/ FILES ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/Alphabet/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/DasherCore/LanguageModelling/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/Common/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/Common/Allocators/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/Common/Platform/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/Common/Types/)
target_include_directories(DasherCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Src/Common/Unicode/)

add_dependencies(DasherCore pugixml)
target_link_libraries(DasherCore pugixml)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DasherCore)
