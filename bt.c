/*
# Course: CS4760-001 - Operating System
# File Name: bt.c
# C program to show a breadth first direcotry search with various options.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>

int main(int argc, char *argv[])
{
  //variable declaration
     int indentation_space = 4;
     bool follow_symlink = false;
     char option_string[10];
  

  // use of getopt
  int opt;
	while((opt = getopt(argc, argv, "hI:Ldgipstul")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("NAME:\n");
				printf("	%s - traverse a specified directory in breadth-first order.\n", argv[0]);
				printf("\nUSAGE:\n");
				printf("	%s bt [-h] [-L -d -g -i -p -s -t -u | -l] [dirname].\n", argv[0]);
				printf("\nDESCRIPTION:\n");
				printf("	-h	: Print a help message and exit.\n");
				printf("	-L	: Follow symbolic links, if any. Default will be to not follow symbolic links.\n");
				printf("	-d	: Show the time of last modification.\n");
				printf("	-g	: Print the GID associated with the file.\n");
				printf("	-i	: Print the number of links to file in inode table.\n");
				printf("	-p	: Print permission bits as rwxrwxrwx.\n");
				printf("	-s	: Print the size of file in bytes.\n");
				printf("	-t	: Print information on file type.\n");
				printf("	-u	: Print the UID associated with the file.\n");
				printf("	-l	: Print information on the file as if the options \"tpiugs\" are all specified.\n\n");
				return EXIT_SUCCESS;

                        case 'I':
				indentation_space = atoi(optarg);
				break;
			
			case 'L':
				follow_symlink = true;
				break;
				
			case 'd':
				strcat(option_string, "d");
				break;
				
			case 'g':
				strcat(option_string, "g");
				break;
				
			case 'i':
				strcat(option_string, "i");
				break;
				
			case 'p':
				strcat(option_string, "p");
				break;
				
			case 's':
				strcat(option_string, "s");
				break;
				
			case 't':
				strcat(option_string, "t");
				break;
				
			case 'u':
				strcat(option_string, "u");
				break;
				
			case 'l':
				strcat(option_string, "tpiugs");
				break;

			default:
			        perror ("The following error occurred");
                                printf( "Value of errno: %d\n", errno );
                           	fprintf(stderr, "%s: Please use \"-h\" option for more info.\n", argv[0]);
				return EXIT_FAILURE;
		}
       }
      //If no directory were given, default to current working directory.
      	char *topdir, *targetdir, current[2]=".";
         	if(argv[optind] == NULL)
      	         {
      	    	  char origin[4096];
      	    	  getcwd(origin, sizeof(origin));
      	    	  topdir = origin;
      	    	  targetdir = current;
      	    	 }
      	    	else
      	    	{
      	    	topdir=argv[optind];
      	    	targetdir = topdir;
      	        }
      //Get longest name in the whole directory
      int longest_name;
      //lookfirst(&longest_name, topdir, 0, indentation_space, follow_symlink);
      //Execute breadthfirst search traverse directories
      printf("Directory scan of: %s\n", targetdir);
      //breadthfirst(topdir, 0, longest_name, indentation_space, follow_symlink, option_string);

	return EXIT_SUCCESS;
}
   


