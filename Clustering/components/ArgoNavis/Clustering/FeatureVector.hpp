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

/** @file Declaration of the Feature class. */

#include <stdexcept>
#include <string>
#include <vector>

#include <ArgoNavis/Base/Raise.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

#include <ArgoNavis/Clustering/Feature.hpp>

namespace ArgoNavis { namespace Clustering {

    /**
     * Feature vector characterizing aspects of a single thread's performance.
     * Includes a name which is used to ensure only same-named feature vectors
     * from different threads are compared against each other.
     *    
     * @sa https://en.wikipedia.org/wiki/Feature_vector
     */
    class FeatureVector
    {
        
    public:

        /**
         * Construct an empty (zero-dimensional) feature vector.
         *
         * @param name      Name of this feature vector.
         * @param thread    Name of the thread charaterized
         *                  by this feature vector.
         */
        FeatureVector(const std::string& name, const Base::ThreadName& thread) :
            dm_name(name),
            dm_thread(thread),
            dm_features()
        {
        }

        /** Get the name of this feature vector. */
        const std::string& name() const
        {
            return dm_name;
        }
        
        /** Get the name of the thread characterized by this feature vector. */
        const Base::ThreadName& thread() const
        {
            return dm_thread;
        }

        /** Get the individual features in this feature vector. */
        const std::vector<Feature> features() const
        {
            return dm_features;
        }
        
        /** Are the features in this feature vector named? */
        bool named() const
        {
            return dm_features.empty() ? false : !dm_features[0].name().empty();
        }
        
        /** Add a feature to this feature vector. */
        void add(const Feature& feature)
        {
            if (!dm_features.empty())
            {
                if (feature.name().empty() != dm_features[0].name().empty())
                {
                    Base::raise<std::invalid_argument>(
                        "A feature vector must contain all named, or all "
                        "unnamed, features. Not a combination of both."
                        );
                }
            }
            
            dm_features.push_back(feature);
        }
        
    private:

        /** Name of this feature vector. */
        const std::string dm_name;
        
        /** Name of the thread characterized by this feature vector. */
        const Base::ThreadName dm_thread;

        /** Individual features in this feature vector. */
        std::vector<Feature> dm_features;
        
    }; // class FeatureVector

} } // namespace ArgoNavis::Clustering
