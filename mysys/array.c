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

/* Handling of arrays that can grow dynamicly. */

#include "mysys_priv.h"
#include <mystrings/m_string.h>

/*
  Initiate dynamic array

  SYNOPSIS
    init_dynamic_array2()
      array		Pointer to an array
      element_size	Size of element
      init_buffer       Initial buffer pointer
      init_alloc	Number of initial elements
      alloc_increment	Increment for adding new elements

  DESCRIPTION
    init_dynamic_array() initiates array and allocate space for 
    init_alloc eilements. 
    Array is usable even if space allocation failed.
    Static buffers must begin immediately after the array structure.

  RETURN VALUE
    true	my_malloc_ci() failed
    false	Ok
*/

bool init_dynamic_array2(DYNAMIC_ARRAY *array, uint32_t element_size,
                            void *init_buffer, uint32_t init_alloc, 
                            uint32_t alloc_increment CALLER_INFO_PROTO)
{
  if (!alloc_increment)
  {
    alloc_increment=cmax((8192-MALLOC_OVERHEAD)/element_size,16);
    if (init_alloc > 8 && alloc_increment > init_alloc * 2)
      alloc_increment=init_alloc*2;
  }

  if (!init_alloc)
  {
    init_alloc=alloc_increment;
    init_buffer= 0;
  }
  array->elements=0;
  array->max_element=init_alloc;
  array->alloc_increment=alloc_increment;
  array->size_of_element=element_size;
  if ((array->buffer= init_buffer))
    return(false);
  if (!(array->buffer=(unsigned char*) my_malloc_ci(element_size*init_alloc,
                                            MYF(MY_WME))))
  {
    array->max_element=0;
    return(true);
  }
  return(false);
} 

bool init_dynamic_array(DYNAMIC_ARRAY *array, uint32_t element_size,
                           uint32_t init_alloc, 
                           uint32_t alloc_increment CALLER_INFO_PROTO)
{
  /* placeholder to preserve ABI */
  return my_init_dynamic_array_ci(array, element_size, init_alloc, 
                                  alloc_increment);
}
/*
  Insert element at the end of array. Allocate memory if needed.

  SYNOPSIS
    insert_dynamic()
      array
      element

  RETURN VALUE
    true	Insert failed
    false	Ok
*/

bool insert_dynamic(DYNAMIC_ARRAY *array, unsigned char* element)
{
  unsigned char* buffer;
  if (array->elements == array->max_element)
  {						/* Call only when nessesary */
    if (!(buffer=alloc_dynamic(array)))
      return true;
  }
  else
  {
    buffer=array->buffer+(array->elements * array->size_of_element);
    array->elements++;
  }
  memcpy(buffer,element,(size_t) array->size_of_element);
  return false;
}


/*
  Alloc space for next element(s) 

  SYNOPSIS
    alloc_dynamic()
      array

  DESCRIPTION
    alloc_dynamic() checks if there is empty space for at least
    one element if not tries to allocate space for alloc_increment
    elements at the end of array.

  RETURN VALUE
    pointer	Pointer to empty space for element
    0		Error
*/

unsigned char *alloc_dynamic(DYNAMIC_ARRAY *array)
{
  if (array->elements == array->max_element)
  {
    char *new_ptr;
    if (array->buffer == (unsigned char *)(array + 1))
    {
      /*
        In this senerio, the buffer is statically preallocated,
        so we have to create an all-new malloc since we overflowed
      */
      if (!(new_ptr= (char *) my_malloc((array->max_element+
                                         array->alloc_increment) *
                                        array->size_of_element,
                                        MYF(MY_WME))))
        return 0;
      memcpy(new_ptr, array->buffer, 
             array->elements * array->size_of_element);
    }
    else
    if (!(new_ptr=(char*) my_realloc(array->buffer,(array->max_element+
                                     array->alloc_increment)*
                                     array->size_of_element,
                                     MYF(MY_WME | MY_ALLOW_ZERO_PTR))))
      return 0;
    array->buffer= (unsigned char*) new_ptr;
    array->max_element+=array->alloc_increment;
  }
  return array->buffer+(array->elements++ * array->size_of_element);
}


/*
  Pop last element from array.

  SYNOPSIS
    pop_dynamic()
      array
  
  RETURN VALUE    
    pointer	Ok
    0		Array is empty
*/

unsigned char *pop_dynamic(DYNAMIC_ARRAY *array)
{
  if (array->elements)
    return array->buffer+(--array->elements * array->size_of_element);
  return 0;
}

/*
  Replace element in array with given element and index

  SYNOPSIS
    set_dynamic()
      array
      element	Element to be inserted
      idx	Index where element is to be inserted

  DESCRIPTION
    set_dynamic() replaces element in array. 
    If idx > max_element insert new element. Allocate memory if needed. 
 
  RETURN VALUE
    true	Idx was out of range and allocation of new memory failed
    false	Ok
*/

bool set_dynamic(DYNAMIC_ARRAY *array, unsigned char* element, uint32_t idx)
{
  if (idx >= array->elements)
  {
    if (idx >= array->max_element && allocate_dynamic(array, idx))
      return true;
    memset(array->buffer+array->elements*array->size_of_element, 0,
           (idx - array->elements)*array->size_of_element);
    array->elements=idx+1;
  }
  memcpy(array->buffer+(idx * array->size_of_element),element,
	 (size_t) array->size_of_element);
  return false;
}


/*
  Ensure that dynamic array has enough elements

  SYNOPSIS
    allocate_dynamic()
    array
    max_elements        Numbers of elements that is needed

  NOTES
   Any new allocated element are NOT initialized

  RETURN VALUE
    false	Ok
    true	Allocation of new memory failed
*/

bool allocate_dynamic(DYNAMIC_ARRAY *array, uint32_t max_elements)
{
  if (max_elements >= array->max_element)
  {
    uint32_t size;
    unsigned char *new_ptr;
    size= (max_elements + array->alloc_increment)/array->alloc_increment;
    size*= array->alloc_increment;
    if (array->buffer == (unsigned char *)(array + 1))
    {
       /*
         In this senerio, the buffer is statically preallocated,
         so we have to create an all-new malloc since we overflowed
       */
       if (!(new_ptr= (unsigned char *) my_malloc(size *
                                         array->size_of_element,
                                         MYF(MY_WME))))
         return 0;
       memcpy(new_ptr, array->buffer, 
              array->elements * array->size_of_element);
     }
     else


    if (!(new_ptr= (unsigned char*) my_realloc(array->buffer,size*
                                       array->size_of_element,
                                       MYF(MY_WME | MY_ALLOW_ZERO_PTR))))
      return true;
    array->buffer= new_ptr;
    array->max_element= size;
  }
  return false;
}


/*
  Get an element from array by given index

  SYNOPSIS
    get_dynamic()
      array	
      unsigned char*	Element to be returned. If idx > elements contain zeroes.
      idx	Index of element wanted. 
*/

void get_dynamic(DYNAMIC_ARRAY *array, unsigned char* element, uint32_t idx)
{
  if (idx >= array->elements)
  {
    memset(element, 0, array->size_of_element);
    return;
  }
  memcpy(element,array->buffer+idx*array->size_of_element,
         (size_t) array->size_of_element);
}


/*
  Empty array by freeing all memory

  SYNOPSIS
    delete_dynamic()
      array	Array to be deleted
*/

void delete_dynamic(DYNAMIC_ARRAY *array)
{
  /*
    Just mark as empty if we are using a static buffer
  */
  if (array->buffer == (unsigned char *)(array + 1))
    array->elements= 0;
  else
  if (array->buffer)
  {
    free(array->buffer);
    array->buffer=0;
    array->elements=array->max_element=0;
  }
}

/*
  Delete element by given index

  SYNOPSIS
    delete_dynamic_element()
      array
      idx        Index of element to be deleted
*/

void delete_dynamic_element(DYNAMIC_ARRAY *array, uint32_t idx)
{
  char *ptr= (char*) array->buffer+array->size_of_element*idx;
  array->elements--;
  memmove(ptr,ptr+array->size_of_element,
          (array->elements-idx)*array->size_of_element);
}


/*
  Free unused memory

  SYNOPSIS
    freeze_size()
      array	Array to be freed

*/

void freeze_size(DYNAMIC_ARRAY *array)
{
  uint32_t elements=cmax(array->elements,1);

  /*
    Do nothing if we are using a static buffer
  */
  if (array->buffer == (unsigned char *)(array + 1))
    return;
    
  if (array->buffer && array->max_element != elements)
  {
    array->buffer=(unsigned char*) my_realloc(array->buffer,
                                     elements*array->size_of_element,
                                     MYF(MY_WME));
    array->max_element=elements;
  }
}


/*
  Get the index of a dynamic element

  SYNOPSIS
    get_index_dynamic()
     array	Array
     element Whose element index 

*/

int get_index_dynamic(DYNAMIC_ARRAY *array, unsigned char* element)
{
  uint32_t ret;
  if (array->buffer > element)
    return -1;

  ret= (element - array->buffer) /  array->size_of_element;
  if (ret > array->elements)
    return -1;

  return ret;

}
