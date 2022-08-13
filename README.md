# Interactive Calendar

Group members:
- Ian Havenaar
- Simran Moolchandaney
- Jacob Schwartz

Assigned TAs:
- Alamin Mohammed
- Luke Siela

Due date: Friday, March 25th @ 10 PM

## Usage
- make (ran in ComputerNetworksProject01 directory): creates ./client/mycal and ./server/mycalserver executables
- ./mycal CalendarName action -> data for action <- (ran under client subdirectory)
- ./mycalserver 41113 (ran under server subdirectory) *use -mt flag at the end of the command if you want to use forked version that allows for multiple clients to connect
- make clean: removes executables 
## Notes
- the ./server/data folder was not included at startup so it'll be created 
- the sample json format we expect can be found in ./client/test.json (e.g. to run the input command: ./client/mycal calendar_name input ./client/test.json)
        

