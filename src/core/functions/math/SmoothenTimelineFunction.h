#ifndef SmoothenTimelineFunction_H
#define SmoothenTimelineFunction_H

#include <stddef.h>

#include "RbVector.h"
#include "TypedFunction.h"

namespace RevBayesCore {
class DagNode;
template <class valueType> class TypedDagNode;

    /**
     * @brief Smoothen a timeline by setting all values after a given age to its previous value (recursively).
     *
     * This function takes theta[1] on the log or non-log scale, and the first-order differences (theta[i] - theta[i-1]).
     * It then computes the entire vector theta, optionally exponentiating the output first.
     *
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team
     * @since Version 1.2, 2023-02-25
     *
     */
    class SmoothenTimelineFunction : public TypedFunction<RbVector<double> > {

    public:
        SmoothenTimelineFunction(const TypedDagNode< double > *max_t, const TypedDagNode< RbVector<double> > *times, const TypedDagNode< RbVector<double> > *values);
        virtual                                            ~SmoothenTimelineFunction(void);                                                    //!< Virtual destructor

        // public member functions
        SmoothenTimelineFunction*                           clone(void) const;                                                              //!< Create an independent clone
        void                                                update(void);

    protected:
        void                                                swapParameterInternal(const DagNode *oldP, const DagNode *newP);                        //!< Implementation of swaping parameters

    private:

        // members
        const TypedDagNode< RbVector<double> >*             times;
        const TypedDagNode< RbVector<double> >*             values;
        const TypedDagNode< double >*                       max_time;

    };

}

#endif