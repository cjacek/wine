/*
 * Server-side USER handles
 *
 * Copyright (C) 2001 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "thread.h"
#include "process.h"
#include "user.h"
#include "request.h"

static USER_HANDLE_ENTRY *handles;
static USER_HANDLE_ENTRY *freelist;
static struct user_session_info *session_info;

static USER_HANDLE_ENTRY *handle_to_entry( user_handle_t handle )
{
    unsigned short generation;
    unsigned int index = handle & 0xffff;
    if (index >= session_info->nb_handles || !handles[index].type) return NULL;
    generation = handle >> 16;
    if (generation == handles[index].generation || !generation || generation == 0xffff)
        return &handles[index];
    return NULL;
}

static inline user_handle_t entry_to_handle( USER_HANDLE_ENTRY *ptr )
{
    return (ptr - handles) | (ptr->generation << 16);
}

static void *get_entry_obj_ptr( USER_HANDLE_ENTRY *ptr )
{
    return (void *)(UINT_PTR)ptr->object;
}

static void *free_user_entry( USER_HANDLE_ENTRY *ptr )
{
    void *ret = get_entry_obj_ptr( ptr );
    atomic_store_ulong( &ptr->uniq, ptr->generation << 16 );
    ptr->object = (UINT_PTR)freelist;
    freelist = ptr;
    return ret;
}

/* initialize session shared data */
void init_session_shared_data( void *ptr )
{
    session_info = ptr;
    handles = (void *)(session_info + 1);
    atomic_store_ulong( &session_info->nb_handles, FIRST_USER_HANDLE );
}

/* allocate a user handle for a given object */
user_handle_t alloc_user_handle( void *ptr, unsigned int type )
{
    USER_HANDLE_ENTRY *handle;
    unsigned int generation;

    if (freelist)
    {
        handle = freelist;
        freelist = get_entry_obj_ptr( handle );
        generation = handle->generation + 1;
        if (generation >= 0xffff) generation = 1;
    }
    else
    {
        unsigned int nb_handles = session_info->nb_handles;
        if (nb_handles == LAST_USER_HANDLE) return 0;
        handle = &handles[nb_handles];
        atomic_store_ulong( &session_info->nb_handles, nb_handles + 1 );
        generation = 1;
    }

    handle->object = (UINT_PTR)ptr;
    atomic_store_ulong( &handle->tid, current->id );
    atomic_store_ulong( &handle->pid, current->process->id );
    atomic_store_ulong( &handle->uniq, generation << 16 | type );
    return entry_to_handle( handle );
}

/* return a pointer to a user object from its handle */
void *get_user_object( user_handle_t handle, unsigned int type )
{
    USER_HANDLE_ENTRY *entry;

    if (!(entry = handle_to_entry( handle )) || entry->type != type) return NULL;
    return get_entry_obj_ptr( entry );
}

/* get the full handle for a possibly truncated handle */
user_handle_t get_user_full_handle( user_handle_t handle )
{
    USER_HANDLE_ENTRY *entry;

    if (handle >> 16) return handle;
    if (!(entry = handle_to_entry( handle ))) return handle;
    return entry_to_handle( entry );
}

/* same as get_user_object plus set the handle to the full 32-bit value */
void *get_user_object_handle( user_handle_t *handle, unsigned int type )
{
    USER_HANDLE_ENTRY *entry;

    if (!(entry = handle_to_entry( *handle )) || entry->type != type) return NULL;
    *handle = entry_to_handle( entry );
    return get_entry_obj_ptr( entry );
}

/* free a user handle and return a pointer to the object */
void *free_user_handle( user_handle_t handle )
{
    USER_HANDLE_ENTRY *entry;

    if (!(entry = handle_to_entry( handle )))
    {
        set_error( STATUS_INVALID_HANDLE );
        return NULL;
    }
    return free_user_entry( entry );
}

/* return the next user handle after 'handle' that is of a given type */
void *next_user_handle( user_handle_t *handle, unsigned int type )
{
    unsigned int nb_handles = session_info->nb_handles;
    USER_HANDLE_ENTRY *entry;

    if (!*handle) entry = handles;
    else
    {
        unsigned int index = *handle & 0xffff;
        if (index >= nb_handles) return NULL;
        entry = handles + index + 1;  /* start from the next one */
    }
    while (entry < handles + nb_handles)
    {
        if (!type || entry->type == type)
        {
            *handle = entry_to_handle( entry );
            return get_entry_obj_ptr( entry );
        }
        entry++;
    }
    return NULL;
}

static int is_client_type( unsigned int type )
{
    return type && type != NTUSER_OBJ_WINDOW && type != NTUSER_OBJ_HOOK;
}

/* free client-side user handles managed by the process */
void free_process_user_handles( struct process *process )
{
    unsigned int i, nb_handles = session_info->nb_handles;

    for (i = 0; i < nb_handles; i++)
        if (is_client_type( handles[i].type ) && get_entry_obj_ptr( &handles[i] ) == process)
            free_user_entry( &handles[i] );
}

/* allocate an arbitrary user handle */
DECL_HANDLER(alloc_user_handle)
{
    if (is_client_type( req->type ))
        reply->handle = alloc_user_handle( current->process, req->type );
    else
        set_error( STATUS_INVALID_PARAMETER );
}


/* free an arbitrary user handle */
DECL_HANDLER(free_user_handle)
{
    USER_HANDLE_ENTRY *entry;

    if ((entry = handle_to_entry( req->handle )) && is_client_type( entry->type ))
        free_user_entry( entry );
    else
        set_error( STATUS_INVALID_HANDLE );
}
