# HansoftCopy

Command line utility for copying column data within a [Hansoft](http://hansoft.com) project. For a simple setup have a look at the [Hansoft docker](https://hub.docker.com/r/patrikha/hansoftserver/).

## Build
Download [HansoftSDK](http://www.hansoft.com/en/support/downloads) and unzip the content to the HansoftSDK folder.

## Usage
For help please use the build-in help command: HansoftCopy --help
```
Usage: HansoftCopy [options]

Hansoft copy column data utility

Options:
  -h, --help            show this help message and exit
  -s SERVER, --server=SERVER
                        Hansoft server address
  -p PORT, --port=PORT  Hansoft server port
  -d DATABASE, --database=DATABASE
                        Hansoft database
  -u USERNAME, --username=USERNAME
                        Hansoft SDK username
  -p PASSWORD, --password=PASSWORD
                        Hansoft SDK password
  -r PROJECT, --project=PROJECT
                        Hansoft project
  -f SOURCE, --source=SOURCE
                        Hansoft source column
  -t DESTINATION, --destination=DESTINATION
                        Hansoft destination column
```

## Limitations
Tested on OSX and Linux.

* Supports only custom columns.
* Support simple type column definitions, like text/number.
* Support single choice drop down menus.
