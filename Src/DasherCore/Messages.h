// Messages.h
//
// Created 2011 by Alan Lawrence

#pragma once

///Abstract superclass = interface for displaying messages to the user.
///Each platform must implement: see CDasherInterfaceBase, CDashIntfScreenMsgs

#include <string>

/// \ingroup Core
/// @{

class CMessageDisplay {
public:
  ///Displays a message to the user - somehow. Two styles
  /// of message are supported: (1) modal messages, i.e. which interrupt text entry;
  /// these should be explicitly dismissed (somehow) before text entry resumes; and
  /// (2) non-modal or asynchronous messages, which should be displayed in the background
  /// but allow the user to continue text entry as normal.
  /// NOTE for subclasses: it is best not to popup any modal window here but rather to
  /// store all messages until the next frame is rendered and then combine them into one.
  /// \param strText text of message to display.
  /// \param bInterrupt if true, text entry should be interrupted; if false, user should
  /// be able to continue writing uninterrupted.
  virtual void Message(const std::string &strText, bool bInterrupt)=0;
  
  /// Utility method for common case of displaying a modal message with a format
  void FormatMessage(const char* format, va_list args);
  void FormatMessage(const char* format, ...);
};

/// @}