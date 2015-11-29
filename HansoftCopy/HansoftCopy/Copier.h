//
//  Copier.h
//  HansoftCopy
//
//  Created by Patrik Hartlén on 28/11/15.
//  Copyright © 2015 Patrik Hartlén. All rights reserved.
//

#ifndef Copier_h
#define Copier_h

#include <stdio.h>
#include <string>

using std::string;

void Copy(string server, int port, string database, string username, string password, string project, string source, string destination);

#endif /* Copier_h */
