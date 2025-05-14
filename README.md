Ver 1.0 -----------------------------------------------------
- Boiler plate code contain serialPortHandler class and MainWindow class : used for sending and receiving commands with port error handling.

Ver 1.1 -----------------------------------------------------
- before appending data in buffer checking conditions it is not empty nor huge size
- Added Mutex for Thread Safety.

Ver 1.2 -------------------------------------------------------
- Caution comments introduced.
 
Ver 1.3 --------------------------------------------------------
- All caution comments are implemented.
- Added responseTimer, File logging system, PowerId implementation to avoid QByteRef out of bond error, chksum , float conversion functions in serialPortHandler class also,

Ver 1.4 --------------------------------------------------------
- Eliminated circular inclusion warning especially for writeToNotes() with executeWriteToNotes() from static function to signal.

Ver 1.5 ------------------------------------------------------
- added checksum function.

Ver 1.6 ------------------------------------------------------
- added packet structure command for easy packet making.

