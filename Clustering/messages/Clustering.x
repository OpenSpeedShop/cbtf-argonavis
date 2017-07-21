/*******************************************************************************
** Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Specification of the cluster analysis communication protocol. */

%#include "KrellInstitute/Messages/Thread.h"



/** Representation of a data cluster */
struct Cluster
{
    /** Representative thread for this cluster. */
    CBTF_Protocol_ThreadName representative;

    /** All threads in this cluster. */
    CBTF_Protocol_ThreadName threads<>;
};



/** Representation of a data clustering criterion. */
struct Clustering_Criterion
{
    string name<>;      /**< Name of this criterion. */
    Cluster clusters<>; /**< Clusters for this criterion. */
};
