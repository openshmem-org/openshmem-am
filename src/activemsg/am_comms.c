/*
 *
 * Copyright (c) 2011 - 2016
 *   University of Houston System and UT-Battelle, LLC.
 * Copyright (c) 2009 - 2016
 *   Silicon Graphics International Corp.  SHMEM is copyrighted
 *   by Silicon Graphics International Corp. (SGI) The OpenSHMEM API
 *   (shmem) is released by Open Source Software Solutions, Inc., under an
 *   agreement with Silicon Graphics International Corp. (SGI).
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * o Neither the name of the University of Houston System,
 *   UT-Battelle, LLC. nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>              /* NULL */
#include <string.h>             /* memcpy() */
#include <sys/types.h>          /* size_t */
#include "trace.h"

#include "shmem.h"
#include "comms/comms.h"
#include "uthash.h"

#ifdef HAVE_FEATURE_PSHMEM
#include "pshmem.h"
#endif /* HAVE_FEATURE_PSHMEM */

#if defined(HAVE_FEATURE_EXPERIMENTAL)

struct shmemx_am_handler2id_map *am_maphashptr=NULL;
volatile int request_cnt = 0;


void 
shmemx_am_attach (int function_id, shmemx_am_handler_w_token function_handler)
{
   struct shmemx_am_handler2id_map* temp_handler_entry;
   temp_handler_entry = (struct shmemx_am_handler2id_map*) malloc(sizeof(struct shmemx_am_handler2id_map));
   temp_handler_entry->id = function_id;
   temp_handler_entry->fn_ptr = function_handler;
   HASH_ADD_INT(am_maphashptr, id, temp_handler_entry);
   /* shmem attach is a collective operation */
   shmem_barrier_all();
}

void
shmemx_am_detach(int handler_id)
{
   struct shmemx_am_handler2id_map* temp_handler_entry;
   HASH_FIND_INT( am_maphashptr, &handler_id, temp_handler_entry );
   HASH_DEL( am_maphashptr, temp_handler_entry);  /* delete - pointer to handler */
   free(am_maphashptr);                           /* optional */
}


void
shmemx_am_request(int dest, int handler_id, void* source_addr, size_t nbytes)
{
   request_cnt++;
   //atomic_inc_am_counter();
   GASNET_SAFE(gasnet_AMRequestMedium2 
		   (dest, GASNET_HANDLER_activemsg_request_handler, 
		    source_addr, nbytes, 
		    handler_id, 
		    shmem_my_pe()));
}


void
shmemx_am_reply(int handler_id, void* source_addr, size_t nbytes, shmemx_am_token_t temp_token)
{
   GASNET_SAFE(gasnet_AMReplyMedium2 
		  ((gasnet_token_t)temp_token->gasnet_token, GASNET_HANDLER_activemsg_reply_handler, 
	           source_addr, nbytes, 
		   handler_id, 
		   shmem_my_pe()));
   temp_token->is_reply_called = 1;
}



void
shmemx_am_quiet()
{
   //atomic_wait_am_zero();
   GASNET_BLOCKUNTIL(request_cnt==0);
}


void 
shmemx_am_poll()
{
   gasnet_AMPoll();
}

#endif /* HAVE_FEATURE_EXPERIMENTAL */
