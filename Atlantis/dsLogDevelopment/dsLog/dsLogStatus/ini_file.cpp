/* this is a generic ini file reader written by Jonathan Howland of the Woods Hole Oceanographic Institution
It is a modification of the ini file reader initially written for the rov software development
program conducted jointly with Louis Whitcomb of the Johns Hopkins University.  The major change is the
"c++-ification" of the code

  */

#include <stdio.h>
#include <string.h>
#include "compilation.h"

#include "ini_file.h"


IniFile::IniFile()
{
	fd = NULL;
}

void IniFile::close_ini()
{

	fclose(fd);
}

void IniFile::setFD(FILE *inFD)
{
	fclose(fd);
	fd = inFD;
}
int	IniFile::open_ini(char *file_name)
{
	fd = fopen(file_name,"r");
	if(fd){
			return GOOD_INI_FILE_READ;
		}
	else{
			return BAD_INI_FILE_READ;
		}
}

int IniFile::read_int(const char *section, const char *attribute, int	default_int)
{
	int file_position, string_position, attribute_length, count, integer_value;


	char my_line[MAX_CHARACTER_COUNT], *ch;
	char identifier[MAX_CHARACTER_COUNT];
	char section_label[MAX_CHARACTER_COUNT];
	// form the section variable

	if(!fd){
			return default_int;
		}
	string_position = sprintf (section_label, "[%s]", section);
	attribute_length = strlen (attribute);

	// go to the beginning of the file	
	file_position = fseek (fd, 0, SEEK_SET);
	if (file_position == -1)
		{
			return default_int;
		}
	else
		{
			while (!feof (fd))
				{
					ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
					if (ch)
						{
							count = sscanf (my_line, "%s", identifier);
							if ((count == 1) && (!_stricmp (section_label, identifier))){		// found the section, now read within it
									while (!feof (fd)){
											ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
											if (my_line[0] == '[')
												{	// this is a new section
													return default_int;
												}
											if (!_strnicmp (&(my_line[0]), attribute, attribute_length)){	// the first part is right
													if (my_line[attribute_length] == '=')
														{	// the = is in the right place
															count = sscanf (&(my_line[attribute_length + 1]), "%d", &integer_value);
															if (count == 1)
																{
																	return integer_value;
																}
															else
																{
																	return default_int;
																}
														}	// no equal sign, keep reading
												}	// attribute doesn't match, keep reading
										}		// end while not feof  looking within section
								}		// not the section identifier, keep reading
						}			// end of got my_line
					else
						{
							return default_int;
						}
				}			// end while not feof looking for section
		}				// end of valid file pointer, must have not found what we were looking for
	return default_int;
}

int IniFile::read_HexInt(const char *section, const char *attribute, int	default_int)
{
	int file_position, string_position, attribute_length, count, integer_value;


	char my_line[MAX_CHARACTER_COUNT], *ch;
	char identifier[MAX_CHARACTER_COUNT];
	char section_label[MAX_CHARACTER_COUNT];
	// form the section variable

	if(!fd)
		{
			return default_int;
		}
	string_position = sprintf (section_label, "[%s]", section);
	attribute_length = strlen (attribute);

	// go to the beginning of the file	
	file_position = fseek (fd, 0, SEEK_SET);
	if (file_position == -1)
		{
			return default_int;
		}
	else
		{
			while (!feof (fd))
				{
					ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
					if (ch){
							count = sscanf (my_line, "%s", identifier);
							if ((count == 1) && (!_stricmp (section_label, identifier))){		// found the section, now read within it
									while (!feof (fd))
										{
											ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
											if (my_line[0] == '[')
												{	// this is a new section
													return default_int;
												}
											if (!_strnicmp (&(my_line[0]), attribute, attribute_length))
												{	// the first part is right
													if (my_line[attribute_length] == '='){	// the = is in the right place
															count = sscanf (&(my_line[attribute_length + 1]), "%x", &integer_value);
															if (count == 1)
																{
																	return integer_value;
																}
															else
																{
																	return default_int;
																}
														}	// no equal sign, keep reading
												}	// attribute doesn't match, keep reading
										}		// end while not feof  looking within section
								}		// not the section identifier, keep reading
						}			// end of got my_line
					else{
							return default_int;
						}
				}			// end while not feof looking for section
		}				// end of valid file pointer, must have not found what we were looking for
	return default_int;
}
double  IniFile::read_double (const char *section, const char *attribute, double default_double)
{
	int file_position, string_position, attribute_length, count;
	double double_value;

	char my_line[MAX_CHARACTER_COUNT], *ch;
	char identifier[MAX_CHARACTER_COUNT];
	char section_label[MAX_CHARACTER_COUNT];
	// form the section variable

	if(!fd)
		{
			return default_double;
		}
	string_position = sprintf (section_label, "[%s]", section);
	attribute_length = strlen (attribute);

	// go to the beginning of the file

	file_position = fseek (fd, 0, SEEK_SET);
	if (file_position == -1)
		{
			return default_double;
		}
	else{
			while (!feof (fd))
				{
					ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
					if (ch)
						{
							count = sscanf (my_line, "%s", identifier);
							if ((count == 1) && (!_stricmp (section_label, identifier))
								){		// found the section, now read within it
									while (!feof (fd)){
											ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
											if (my_line[0] == '[')
												{	// this is a new section
													return default_double;
												}
											if (!_strnicmp (&(my_line[0]), attribute, attribute_length))
												{	// the first part is right
													if (my_line[attribute_length] == '=')
														{	// the = is in the right place
															count = sscanf (&(my_line[attribute_length + 1]), "%lf", &double_value);
															if (count == 1)
																{
																	return double_value;
																}
															else
																{
																	return default_double;
																}
														}	// no equal sign, keep reading
												}	// attribute doesn't match, keep reading
										}		// end while not feof  looking within section
								}		// not the section identifier, keep reading
						}			// end of got my_line
					else{
							return default_double;
						}
				}			// end while not feof looking for section
		}				// end of valid file pointer, must have not found what we were looking for
	return default_double;
}

char *IniFile::read_string (const char *section, const char *attribute,const  char *default_string)
{
	int file_position, string_position, attribute_length, count, j, my_char_position;

	// change this to put in a strdup

	char my_line[MAX_CHARACTER_COUNT], *ch;
	char identifier[MAX_CHARACTER_COUNT];
	char section_label[MAX_CHARACTER_COUNT];
	char new_string[MAX_CHARACTER_COUNT];


	// form the section variable
	if(!fd)
		{
			return strdup (default_string);;
		}
	string_position = sprintf (section_label, "[%s]", section);
	attribute_length = strlen (attribute);

	// go to the beginning of the file

	file_position = fseek (fd, 0, SEEK_SET);
	if (file_position == -1)
		{
			return strdup (default_string);
		}
	else{
			while (!feof (fd))
				{
					ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
					if (ch){
							count = sscanf (my_line, "%s", identifier);
							if ((count == 1) && (!_stricmp (section_label, identifier))){		// found the section, now read within it
									while (!feof (fd))
										{
											ch = fgets (&(my_line[0]), MAX_CHARACTER_COUNT - 1, fd);
											if (my_line[0] == '[')
												{	// this is a new section
													return strdup (default_string);
												}
											if (!_strnicmp (&(my_line[0]), attribute, attribute_length))
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
																{
																	count = sscanf (&(my_line[attribute_length + 1]), "%s", new_string);
																}
															if ((count == 1) && (strlen (new_string) < MAX_CHARACTER_COUNT))
																{
																	return strdup (new_string);
																}
															else
																{
																	return strdup (default_string);
																}
														}	// no equal sign, keep reading
												}	// attribute doesn't match, keep reading
										}		// end while not feof  looking within section
								}		// not the section identifier, keep reading
						}			// end of got my_line
					else{
							return strdup (default_string);
						}
				}			// end while not feof looking for section
		}				// end of valid file pointer, must have not found what we were looking for
	return strdup (default_string);
}
