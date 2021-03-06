/* silly.c - silly command shell */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
// Required for wait
#include <sys/wait.h>
// Required for file handling
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Constants
const char MAX_TOKEN = 99;

// Function prototypes
int doCmd(char * tokenAry[]);
void say(const char *msg, int smile);

int main()
{
   char line[256];
   char prompt[] = "o_O";
   
   // Pointer array to up to 100 tokens
   char * tokenAry[MAX_TOKEN];
   
   printf("Silly Shell, for CS 3376 by Robert Brooks / rab120130@utdallas.edu\n");
   say("Hi! I'm a silly shell, and I'm at your 'command'.\n", 1);
   /* prompt me! */
   printf("%s ", prompt);
   /* Input... INPUT!!! */
   while(fgets(line, sizeof line, stdin) != NULL)
   {
      /* Get rid of the cr */
      if (strlen(line) == 0) return 0;
      line[strlen(line)-1] = '\0';
      
      /* Tokenize the line, using space as delimiter (note that this munges the original string) */
      int curTokPtr = 0;
      char *curTok = strtok(line, " ");
      
      if (curTok == NULL) {
         say("You didn't give me anything to do, so I think I'll go home now. Bye!\n", 1);
         return 0;
      }
   
      while (curTok != NULL)
      {
         //printf ("%s\n",curTok);
         if (curTokPtr > MAX_TOKEN) {
            // Well, that was a lot of typing wasted, wasn't it?
            say("Oh man, you typed so much I couldn't pay attention any more. Sorry!\n", 1);
            return 1;
         }
         tokenAry[curTokPtr] = curTok;
   
         curTokPtr++;
         curTok = strtok (NULL, " ");
      } // while more tokens    
   
      if (curTokPtr <= MAX_TOKEN) {
         tokenAry[curTokPtr] = curTok; // null
      }
   
      //say("Here's what I saw when you hit Enter:\n", 1);
      //int i;
      //for (i = 0; i < MAX_TOKEN; i++) {
      //   if (tokenAry[i] == NULL) break;
      //   printf("%s\n", tokenAry[i]);
      //}

      if (strcasecmp(tokenAry[0], "exit") == 0) {
         say("You said 'exit'\n", 1);
         say("Let's blow this pop stand!\n", 1);
         return 0;
      }

      // We have a command! Run with it!
      int retVal = doCmd(tokenAry);
      // If nonzero returned, some sort of error happened, so quit
      if (retVal != 0) return retVal;
      
      // That was fun! Let's do it again!
      printf("%s ", prompt);
   } // while another line
   
   return 0;
} // end main


// Give the silly shell a bit of personality
void say(const char *msg, int smile) {
   if (smile) {   
      // Codes to make bright/black/yellow and then reset/white/black (see http://www.linuxjournal.com/article/8603)
      printf("%c[%d;%d;%dm<#_#>", 0x1B, 1, 0 + 30, 3 + 40);
   }
   // Would like to set to reset/yellow/black, but it sticks like that.
   // printf("%c[%d;%d;%dm", 0x1B, 0, 3 + 30, 0 + 40);
   // So for now, just reset to reset/white/black here instead of after output.
   printf("%c[%d;%d;%dm", 0x1B, 0, 7 + 30, 0 + 40);
   if (smile) {
      printf(" ");
   }
   printf("%s", msg);
}

// Perform the command (will be called recursively)
int doCmd(char * tokenAry[]) {
   //char temp[256]; // temp string for messages about the line
   //char filename[256];
   int fdRedir;
   
   // We have a command. How far does it go?
   int nextCommand = 0; // 0 = end  
   int idxDelim;
   // Note, we could start with a delimiter (grep silly < silly.c | less)
   for (idxDelim = 0; idxDelim < MAX_TOKEN; idxDelim++) {
      if (tokenAry[idxDelim] == NULL) { nextCommand = 0; break; }
      if (strcmp(tokenAry[idxDelim], "<") == 0) { nextCommand = 1; break; }
      if (strcmp(tokenAry[idxDelim], ">") == 0) { nextCommand = 2; break; }
      if (strcmp(tokenAry[idxDelim], ">>") == 0) { nextCommand = 3; break; }
      if (strcmp(tokenAry[idxDelim], "|") == 0) { nextCommand = 4; break; }
      if (strcmp(tokenAry[idxDelim], "&") == 0) { nextCommand = 5; break; }
   }
   
   // Do we have anything to do? If not, exit
   if (nextCommand == 0 && idxDelim == 0) {
      say("Hey, you left me hanging! No command to execute.\n", 1);
      return 1;
   }   

   // Send off a child and see what it does
   int childPid = fork();
   
   if (childPid != 0) {
      // Parent here. All we can do is sit by the door and wait for the kid to come home.
      if (nextCommand != 5) {
         // Wait for the child to complete
         //say("Waiting for the command...\n", 1);
         // waitpid(-1, &status, 0); // wait for all children, but we want to wait for specific child
         waitpid(childPid, NULL, 0); // null because we're not checking status (yet)
         //say("All done!\n", 1);
      } // if we need to wait
      //else {
      //   say("I'm not waiting up for you!\n", 1);
      //}
      
      // That's it! The parent is all done. Return to caller
      return 0;
   }
   
   // If we're here, we're the child process, and we need to do whatever the tokens in our head say to do.
   //say("Performing the command...\n", 1);

   // Do we need another child process?
   if (nextCommand == 4) {
      // I'm the parent now. Do what I say.
      // I will execute the command I see now and pipe stdout to the child.
      // The child is going to execute the next command, and pipe stdin from me
      // element 0 = input, element 1 = output
      int pipefd[2];
      pipe(pipefd);
      // Now do the classic swap around from sunysb.edu handout
      int grandChild = fork();
      if (grandChild != 0) {
         // I'm the new parent
         //say("I'm the new parent!\n", 1);
         // Switch my output (1) to the pipe's "write" end (1)
         dup2(pipefd[1], 1);
         // Get rid of the pipe's "write" (it's duplicate)
         close(pipefd[1]);
         // And get rid of the "read" end (the child already has it)
         close(pipefd[0]);

         // We don't care about anything after the end of the command, so zap it at the pipe.
         // "The array of pointers must be terminated by a NULL pointer."
         tokenAry[idxDelim] = NULL;

         // int execvp(const char *file, char *const argv[]);
         execvp(tokenAry[0], tokenAry);
         say("Something went terribly wrong with the command (attempting to pipe, parent) Sorry!\n", 1);
         return 1;
      }
      else {
         // I'm the baby
         //say("I'm the new baby!\n", 1);
         // Switch my input (0) to the pipe's "read" end (0)
         dup2(pipefd[0], 0);
         // Get rid of the pipe's "read" (it's duplicate)
         close(pipefd[0]);
         // And get rid of the pipe's "write" end (the parent already has it)
         close(pipefd[1]);
         
         // Now, run the command *after* the pipe symbol.
         // My stdin is already redirected, so just run with it!
         return doCmd(&tokenAry[idxDelim + 1]);
      }
      
      // (We never get here - returns in both branches)

   } // end if command = pipe "|"      

   // We're not piping to a child. Do we need to redirect i/o?
   switch (nextCommand)
   {
      case 1:
      case 2:
      case 3:
         // Next token is filename
         // We could have something like "< in.txt | less".
         // Not sure how to handle that yet.
         if (tokenAry[idxDelim+1] == NULL) {
            say("Hey, you told me there was a file, but you didn't give me one!\n", 1);
            return 1;
         }
         //printf("%s\n", tokenAry[idxDelim+1]);
         
         // Open the file with the low-level command so we get a File Descriptor
         // int open(const char *pathname, int flags)
         switch (nextCommand)
         {
            case 1: // <
               // Get a FD for reading, and change stdin to use it
               fdRedir = open(tokenAry[idxDelim+1], O_RDONLY);
               dup2(fdRedir, 0);
               break;
            case 2: // >
               // Get a FD for writing, and change stdout to use it
               fdRedir = open(tokenAry[idxDelim+1], O_WRONLY | O_CREAT);
               dup2(fdRedir, 1);
               break;
            case 3: // >>
               fdRedir = open(tokenAry[idxDelim+1], O_WRONLY | O_APPEND);
               dup2(fdRedir, 1);
               break;
            //default:
         } // end switch (where we treat each differently)

         // Don't need both, right?
         //close(fdRedir);
         break;

      //default:
   } // end switch (where we get a filename if needed)
   
   // We don't care what made the command line end, just zap it
   // (Won't hurt the child or parent process, we got our own copy)
   // "The array of pointers must be terminated by a NULL pointer."
   tokenAry[idxDelim] = NULL;
   
   // int execvp(const char *file, char *const argv[]);
   execvp(tokenAry[0], tokenAry);
   // We only get here on error
   say("Something went terribly wrong with the command. Sorry!\n", 1);
   return 1;

} // end doExec()
