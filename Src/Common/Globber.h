//
//  Globber.h
//  Dasher
//
//  Created by Alan Lawrence on 21/9/11.
//  Copyright 2011 Cambridge University. All rights reserved.
//

#pragma once

#include "../DasherCore/AbstractXMLParser.h"

void globScan(AbstractParser *parser,
              const char **userPaths,
              const char **systemPaths);
void globScan(AbstractParser *parser, 
              const char **usrPaths, 
              const char **sysPaths, 
              int(*error_callback)(const char *,int));

