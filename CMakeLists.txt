cmake_minimum_required(VERSION 3.10)

project("DasherCore")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(HAVE_ROUND XML_STATIC _CRT_SECURE_NO_WARNINGS)
set(CMAKE_SUPPRESS_REGENERATION true)

###############################
# Building Expat Library
###############################

set(EXPAT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EXPAT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(EXPAT_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(EXPAT_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(EXPAT_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("Thirdparty/libexpat/expat")

###############################
# Building Core Dasher Library
###############################

ADD_library(DasherCore STATIC
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Parameters.cpp 
	${PROJECT_SOURCE_DIR}/Src/DasherCore/SettingsStore.cpp 
	${PROJECT_SOURCE_DIR}/Src/DasherCore/XmlSettingsStore.h
	${PROJECT_SOURCE_DIR}/Src/DasherCore/XmlSettingsStore.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/AbstractXMLParser.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Alphabet/AlphIO.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Alphabet/AlphInfo.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Alphabet/AlphabetMap.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/AlternatingDirectMode.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/AlphabetManager.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/AutoSpeedControl.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/BasicLog.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/FileLogger.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ButtonMode.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ButtonMultiPress.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/CircleStartHandler.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ClickFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ColourIO.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/CompassMode.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ControlManager.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ConversionManager.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ConvertingAlphMgr.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherButtons.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherInterfaceBase.h
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherInterfaceBase.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DashIntfSettings.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DashIntfScreenMsgs.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherModule.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherNode.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherView.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherViewSquare.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DasherViewSquare.inl
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DefaultFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DemoFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DynamicButtons.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/DynamicFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/FileWordGenerator.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/FrameRate.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/GameModule.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/MandarinAlphMgr.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/MemoryLeak.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Messages.cpp 
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ModuleManager.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/NodeCreationManager.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ExpansionPolicy.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/OneButtonDynamicFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/OneButtonFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/OneDimensionalFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/RoutingAlphMgr.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/SCENode.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/ScreenGameModule.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/SimpleTimer.cpp
	# ${PROJECT_SOURCE_DIR}/Src/DasherCore/SocketInput.cpp
	# ${PROJECT_SOURCE_DIR}/Src/DasherCore/SocketInputBase.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/StylusFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/TimeSpan.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/Trainer.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/TwoBoxStartHandler.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/TwoButtonDynamicFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/TwoPushDynamicFilter.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserButton.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserLocation.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserLog.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserLogBase.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserLogParam.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/UserLogTrial.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/WordGeneratorBase.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/XMLUtil.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/CTWLanguageModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/DictLanguageModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/HashTable.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/PPMLanguageModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/PPMPYLanguageModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/RoutingPPMLanguageModel.cpp
	${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/WordLanguageModel.cpp  
)

target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/DasherCore/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/DasherCore/Alphabet/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/DasherCore/LanguageModelling/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Common/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Common/Allocators/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Common/Platform/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Common/Types/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Common/Unicode/)
target_include_directories(DasherCore PUBLIC ${PROJECT_SOURCE_DIR}/Src/Win32/Common/)

add_dependencies(DasherCore expat)
target_link_libraries(DasherCore expat)