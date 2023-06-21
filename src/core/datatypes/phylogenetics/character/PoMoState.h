#ifndef PoMoState_H
#define PoMoState_H

#include <stddef.h>
#include <ostream>
#include <vector>

#include "DiscreteCharacterState.h"
#include "RbBitSet.h"

namespace RevBayesCore {
    
    class PoMoState : public DiscreteCharacterState {
        
    public:
        enum WEIGHTING { FIXED, BINOMIAL, SAMPLED };

        PoMoState(size_t n=4, size_t vps = 10, const std::string &s = "", const std::string &chromosome = "",
                  size_t position = 0);                                                   //!< Constructor that sets all fields
        
        PoMoState*                      clone(void) const;                                 //!< Get a copy of this object
        
        // Discrete character observation functions
        void                            addState(const std::string &symbol);                //!< Add a character state to the set of character states
        RbBitSet                        getState(void) const;                               //!< Get the state (as the bitset)
        void                            setToFirstState(void);                              //!< Set this character state to the first (lowest) possible state
        void                            setState(const std::vector<size_t> &counts);        //!< Compute the internal state value for this character.
        void                            setState(const std::string &symbol);                //!< Compute the internal state value for this character.
        void                            setStateByIndex(size_t index);                      //!< Set the discrete observation
        
        std::string                     getDataType(void) const;                            //!< Get the datatype as a common string.
        std::string                     getStateLabels(void) const;                         //!< Get valid state labels
        std::string                     getStringValue(void) const;                         //!< Get a representation of the character as a string
        virtual const std::string&      nexusSeparator(void) const;                         //!< Get the separator for printing in nexus format
        void                            setVirtualPopulationSize(size_t populationSize);    //!< Set the virtual population size for the state space
        void                            setChromosome(std::string chromosome);              //!< Set the chromosome for the state
        void                            setPosition(size_t position);                       //!< Set the position for the state
        const std::string&              getChromosome( void ) const;                        //!< Get the chromosome for the state
        size_t                          getPosition( void ) const;                          //!< Get the position for the state
        
        const std::vector<double>&      getWeights( void ) const;                            //!< Get the weight of the state
        bool                            isWeighted( void ) const;
        void                            setWeighted( bool tf );
        bool                            isGapState(void) const;                             //!< Get whether this is a gapped character state
        bool                            isMissingState(void) const;                         //!< Get whether this is a missing character state
        bool                            isStateIncludedInAscertainmentBiasCorrection(void) const;  //!< Is the currently set state included in ascertainment bias correction
        void                            setGapState(bool tf);                               //!< set whether this is a gapped character
        void                            setMissingState(bool tf);                           //!< set whether this is a missing character
        
    private:
        void                            populateWeightedStatesForMonoallelicState(size_t id1, int sum); //!< Sets the weights of all the states compatible with a monoallelic state
        void                            setStateFixed(size_t t, size_t c, size_t b, size_t f, size_t s);   //!< Compute the internal state value for this frequency as the fixed average.
        void                            setStateBinomial(size_t t, size_t c, size_t b);     //!< Compute the internal state value as weights from a binomial distribution.
        void                            setStateBinomialForMonomorphic(size_t t, size_t f);     //!< Compute the internal state value as weights from a binomial distribution.
        void                            setStateSampled(size_t t, size_t c, size_t b);      //!< Compute the internal state value by sampling from a binomial distribution.

        bool                            is_gap;
        bool                            is_missing;
        size_t                          index_single_state;
        size_t                          virtual_population_size;                            //!< The virtual population size of the PoMo model (by default, 10)
        size_t                          num_raw_states;                                     //!< The number of raw states (4 for A,C,G and T)
        size_t                          num_pomo_states;                                    //!< The number of PoMo states
        size_t                          num_observed_states;
        RbBitSet                        state;
        
        std::string                     chromosome;                                         //!< The chromosome on which the state lies
        size_t                          position;                                           //!< The position of the state in the chromosome
        std::vector<double>             weights;                                            //!< Weights are used when the "average" option is used
        bool                            weighted;
        WEIGHTING                       weighting;
        std::string                     string_value;                                       //!< The string description of the state.
    };
    
}

#endif
