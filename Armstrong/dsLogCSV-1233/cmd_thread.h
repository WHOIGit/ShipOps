/* ----------------------------------------------------------------------

   cmd thread for ROV control system

   Modification History:
   DATE         AUTHOR  COMMENT
   14-JUL-2000  LLW      Created and written.

---------------------------------------------------------------------- */
#ifndef CMD_PROCESS_INC
#define CMD_PROCESS_INC

// ----------------------------------------------------------------------
// DEBUG FLAG:  Uncomment this and recompile to get verbosr debugging 
// ----------------------------------------------------------------------
//#define DEBUG_CMD
// ----------------------------------------------------------------------

extern void *cmd_thread (void *my_thread_address);

#endif
