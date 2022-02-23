/* ----------------------------------------------------------------------

   Config file code for ROV system

   Modification History:
   DATE               AUTHOR      COMMENT
   22 September 2001  J. Howland  created and written
   06 December  2001  J. Howland  munga changes in hotel_config
   29 December  2001  J. Howland  add notify threadids in hotel config
   06 February  2002  J. Howland  add power manager read in hotel config
   18 February  2002  J. Howland  add read of binary type flag to logging
                                  add read directory for logging
                                  add read total power budget
   11 March     2002  J. Howland  add always on to hotel    
   11 March     2002  J. Howland  move specific read_ini functions (like read sio)
                                  to their specific location   
   30 May       2002  J. Howland  add function to read a vector of doubles 
   31 May       2002  J. Howland   add function to read a hex                                           
   27 June      2002  LLW         Modified read_double_vec to permit whitespace 
   16 october 	2003  jch	 fix to correct bug that tim thiel found--white space in ini file
   

---------------------------------------------------------------------- */

/* standard ansi C header files */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* posix header files */
#define  POSIX_SOURCE 1

#include "dsLogTalk.h"
#include "config_file.h"
#include "sio_thread.h"

//#include "logging_thread.h"

#include "dsLog.h"



/* ----------------------------------------------------------------------

   rov_read_double

   Modification History:
   DATE               AUTHOR      COMMENT
   22 SEPTEMBER-2001  J. HOWLAND  Created and written.

---------------------------------------------------------------------- */
double
rov_read_double (FILE * config_fd, const char *section, const char *attribute, double default_value)
{
  int file_position, string_position, attribute_length, count;
  double double_value;

  char my_line[MAX_CHARACTER_COUNT], *ch;
  char identifier[MAX_CHARACTER_COUNT];
  char section_label[MAX_CHARACTER_COUNT];
  // form the section variable

#ifdef DEBUG_CONFIG
  printf ("entering read double, section: %s, attribute %s default value %g\n", section, attribute, default_value);

#endif

  string_position = snprintf (section_label, MAX_CHARACTER_COUNT,"[%s]", section);


  attribute_length = strlen (attribute);

  // go to the beginning of the file

  file_position = fseek (config_fd, 0, SEEK_SET);
  if (file_position == -1)
    {

#ifdef DEBUG_CONFIG
      printf (" couldn't seek to beginning of file in config read double\n");

#endif
      return default_value;
    }
  else
    {
      while (!feof (config_fd))
	{
	  ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
	  if (ch)
	    {
	      count = sscanf (my_line, "%s", identifier);
	      if ((count == 1) && (!strcasecmp (section_label, identifier)))
		{		// found the section, now read
		  // within it
#ifdef DEBUG_CONFIG
		  printf (" found section %s\n", identifier);

#endif
		  while (!feof (config_fd))
		    {
		      ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
		      if (my_line[0] == '[')
			{	// this is a new section
#ifdef DEBUG_CONFIG
			  printf (" got to end of section %s without finding %s\n", identifier, attribute);

#endif
			  return default_value;
			}
		      if (!strncasecmp (&(my_line[0]), attribute, attribute_length))
			{	// the first part is right
			  if (my_line[attribute_length] == '=')
			    {	// the = is in the right place
			      count = sscanf (&(my_line[attribute_length + 1]), "%lf", &double_value);
			      if (count == 1)
				return double_value;
			      else
				return default_value;
			    }	// no equal sign, keep reading
			}	// attribute doesn't match, keep reading 
		    }		// end while not feof  looking within section
		}		// not the section identifier, keep reading
	    }			// end of got my_line
	  else
	    return default_value;
	}			// end while not feof looking for section
    }				// end of valid file pointer, must have not found what we were looking for
  return default_value;
}


int rov_read_double_vec (FILE * config_fd, const char *section, const char *attribute, double *vec, int vec_len,double default_value)
{


  int file_position, string_position, attribute_length, count;
  char my_line[MAX_CHARACTER_COUNT], *ch;
  char identifier[MAX_CHARACTER_COUNT];
  char section_label[MAX_CHARACTER_COUNT];
  int stat=1;
  int i;
  char        str[1024];
  int         len = 0;
  int         chars_already_read = 0;
  
  
  for( i=0; i < vec_len; i++)
    {
      vec[i] = default_value;
    }
  
#ifdef DEBUG_READ_VECTOR
  printf ("ROV_READ_VEC entering read double vec, section: %s, attribute %s default value %f\n", section, attribute, default_value);

#endif

  string_position = snprintf (section_label,MAX_CHARACTER_COUNT, "[%s]", section);


  attribute_length = strlen (attribute);

  // go to the beginning of the file

  file_position = fseek (config_fd, 0, SEEK_SET);
  if (file_position == -1)
    {

#ifdef DEBUG_READ_VECTOR
      printf ("ROV_READ_VEC couldn't seek to beginning of file in config read double vec\n");

#endif
      
      return -1;
    }
  else{ while (!feof (config_fd))
    {
      ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
      if (ch)
	{
	  count = sscanf (my_line, "%s", identifier);
#ifdef DEBUG_READ_VECTOR
	  printf (" ROV_READ_VEC looking for section read  %s\n", identifier);

#endif      
	  if ((count == 1) && (!strcasecmp (section_label, identifier)))
	    {
	      // found the section, now read
	      // within it
#ifdef DEBUG_READ_VECTOR
	      printf (" ROV_READ_VEC found section %s\n", identifier);

#endif
	      while (!feof (config_fd)  )
		{

		  ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
#ifdef DEBUG_READ_VECTOR
		  printf (" ROV_READ_VEC in section read %s\n", ch);

#endif            
		  if (my_line[0] == '[')
		    {	// this is a new section
#ifdef DEBUG_READ_VECTOR
		      printf (" ROV_READ_VEC got to end of section %s without finding %s\n", identifier, attribute);

#endif
			        
              
		      return 1;
		    } // in new section
		  if (!strncasecmp (&(my_line[0]), attribute, attribute_length))
		    {
#ifdef DEBUG_READ_VECTOR
		      printf (" ROV_READ_VEC matched attribute %s \n",attribute);

#endif            
		      // the first part is right
		      if (my_line[attribute_length] == '=')
			{
			  // the = is in the right place
			  // take out the brackets

			  // 27 June 2002 LLW commented out 
			  // sscanf(&(my_line[attribute_length+1]),"[ %[^]]s",str);
			  // 27 June 2002 LLW revised to permit leading spaces before the [
			  sscanf(&(my_line[attribute_length+1])," [ %[^]]s",str);

#ifdef DEBUG_READ_VECTOR
			  printf (" ROV_READ_VEC string to parse = %s\n",str);

#endif
			  for(i=0; ((i < vec_len) && (stat == 1)); i++)
			    {
			      stat = sscanf(&str[chars_already_read],"%lg%n",&vec[i],&len);
#ifdef DEBUG_READ_VECTOR
			      printf (" ROV_READ_VEC read value %f %d characters\n",vec[i],len);

#endif                   
			      chars_already_read +=len;
			    }
			  if ((i == vec_len) && (stat == 1))
			    {
                  
			      stat = 0;
                  
			      return 0;
			    }
			  else
			    {
			      stat = 1;
			      return 1;
			    }
			      
			}	// no equal sign, keep reading
		    }	// attribute doesn't match, keep reading 
		}		// end while not feof  looking within section
	    }		// not the section identifier, keep reading
	}			// end of got my_line
      else
	{
    
         
	  return -1;
	}
    }			// end while not feof looking for section
  }				// end of valid file pointer, must have not found what we were looking for
  return stat;


}



//--------------------------------------------------------------------------------
int
rov_read_int (FILE * config_fd, const char *section, const char *attribute, int default_value)
{
  int file_position, string_position, attribute_length, count, integer_value;


  char my_line[MAX_CHARACTER_COUNT], *ch;
  char identifier[MAX_CHARACTER_COUNT];
  char section_label[MAX_CHARACTER_COUNT];
  // form the section variable

#ifdef DEBUG_CONFIG
  printf ("entering read int, section: %s, attribute %s default value %d\n", section, attribute, default_value);

#endif

  string_position = snprintf (section_label, MAX_CHARACTER_COUNT,"[%s]", section);


  attribute_length = strlen (attribute);

  // go to the beginning of the file

  file_position = fseek (config_fd, 0, SEEK_SET);
  if (file_position == -1)
    {

#ifdef DEBUG_CONFIG
      printf (" couldn't seek to beginning of file in config read int\n");

#endif
      return default_value;
    }
  else
    {
      while (!feof (config_fd))
	{
	  ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
	  if (ch)
	    {
	      count = sscanf (my_line, "%s", identifier);
	      if ((count == 1) && (!strcasecmp (section_label, identifier)))
		{		// found the section, now read
		  // within it
#ifdef DEBUG_CONFIG
		  printf (" found section %s\n", identifier);

#endif
		  while (!feof (config_fd))
		    {
		      ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
		      if (my_line[0] == '[')
			{	// this is a new section
#ifdef DEBUG_CONFIG
			  printf (" got to end of section %s without finding %s\n", identifier, attribute);

#endif
			  return default_value;
			}
		      if (!strncasecmp (&(my_line[0]), attribute, attribute_length))
			{	// the first part is right
			  if (my_line[attribute_length] == '=')
			    {	// the = is in the right place
			      count = sscanf (&(my_line[attribute_length + 1]), "%d", &integer_value);
			      if (count == 1)
				return integer_value;
			      else
				return default_value;
			    }	// no equal sign, keep reading
			}	// attribute doesn't match, keep reading 
		    }		// end while not feof  looking within section
		}		// not the section identifier, keep reading
	    }			// end of got my_line
	  else
	    return default_value;
	}			// end while not feof looking for section
    }				// end of valid file pointer, must have not found what we were looking for
  return default_value;
}

//--------------------------------------------------------------------------------
int
rov_read_hex (FILE * config_fd, const char *section, const char *attribute, int default_value)
{
  int file_position, string_position, attribute_length, count;
  unsigned int integer_value;


  char my_line[MAX_CHARACTER_COUNT], *ch;
  char identifier[MAX_CHARACTER_COUNT];
  char section_label[MAX_CHARACTER_COUNT];
  // form the section variable

#ifdef DEBUG_CONFIG
  printf ("entering read int, section: %s, attribute %s default value %d\n", section, attribute, default_value);

#endif

  string_position = snprintf (section_label, MAX_CHARACTER_COUNT,"[%s]", section);


  attribute_length = strlen (attribute);

  // go to the beginning of the file

  file_position = fseek (config_fd, 0, SEEK_SET);
  if (file_position == -1)
    {

#ifdef DEBUG_CONFIG
      printf (" couldn't seek to beginning of file in config read int\n");

#endif
      return default_value;
    }
  else
    {
      while (!feof (config_fd))
	{
	  ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
	  if (ch)
	    {
	      count = sscanf (my_line, "%s", identifier);
	      if ((count == 1) && (!strcasecmp (section_label, identifier)))
		{		// found the section, now read
		  // within it
#ifdef DEBUG_CONFIG
		  printf (" found section %s\n", identifier);

#endif
		  while (!feof (config_fd))
		    {
		      ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
		      if (my_line[0] == '[')
			{	// this is a new section
#ifdef DEBUG_CONFIG
			  printf (" got to end of section %s without finding %s\n", identifier, attribute);

#endif
			  return default_value;
			}
		      if (!strncasecmp (&(my_line[0]), attribute, attribute_length))
			{	// the first part is right
			  if (my_line[attribute_length] == '=')
			    {	// the = is in the right place
			      count = sscanf (&(my_line[attribute_length + 1]), "%x", &integer_value);
			      if (count == 1)
				return integer_value;
			      else
				return default_value;
			    }	// no equal sign, keep reading
			}	// attribute doesn't match, keep reading 
		    }		// end while not feof  looking within section
		}		// not the section identifier, keep reading
	    }			// end of got my_line
	  else
	    return default_value;
	}			// end while not feof looking for section
    }				// end of valid file pointer, must have not found what we were looking for
  return default_value;
}

/* ----------------------------------------------------------------------

   rov_read_string

   Modification History:
   DATE               AUTHOR      COMMENT
   22 SEPTEMBER-2001  J. HOWLAND  Created and written.

---------------------------------------------------------------------- */
char *
rov_read_string (FILE * config_fd, const char *section, const char *attribute, const char *default_value)
{
  int file_position, string_position, attribute_length, count, j, my_char_position;

  // change this to put in a strdup

  char my_line[MAX_CHARACTER_COUNT], *ch;
  char identifier[MAX_CHARACTER_COUNT];
  char section_label[MAX_CHARACTER_COUNT];
  char new_string[MAX_CHARACTER_COUNT];


  // form the section variable

  string_position = snprintf (section_label, MAX_CHARACTER_COUNT,"[%s]", section);


  attribute_length = strlen (attribute);

  // go to the beginning of the file

  file_position = fseek (config_fd, 0, SEEK_SET);
  if (file_position == -1)
    {
      // heres where I should print an error message if debug is on
      return strdup (default_value);
    }
  else
    {
      while (!feof (config_fd))
	{
	  ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
	  if (ch)
	    {
	      count = sscanf (my_line, "%s", identifier);
	      if ((count == 1) && (!strcasecmp (section_label, identifier)))
		{		// found the section, now read
		  // within it
		  while (!feof (config_fd))
		    {
		      ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, config_fd);
		      if (my_line[0] == '[')	// this is a new section
			return strdup (default_value);
		      if (!strncasecmp (&(my_line[0]), attribute, attribute_length))
			{	// the first part is right
			  if (my_line[attribute_length] == '=')
			    {	// the = is in the right place
			      // check for quotation marks
			      if (my_line[attribute_length + 1] == '"')
				{
				  j = 0;
				  my_char_position = attribute_length + 2;
				  while ((my_line[my_char_position] != '"') && (my_char_position < MAX_CHARACTER_COUNT))
				    {
				      new_string[j] = my_line[my_char_position++];
				      j++;
				    }
				  count = 1;
				  new_string[j] = '\0';
				}
			      else
				count = sscanf (&(my_line[attribute_length + 1]), "%s", new_string);
			      if ((count == 1) && (strlen (new_string) < MAX_CHARACTER_COUNT))
				{
				  return strdup (new_string);
				}
			      else
				return strdup (default_value);
			    }	// no equal sign, keep reading
			}	// attribute doesn't match, keep reading 
		    }		// end while not feof  looking within section
		}		// not the section identifier, keep reading
	    }			// end of got my_line
	  else
	    return strdup (default_value);
	}			// end while not feof looking for section
    }				// end of valid file pointer, must have not found what we were looking for
  return strdup (default_value);
}



