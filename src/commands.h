#ifndef commands_h_
#define commands_h_

/* Command definitions*/
struct commands
{
    char *name;             /* User printable name of the function. */
    bool (*func)(char *);   /* Function to call to do the job. */
    char *doc;              /* Documentation for this function.  */
    bool req_args;          /* True if the commands expects arguments */
};

extern struct commands commands[];

#endif /*commands_h_*/

