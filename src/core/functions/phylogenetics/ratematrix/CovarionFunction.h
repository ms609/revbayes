#ifndef CovarionFunction_H
#define CovarionFunction_H

#include "TypedFunction.h"
#include "RateGenerator.h"

namespace RevBayesCore {
class DagNode;
template <class valueType> class RbVector;
template <class valueType> class TypedDagNode;
    
    /**
     * @brief Hidden state rate matrix function.
     *
     * The RevLanguage wrapper of the Free-K rate matrix connects
     * the variables/parameters of the function and creates the internal CovarionFunction object.
     * Please read the CovarionFunction.h for more info.
     *
     * @author Sebastian Hoehna & Lyndon Coghill
     *
     */
    class CovarionFunction : public TypedFunction<RateGenerator> {
        
    public:
        CovarionFunction(bool r);
        virtual                                            ~CovarionFunction(void);                                      //!< Virtual destructor
        
        // public member functions
        CovarionFunction*                                   clone(void) const;                                                          //!< Create an independent clone
        void                                                update(void);
        void                                                setRateMatrices(const TypedDagNode< RbVector<RateGenerator> > *rm);
        void                                                setRateScalars(const TypedDagNode< RbVector<double> > *tr);
        void                                                setSwitchRates(const TypedDagNode< RbVector<RbVector<double> > > *tr);
        
    protected:
        void                                                swapParameterInternal(const DagNode *oldP, const DagNode *newP);            //!< Implementation of swaping parameters
        
    private:
        
        // members
        bool                                                rescale;
        const TypedDagNode<RbVector<RateGenerator> >*       rate_matrices;
        const TypedDagNode<RbVector<double> >*              rate_scalars;
        const TypedDagNode<RbVector<RbVector<double> > >*   switch_rates;
        
    };
    
}

#endif
