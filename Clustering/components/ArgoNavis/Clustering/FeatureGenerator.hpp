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

#pragma once

#include <boost/shared_ptr.hpp>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Messages/Blob.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

#include <ArgoNavis/Clustering/FeatureVector.hpp>

namespace ArgoNavis { namespace Clustering {

    /**
     * Abstract base class for a CBTF component that generates feature vectors
     * for clustering analysis. Such components work directly with performance
     * data blobs and thus are typically specific to a particular collector.
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
         * Emit the given clustering feature. Called zero or more times by the
         * derived class' onEmitFeatures() implementation.
         *
         * @param feature    Clustering feature vector to be emitted.
         */
        void emitFeature(const FeatureVector& feature);

        /**
         * Emit the given observed address. Called zero or more times by the
         * derived class' onPerformanceData() implementation. These observed
         * addresses are used to inform Open|SpeedShop which addresses need
         * to be resolved to symbols in order to view the performance data.
         *
         * @param address    Address observed in a performance data blob.
         */
        void emitObservedAddress(const Base::Address& address);

        /**
         * Emit the given performance data blob. Called zero or more times by
         * the derived class' onEmitPerformanceData() implementation.
         *
         * @param blob    Performance data blob to be emitted.
         */
        void emitPerformanceData(
            const boost::shared_ptr<CBTF_Protocol_Blob>& blob
            );

        /**
         * Get the address spaces of all observed threads. Called by the derived
         * class' onEmitFeatures() implementation when necessary to generate the
         * clustering features.
         *
         * @return    Address spaces of all observed threads.
         */
        const Base::AddressSpaces& spaces() const;

        /**
         * Callback invoked in order to request the emission of all clustering
         * features associated with the specified thread. The derived class'
         * implementation should emit the requested clustering features via
         * emitFeature().
         *
         * @param thread    Thread for which to emit clustering features.
         */
        virtual void onEmitFeatures(const Base::ThreadName& thread) = 0;

        /**
         * Callback invoked in order to request the emission of all performance
         * data blobs associated with the specified thread. The derived class'
         * implementation should emit the requested performance data blobs via
         * emitPerformanceData().
         *
         * @param thread    Thread for which to emit performance data blobs.
         */
        virtual void onEmitPerformanceData(const Base::ThreadName& thread) = 0;
        
        /**
         * Callback invoked when a performance data blob is received from the
         * collector. The derived class' implementation should store the blob,
         * possibly in a form suitable for clustering feature generation, and
         * be prepared to emit the performance data blobs associated with a
         * particular thread when requested via onEmitPerformanceData().
         *
         * @param blob    Performance data blob received from the collector.
         */
        virtual void onPerformanceData(
            const boost::shared_ptr<CBTF_Protocol_Blob>& blob
            ) = 0;

    private:

        /** Handler for the "AddressSpaces" input. */
        void handleAddressSpaces(const Base::AddressSpaces& spaces);

        /** Handler for the "EmitFeatures" input. */
        void handleEmitFeatures(const Base::ThreadName& thread);
        
        /** Handler for the "EmitPerformanceData" input. */
        void handleEmitPerformanceData(const Base::ThreadName& thread);

        /** Handler for the "PerformanceData" input. */
        void handlePerformanceData(
            const boost::shared_ptr<CBTF_Protocol_Blob>& message
            );

        /** Address spaces of all observed threads. */
        Base::AddressSpaces dm_spaces;
        
    }; // class FeatureGenerator

} } // namespace ArgoNavis::Clustering
