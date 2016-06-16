/*******************************************************************************
** Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

/** @file
 * Specification of the internal cluster analysis communication protocol.
 *
 * @note    The prefix "ANCI_" is being used below as as short-hand for
 *          "ArgoNavis::Clustering::Impl::", which is the namespace into
 *          which these types would be placed if XDR was more C++-aware.
 */

%#include "KrellInstitute/Messages/Thread.h"



/**
 * Unique identifier used to reference threads within the other messages below
 * without having to always resort to CBTF_Protocol_ThreadName, which is quite
 * large in comparison.
 */
typedef uint64_t ANCI_ThreadUID;



/**
 * Emit performance data for the specified thread. Issued by ClusteringManager
 * to request the performance data and associated address space information for
 * a single thread.
 */
struct ANCI_EmitPerformanceData
{
    ANCI_ThreadUID thread; /**< UID of the requested thread. */
};



/**
 * Current state of the cluster analysis. ...
 */
struct ANCI_State
{
    /** Name of the feature vector. */
    /** string name<>; */

    int dummy; /**< Temporary dummy value. */
};



/**
 * Table of all threads known to a given clustering component. Issued by both
 * ClusteringLeaf and ClusteringFilter to inform their parent of their current
 * ANCI_ThreadUID to CBTF_Protocol_ThreadName mapping.
 */
struct ANCI_ThreadTable
{
    ANCI_ThreadUID uids<>;            /**< Unique identifiers of all threads. */
    CBTF_Protocol_ThreadName names<>; /**< Name of all threads. */
};
