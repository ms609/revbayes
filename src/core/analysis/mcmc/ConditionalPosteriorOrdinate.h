#ifndef ConditionalPosteriorOrdinate_H
#define ConditionalPosteriorOrdinate_H

#include <iosfwd>
#include <vector>

#include "Cloneable.h"
#include "Parallelizable.h"


namespace RevBayesCore {
    
    /**
     * @brief ConditionalPosteriorOrdinate (CPO) class.
     *
     * The CPO analyzes the output of an MCMC run to compute the leave-one-out cross-validation fitness.
     *
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @since Version 1.2, 2022-09-12
     *
     */
    class ConditionalPosteriorOrdinate : public Cloneable, public Parallelizable {
        
    public:
        ConditionalPosteriorOrdinate(const std::string &fn, const std::string &pn, const std::string &ln, const std::string &del);      //!< Constructor initializing the object.
        virtual                                            ~ConditionalPosteriorOrdinate(void);                                         //!< Virtual destructor
        
        // public methods
        ConditionalPosteriorOrdinate*                       clone(void) const;                                                          //!< Create a deep copy
        double                                              marginalLikelihood( void ) const;                                           //!< Compute the marginal likelihood using SteppingStone-Sampler
        
    private:
        
        std::vector< std::vector<double> >                  samples;
    };
    
}

#endif
