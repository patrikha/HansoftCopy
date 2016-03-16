//
//  main.cpp
//  HansoftCopy
//
//  Created by Patrik Hartlén on 2015-11-28.
//  Copyright (c) 2015 Patrik Hartlén. All rights reserved.
//  License MIT-style license
//

#include "OptionParser.h"
#include "Copier.h"
#include <iostream>
#include <sstream>

using std::string;
using std::vector;
using std::stringstream;

int main(int argc, const char * argv[]) {
    optparse::OptionParser parser = optparse::OptionParser().description("Hansoft copy column data utility");
    parser.add_option("-s", "--server").dest("server").help("Hansoft server address").set_default("localhost").metavar("SERVER");
    parser.add_option("-o", "--port").dest("port").help("Hansoft server port").set_default("50256").metavar("PORT");
    parser.add_option("-d", "--database").dest("database").help("Hansoft database").metavar("DATABASE");
    parser.add_option("-u", "--username").dest("username").help("Hansoft SDK username").metavar("USERNAME");
    parser.add_option("-p", "--password").dest("password").help("Hansoft SDK password").metavar("PASSWORD");
    parser.add_option("-r", "--project").dest("project").help("Hansoft project").metavar("PROJECT");
    parser.add_option("-f", "--source").dest("source").help("Hansoft source custom column or built in [priority, ]").metavar("SOURCE");
    parser.add_option("-t", "--destination").dest("destination").help("Hansoft destination custom column").metavar("DESTINATION");

    optparse::Values options = parser.parse_args(argc, argv);
    vector<string> args = parser.args();
    
    string server = string(options.get("server"));
    int port = atoi(options.get("port"));
    string database = string(options.get("database"));
    string username = string(options.get("username"));
    string password = string(options.get("password"));
    string project = string(options.get("project"));
    string source = string(options.get("source"));
    string destination = string(options.get("destination"));
 
    std::cout << "Copying column data" << std::endl;
    
    Copy(server, port, database, username, password, project, source, destination);
    
    return 0;
}
