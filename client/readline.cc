/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* readline for batch mode */

#include <drizzled/global.h>
#include <mysys/my_sys.h>
#include <mystrings/m_string.h>
#include "my_readline.h"

using namespace std;

static bool init_line_buffer(LINE_BUFFER *buffer,File file,uint32_t size,
			    uint32_t max_size);
static bool init_line_buffer_from_string(LINE_BUFFER *buffer,char * str);
static size_t fill_buffer(LINE_BUFFER *buffer);
static char *intern_read_line(LINE_BUFFER *buffer,uint32_t *out_length);


LINE_BUFFER *batch_readline_init(uint32_t max_size,FILE *file)
{
  LINE_BUFFER *line_buff;
  if (!(line_buff=(LINE_BUFFER*) malloc(sizeof(*line_buff))))
    return 0;
  memset(line_buff, 0, sizeof(*line_buff));
  if (init_line_buffer(line_buff,fileno(file),IO_SIZE,max_size))
  {
    free(line_buff);
    return 0;
  }
  return line_buff;
}


char *batch_readline(LINE_BUFFER *line_buff)
{
  char *pos;
  uint32_t out_length;

  if (!(pos=intern_read_line(line_buff,&out_length)))
    return 0;
  if (out_length && pos[out_length-1] == '\n')
    if (--out_length && pos[out_length-1] == '\r')  /* Remove '\n' */
      out_length--;                                 /* Remove '\r' */
  line_buff->read_length=out_length;
  pos[out_length]=0;
  return pos;
}


void batch_readline_end(LINE_BUFFER *line_buff)
{
  if (line_buff)
  {
    free(line_buff->buffer);
    free(line_buff);
  }
}


LINE_BUFFER *batch_readline_command(LINE_BUFFER *line_buff, char * str)
{
  if (!line_buff)
  {
    if (!(line_buff=(LINE_BUFFER*) malloc(sizeof(*line_buff))))
      return 0;
    memset(line_buff, 0, sizeof(*line_buff));
  }
  if (init_line_buffer_from_string(line_buff,str))
  {
    free(line_buff);
    return 0;
  }
  return line_buff;
}


/*****************************************************************************
      Functions to handle buffered readings of lines from a stream
******************************************************************************/

static bool
init_line_buffer(LINE_BUFFER *buffer,File file,uint32_t size,uint32_t max_buffer)
{
  buffer->file=file;
  buffer->bufread=size;
  buffer->max_size=max_buffer;
  if (!(buffer->buffer = (char*) malloc(buffer->bufread+1)))
    return 1;
  buffer->end_of_line=buffer->end=buffer->buffer;
  buffer->buffer[0]=0;				/* For easy start test */
  return 0;
}

/*
  init_line_buffer_from_string can be called on the same buffer
  several times. the resulting buffer will contain a
  concatenation of all strings separated by spaces
*/
static bool init_line_buffer_from_string(LINE_BUFFER *buffer,char * str)
{
  uint32_t old_length=(uint)(buffer->end - buffer->buffer);
  uint32_t length= (uint) strlen(str);
  char * tmpptr= (char*)realloc(buffer->buffer, old_length+length+2);
  if (tmpptr == NULL)
    return 1;
  
  buffer->buffer= buffer->start_of_line= buffer->end_of_line= tmpptr;
  buffer->end= buffer->buffer + old_length;
  if (old_length)
    buffer->end[-1]=' ';
  memcpy(buffer->end, str, length);
  buffer->end[length]= '\n';
  buffer->end[length+1]= 0;
  buffer->end+= length+1;
  buffer->eof=1;
  buffer->max_size=1;
  return 0;
}


/*
  Fill the buffer retaining the last n bytes at the beginning of the
  newly filled buffer (for backward context).	Returns the number of new
  bytes read from disk.
*/

static size_t fill_buffer(LINE_BUFFER *buffer)
{
  size_t read_count;
  uint32_t bufbytes= (uint) (buffer->end - buffer->start_of_line);

  if (buffer->eof)
    return 0;					/* Everything read */

  /* See if we need to grow the buffer. */

  for (;;)
  {
    uint32_t start_offset=(uint) (buffer->start_of_line - buffer->buffer);
    read_count=(buffer->bufread - bufbytes)/IO_SIZE;
    if ((read_count*=IO_SIZE))
      break;
    buffer->bufread *= 2;
    if (!(buffer->buffer = (char*) realloc(buffer->buffer,
                                           buffer->bufread+1)))
      return (uint) -1;
    buffer->start_of_line=buffer->buffer+start_offset;
    buffer->end=buffer->buffer+bufbytes;
  }

  /* Shift stuff down. */
  if (buffer->start_of_line != buffer->buffer)
  {
    memmove(buffer->buffer, buffer->start_of_line, (uint) bufbytes);
    buffer->end=buffer->buffer+bufbytes;
  }

  /* Read in new stuff. */
  if ((read_count= my_read(buffer->file, (unsigned char*) buffer->end, read_count,
			   MYF(MY_WME))) == MY_FILE_ERROR)
    return (size_t) -1;

  /* Kludge to pretend every nonempty file ends with a newline. */
  if (!read_count && bufbytes && buffer->end[-1] != '\n')
  {
    buffer->eof = read_count = 1;
    *buffer->end = '\n';
  }
  buffer->end_of_line=(buffer->start_of_line=buffer->buffer)+bufbytes;
  buffer->end+=read_count;
  *buffer->end=0;				/* Sentinel */
  return read_count;
}



char *intern_read_line(LINE_BUFFER *buffer,uint32_t *out_length)
{
  char *pos;
  size_t length;


  buffer->start_of_line=buffer->end_of_line;
  for (;;)
  {
    pos=buffer->end_of_line;
    while (*pos != '\n' && *pos)
      pos++;
    if (pos == buffer->end)
    {
      if ((uint) (pos - buffer->start_of_line) < buffer->max_size)
      {
	if (!(length=fill_buffer(buffer)) || length == (size_t) -1)
	  return(0);
	continue;
      }
      pos--;					/* break line here */
    }
    buffer->end_of_line=pos+1;
    *out_length=(uint32_t) (pos + 1 - buffer->eof - buffer->start_of_line);
    return(buffer->start_of_line);
  }
}
