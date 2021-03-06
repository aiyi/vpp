/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vnet/mpls/mpls.h>
#include <vnet/dpo/mpls_label_dpo.h>
#include <vnet/dpo/load_balance.h>
#include <vnet/dpo/drop_dpo.h>

#include "fib_path_ext.h"
#include "fib_path.h"
#include "fib_path_list.h"
#include "fib_internal.h"

u8 *
format_fib_path_ext (u8 * s, va_list * args)
{
    fib_path_ext_t *path_ext;

    path_ext = va_arg (*args, fib_path_ext_t *);

    s = format(s, "path:%d label:%U",
	       path_ext->fpe_path_index,
	       format_mpls_unicast_label,
	       path_ext->fpe_path.frp_label);

    return (s);
}

int
fib_path_ext_cmp (fib_path_ext_t *path_ext,
		  const fib_route_path_t *rpath)
{
    return (fib_route_path_cmp(&path_ext->fpe_path, rpath));
}

static int
fib_path_ext_match (fib_node_index_t pl_index,
		    fib_node_index_t path_index,
		    void *ctx)
{
    fib_path_ext_t *path_ext = ctx;

    if (!fib_path_cmp_w_route_path(path_index,
				   &path_ext->fpe_path))
    {
	path_ext->fpe_path_index = path_index;
	return (0);
    }
    // keep going
    return (1);
}

void
fib_path_ext_resolve (fib_path_ext_t *path_ext,
		      fib_node_index_t path_list_index)
{
    /*
     * Find the path on the path list that this is an extension for
     */
    path_ext->fpe_path_index = FIB_NODE_INDEX_INVALID;
    fib_path_list_walk(path_list_index,
		       fib_path_ext_match,
		       path_ext);
}

void
fib_path_ext_init (fib_path_ext_t *path_ext,
		   fib_node_index_t path_list_index,
		   const fib_route_path_t *rpath)
{
    path_ext->fpe_path = *rpath;
    path_ext->fpe_path_index = FIB_NODE_INDEX_INVALID;

    fib_path_ext_resolve(path_ext, path_list_index);
}

load_balance_path_t *
fib_path_ext_stack (fib_path_ext_t *path_ext,
		    fib_forward_chain_type_t parent_fct,
		    load_balance_path_t *nhs)
{
    fib_forward_chain_type_t child_fct;
    load_balance_path_t *nh;

    if (!fib_path_is_resolved(path_ext->fpe_path_index))
	return (nhs);

    /*
     * Since we are stacking this path-extension, it must have a valid out
     * label. From the chain type request by the child, determine what
     * chain type we will request from the parent.
     */
    switch (parent_fct)
    {
    case FIB_FORW_CHAIN_TYPE_MPLS_EOS:
	ASSERT(0);
        return (nhs);
	break;
    case FIB_FORW_CHAIN_TYPE_UNICAST_IP4:
    case FIB_FORW_CHAIN_TYPE_UNICAST_IP6:
	if (MPLS_IETF_IMPLICIT_NULL_LABEL == path_ext->fpe_label)
	{
            /*
             * implicit-null label for the eos or IP chain, need to pick up
             * the IP adj
             */
	    child_fct = parent_fct;
	}
        else
        {
            /*
             * we have a label to stack. packets will thus be labelled when
             * they encounter th child, ergo, non-eos.
             */
	    child_fct = FIB_FORW_CHAIN_TYPE_MPLS_NON_EOS;
        }
	break;
    case FIB_FORW_CHAIN_TYPE_MPLS_NON_EOS:
        child_fct = parent_fct;
	break;
    default:
        return (nhs);
	break;
    }

    dpo_id_t via_dpo = DPO_INVALID;

    /*
     * The next object in the graph after the imposition of the label
     * will be the DPO contributed by the path through which the packets
     * are to be sent. We stack the MPLS Label DPO on this path DPO
     */
    fib_path_contribute_forwarding(path_ext->fpe_path_index,
				   child_fct,
				   &via_dpo);

    if (dpo_is_drop(&via_dpo) ||
	load_balance_is_drop(&via_dpo))
    {
	/*
	 * don't stack a path extension on a drop. doing so will create
	 * a LB bucket entry on drop, and we will lose a percentage of traffic.
	 */
    }
    else
    {
	vec_add2(nhs, nh, 1);
	nh->path_weight = fib_path_get_weight(path_ext->fpe_path_index);
	nh->path_index = path_ext->fpe_path_index;
	dpo_copy(&nh->path_dpo, &via_dpo);

	/*
	 * The label is stackable for this chain type
	 * construct the mpls header that will be imposed in the data-path
	 */
	if (MPLS_IETF_IMPLICIT_NULL_LABEL != path_ext->fpe_label)
	{
	    dpo_set(&nh->path_dpo,
		    DPO_MPLS_LABEL,
		    DPO_PROTO_MPLS,
		    mpls_label_dpo_create(path_ext->fpe_label,
                                          (parent_fct == FIB_FORW_CHAIN_TYPE_MPLS_NON_EOS ?
                                           MPLS_NON_EOS :
                                           MPLS_EOS),
                                          255, 0,
                                          &nh->path_dpo));
	}
    }
    dpo_reset(&via_dpo);

    return (nhs);
}
