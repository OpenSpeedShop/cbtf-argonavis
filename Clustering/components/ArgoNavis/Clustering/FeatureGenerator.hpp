////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the FeatureGenerator class. */

#include <boost/shared_ptr.hpp>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

namespace ArgoNavis { namespace Clustering {

    /**
     * Abstract base class for a component that generates clustering features.
     * ...
     */
    class FeatureGenerator :
        public KrellInstitute::CBTF::Component
    {
    
    protected:

        /**
         * Construct a new feature generator component of the given type and
         * version. Called from the constructor of a derived class to initalize
         * this base class.
         *
         * @param type       Type of this feature generator component.
         * @param version    Version of this feature generator component.
         */
        FeatureGenerator(const KrellInstitute::CBTF::Type& type,
                         const KrellInstitute::CBTF::Version& version);

        /**
         * Emit the given performance data blob on an outgoing upstream towards
         * the distributed component network's frontend (root). Called zero or
         * more times by the derived class' onEmitData() implementation.
         *
         * @param blob    Performance data blob to be emitted.
         */
        void emit(const boost::shared_ptr<CBTF_Protocol_Blob>& blob);
        
        /**
         * Emit the given clustering feature on an outgoing upstream towards
         * the distributed component network's frontend (root). Called zero
         * or more times by the derived class' onEmitFeatures() implementation.
         *
         * @param feature    Clustering feature to be emitted.
         */
        void emit(/* ... feature */);
        
        /**
         * Note the observation of the given address. Called zero or more times
         * by the derived class' onReceivedData() implementation. These observed
         * addresses are used to inform Open|SpeedShop which addresses need to
         * be resolved to symbols in order to view the performance data.
         *
         * @param address    Address observed in a performance data blob.
         */
        void observed(const Base::Address& address);

        /**
         * Callback invoked when a performance data blob is received from the
         * collector. The derived class' implementation should store the blob,
         * possibly in a form suitable for clustering feature generation, and
         * be prepared to emit the performance data blobs associated with a
         * particular thread when requested via onEmitData().
         *
         * @param blob    Performance data blob received from the collector.
         */
        virtual void onReceivedData(
            const boost::shared_ptr<CBTF_Protocol_Blob>& blob
            ) = 0;
        
        /**
         * Callback invoked in order to request the emission of all performance
         * data blobs associated with the specified thread. The derived class'
         * implementation should emit the requested performance data blobs via
         * emit().
         *
         * @param thread    Thread for which to emit performance data blobs.
         */
        virtual void onEmitData(const Base::ThreadName& thread) = 0;

        /**
         * Callback invoked in order to request the emission of all clustering
         * features associated with the specified thread. The derived class'
         * implementation should emit the requested clustering metrics via
         * emit().
         *
         * @param thread    Thread for which to emit clustering features.
         */
        virtual void onEmitFeatures(const Base::ThreadName& thread) = 0;

        /**
         * Get the address spaces of all observed threads. Called by the derived
         * class' onEmitFeatures() implementation when necessary to generate the
         * clustering features.
         *
         * @return    Address spaces of all observed threads.
         */
        const Base::AddressSpaces& spaces() const;

    private:

        /** Handler for the "Data" input. */
        void handleData(const boost::shared_ptr<CBTF_Protocol_Blob>& message);

        /** Handler for the "EmitAddressBuffer" input. */
        void handleEmitAddressBuffer(const bool& value);

        /** Handler for the "EmitData" input. */
        void handleEmitData(const Base::ThreadName& thread);

        /** Handler for the "EmitFeatures" input. */
        void handleEmitFeatures(const Base::ThreadName& thread);

        /** Handler for the "InitialLinkedObjects" input. */
        void handleInitialLinkedObjects(
            const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
            );
        
        /** Handler for the "LoadedLinkedObject" input. */
        void handleLoadedLinkedObject(
            const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
            );

        /** Handler for the "UnloadedLinkedObject" input. */
        void handleUnloadedLinkedObject(
            const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
            );

        /** Address buffer containing all of the observed addresses. */
        KrellInstitute::Core::AddressBuffer dm_addresses;
        
        /** Address spaces of all observed threads. */
        Base::AddressSpaces dm_address_spaces;
        
    }; // class FeatureGenerator

} } // namespace ArgoNavis::Clustering
