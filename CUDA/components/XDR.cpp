////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Registration of XDR/MRNet conversion components. */

#include <KrellInstitute/CBTF/XDR.hpp>

#include <KrellInstitute/Messages/IO_data.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include "CUDA_data.h"

KRELL_INSTITUTE_CBTF_REGISTER_XDR_CONVERTERS(CBTF_Protocol_LinkedObjectGroup)
KRELL_INSTITUTE_CBTF_REGISTER_XDR_CONVERTERS(CBTF_Protocol_SymbolTable)
KRELL_INSTITUTE_CBTF_REGISTER_XDR_CONVERTERS(CBTF_Protocol_ThreadsStateChanged)
KRELL_INSTITUTE_CBTF_REGISTER_XDR_CONVERTERS(CBTF_cuda_data)
KRELL_INSTITUTE_CBTF_REGISTER_XDR_CONVERTERS(CBTF_io_trace_data)
