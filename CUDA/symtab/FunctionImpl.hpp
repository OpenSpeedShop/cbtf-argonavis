////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2007 William Hachfeld. All Rights Reserved.
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

/** @file Declaration of the FunctionImpl class. */

#pragma once

#include <boost/operators.hpp>
#include <set>
#include <string>

namespace KrellInstitute { namespace SymbolTable { namespace impl {

    class LinkedObject;
    class Statement;

    /**
     * Implementation details of the Function class. Anything that
     * would normally be a private member of Function is instead a
     * member of FunctionImpl.
     */
    class FunctionImpl :
        public boost::totally_ordered<FunctionImpl>
    {

    public:
        
        /** Destructor. */
        virtual ~FunctionImpl();
        
        /** Replace this function with a copy of another one. */
        FunctionImpl& operator=(const FunctionImpl& other);

        /** Is this function less than another one? */
        bool operator<(const FunctionImpl& other) const;

        /** Is this function equal to another one? */
        bool operator==(const FunctionImpl& other) const;
        
        /** Get the linked object containing this function. */
        LinkedObject getLinkedObject() const;

        /** Get the mangled name of this function. */
        std::string getMangledName() const;
        
        /** Get the demangled name of this function. */
        std::string getDemangledName(const bool& all = true) const;
        
        /** Get the definitions of this function. */
        std::set<Statement> getDefinitions() const;

        /** Get the statements associated with this function. */
        std::set<Statement> getStatements() const;
        
    private:
        
        // ...
        
    }; // class FunctionImpl

} } // namespace KrellInstitute::SymbolTable
